/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"
#include "common/recording.h"
#include "kubernetes.h"
#include "terminal/terminal.h"
#include "url.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <libwebsockets.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * Handles data received from Kubernetes over WebSocket, decoding the channel
 * index of the received data and forwarding that data accordingly.
 *
 * @param client
 *     The guac_client associated with the connection to Kubernetes.
 *
 * @param buffer
 *     The data received from Kubernetes.
 *
 * @param length
 *     The size of the data received from Kubernetes, in bytes.
 */
static void guac_kubernetes_receive_data(guac_client* client,
        const char* buffer, size_t length) {

    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    /* Strip channel index from beginning of buffer */
    int channel = *(buffer++);
    length--;

    switch (channel) {

        /* Write STDOUT / STDERR directly to terminal as output */
        case GUAC_KUBERNETES_CHANNEL_STDOUT:
        case GUAC_KUBERNETES_CHANNEL_STDERR:
            guac_terminal_write(kubernetes_client->term, buffer, length);
            break;

        /* Ignore data on other channels */
        default:
            guac_client_log(client, GUAC_LOG_DEBUG, "Received %i bytes along "
                    "channel %i.", length, channel);

    }

}

/**
 * Requests that the given data be sent along the given channel to the
 * Kubernetes server when the WebSocket connection is next available for
 * writing. If the WebSocket connection has not been available for writing for
 * long enough that the outbound message buffer is full, the request to send
 * this particular message will be dropped.
 *
 * @param client
 *     The guac_client associated with the Kubernetes connection.
 *
 * @param channel
 *     The Kubernetes channel on which to send the message,
 *     such as GUAC_KUBERNETES_CHANNEL_STDIN.
 *
 * @param data
 *     A buffer containing the data to send.
 *
 * @param length
 *     The number of bytes to send.
 */
static void guac_kubernetes_send_message(guac_client* client,
        int channel, const char* data, int length) {

    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    pthread_mutex_lock(&(kubernetes_client->outbound_message_lock));

    /* Add message to buffer if space is available */
    if (kubernetes_client->outbound_messages_waiting
            < GUAC_KUBERNETES_MAX_OUTBOUND_MESSAGES) {

        /* Calculate storage position of next message */
        int index = (kubernetes_client->outbound_messages_top
                  + kubernetes_client->outbound_messages_waiting)
                  % GUAC_KUBERNETES_MAX_OUTBOUND_MESSAGES;

        /* Obtain pointer to message slot at calculated position */
        guac_kubernetes_message* message =
            &(kubernetes_client->outbound_messages[index]);

        /* Copy details of message into buffer */
        message->channel = channel;
        memcpy(message->data, data, length);
        message->length = length;

        /* One more message is now waiting */
        kubernetes_client->outbound_messages_waiting++;

        /* Notify libwebsockets that we need a callback to send pending
         * messages */
        lws_callback_on_writable(kubernetes_client->wsi);
        lws_cancel_service(kubernetes_client->context);

    }

    /* Warn if data has to be dropped */
    else
        guac_client_log(client, GUAC_LOG_WARNING, "Send buffer could not be "
                "flushed in time to handle additional data. Outbound "
                "message dropped.");

    pthread_mutex_unlock(&(kubernetes_client->outbound_message_lock));

}

/**
 * Writes the oldest pending message within the outbound message queue,
 * as scheduled with guac_kubernetes_send_message(), removing that message
 * from the queue. This function MAY NOT be invoked outside the libwebsockets
 * event callback and MUST only be invoked in the context of a
 * LWS_CALLBACK_CLIENT_WRITEABLE event. If no messages are pending, this
 * function has no effect.
 *
 * @param client
 *     The guac_client associated with the Kubernetes connection.
 *
 * @return
 *     true if messages still remain to be written within the outbound message
 *     queue, false otherwise.
 */
static bool guac_kubernetes_write_pending_message(guac_client* client) {

    bool messages_remain;
    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    pthread_mutex_lock(&(kubernetes_client->outbound_message_lock));

    /* Send one message from top of buffer */
    if (kubernetes_client->outbound_messages_waiting > 0) {

        /* Obtain pointer to message at top */
        int top = kubernetes_client->outbound_messages_top;
        guac_kubernetes_message* message =
            &(kubernetes_client->outbound_messages[top]);

        /* Write message including channel index */
        lws_write(kubernetes_client->wsi,
                ((unsigned char*) message) + LWS_PRE,
                message->length + 1, LWS_WRITE_BINARY);

        /* Advance top to next message */
        kubernetes_client->outbound_messages_top++;
        kubernetes_client->outbound_messages_top %=
            GUAC_KUBERNETES_MAX_OUTBOUND_MESSAGES;

        /* One less message is waiting */
        kubernetes_client->outbound_messages_waiting--;

    }

    /* Record whether messages remained at time of completion */
    messages_remain = (kubernetes_client->outbound_messages_waiting > 0);

    pthread_mutex_unlock(&(kubernetes_client->outbound_message_lock));

    return messages_remain;

}

/**
 * Callback invoked by libwebsockets for events related to a WebSocket being
 * used for communicating with an attached Kubernetes pod.
 *
 * @param wsi
 *     The libwebsockets handle for the WebSocket connection.
 *
 * @param reason
 *     The reason (event) that this callback was invoked.
 *
 * @param user
 *     Arbitrary data assocated with the WebSocket session. This will always
 *     be a pointer to the guac_client instance.
 *
 * @param in
 *     A pointer to arbitrary, reason-specific data.
 *
 * @param length
 *     An arbitrary, reason-specific length value.
 *
 * @return
 *     An undocumented integer value related the success of handling the
 *     event, or -1 if the WebSocket connection should be closed.
 */
static int guac_kubernetes_lws_callback(struct lws* wsi,
        enum lws_callback_reasons reason, void* user,
        void* in, size_t length) {

    /* Request connection closure if client is stopped (note that the user
     * pointer passed by libwebsockets may be NULL for some events) */
    guac_client* client = (guac_client*) user;
    if (client != NULL && client->state != GUAC_CLIENT_RUNNING)
        return lws_callback_http_dummy(wsi, reason, user, in, length);

    switch (reason) {

        /* Failed to connect */
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND,
                    "Error connecting to Kubernetes server: %s",
                    in != NULL ? (char*) in : "(no error description "
                    "available)");
            break;

        /* Connected / logged in */
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            guac_client_log(client, GUAC_LOG_INFO,
                    "Kubernetes connection successful.");

            /* Schedule check for pending messages in case messages were added
             * to the outbound message buffer prior to the connection being
             * fully established */
            lws_callback_on_writable(wsi);
            break;

        /* Data received via WebSocket */
        case LWS_CALLBACK_CLIENT_RECEIVE:
            guac_kubernetes_receive_data(client, (const char*) in, length);
            break;

        /* WebSocket is ready for writing */
        case LWS_CALLBACK_CLIENT_WRITEABLE:

            /* Send any pending messages, requesting another callback if
             * yet more messages remain */
            if (guac_kubernetes_write_pending_message(client))
                lws_callback_on_writable(wsi);
            break;

#ifdef HAVE_LWS_CALLBACK_CLIENT_CLOSED
        /* Connection closed (client-specific) */
        case LWS_CALLBACK_CLIENT_CLOSED:
#endif

        /* Connection closed */
        case LWS_CALLBACK_CLOSED:
            guac_client_stop(client);
            guac_client_log(client, GUAC_LOG_DEBUG, "WebSocket connection to "
                    "Kubernetes server closed.");
            break;

        /* No other event types are applicable */
        default:
            break;

    }

    return lws_callback_http_dummy(wsi, reason, user, in, length);

}

/**
 * List of all WebSocket protocols which should be declared as supported by
 * libwebsockets during the initial WebSocket handshake, along with
 * corresponding event-handling callbacks.
 */
struct lws_protocols guac_kubernetes_lws_protocols[] = {
    {
        .name = GUAC_KUBERNETES_LWS_PROTOCOL,
        .callback = guac_kubernetes_lws_callback
    },
    { 0 }
};

/**
 * Input thread, started by the main Kubernetes client thread. This thread
 * continuously reads from the terminal's STDIN and transfers all read
 * data to the Kubernetes connection.
 *
 * @param data
 *     The current guac_client instance.
 *
 * @return
 *     Always NULL.
 */
static void* guac_kubernetes_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    char buffer[GUAC_KUBERNETES_MAX_MESSAGE_SIZE];
    int bytes_read;

    /* Write all data read */
    while ((bytes_read = guac_terminal_read_stdin(kubernetes_client->term, buffer, sizeof(buffer))) > 0) {

        /* Send received data to Kubernetes along STDIN channel */
        guac_kubernetes_send_message(client, GUAC_KUBERNETES_CHANNEL_STDIN,
                buffer, bytes_read);

    }

    return NULL;

}

void* guac_kubernetes_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    guac_kubernetes_settings* settings = kubernetes_client->settings;

    pthread_t input_thread;
    char endpoint_path[GUAC_KUBERNETES_MAX_ENDPOINT_LENGTH];

    /* Verify that the pod name was specified (it's always required) */
    if (settings->kubernetes_pod == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "The name of the Kubernetes pod is a required parameter.");
        goto fail;
    }

    /* Generate endpoint for attachment URL */
    if (guac_kubernetes_endpoint_attach(endpoint_path, sizeof(endpoint_path),
                settings->kubernetes_namespace,
                settings->kubernetes_pod,
                settings->kubernetes_container)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to generate path for Kubernetes API endpoint: "
                "Resulting path too long");
        goto fail;
    }

    guac_client_log(client, GUAC_LOG_DEBUG, "The endpoint for attaching to "
            "the requested Kubernetes pod is \"%s\".", endpoint_path);

    /* Set up screen recording, if requested */
    if (settings->recording_path != NULL) {
        kubernetes_client->recording = guac_common_recording_create(client,
                settings->recording_path,
                settings->recording_name,
                settings->create_recording_path,
                !settings->recording_exclude_output,
                !settings->recording_exclude_mouse,
                settings->recording_include_keys);
    }

    /* Create terminal */
    kubernetes_client->term = guac_terminal_create(client,
            kubernetes_client->clipboard,
            settings->max_scrollback, settings->font_name, settings->font_size,
            settings->resolution, settings->width, settings->height,
            settings->color_scheme, settings->backspace);

    /* Fail if terminal init failed */
    if (kubernetes_client->term == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Terminal initialization failed");
        goto fail;
    }

    /* Set up typescript, if requested */
    if (settings->typescript_path != NULL) {
        guac_terminal_create_typescript(kubernetes_client->term,
                settings->typescript_path,
                settings->typescript_name,
                settings->create_typescript_path);
    }

    /* Init libwebsockets context creation parameters */
    struct lws_context_creation_info context_info = {
        .port = CONTEXT_PORT_NO_LISTEN, /* We are not a WebSocket server */
        .uid = -1,
        .gid = -1,
        .protocols = guac_kubernetes_lws_protocols,
        .user = client
    };

    /* Init WebSocket connection parameters which do not vary by Guacmaole
     * connection parameters or creation of future libwebsockets objects */
    struct lws_client_connect_info connection_info = {
        .host = settings->hostname,
        .address = settings->hostname,
        .origin = settings->hostname,
        .port = settings->port,
        .protocol = GUAC_KUBERNETES_LWS_PROTOCOL,
        .pwsi = &kubernetes_client->wsi,
        .userdata = client
    };

    /* If requested, use an SSL/TLS connection for communication with
     * Kubernetes */
    if (settings->use_ssl) {

        /* Enable use of SSL/TLS */
        context_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        connection_info.ssl_connection = LCCSCF_USE_SSL;

        /* Bypass certificate checks if requested */
        if (settings->ignore_cert) {
            connection_info.ssl_connection |=
                  LCCSCF_ALLOW_SELFSIGNED
                | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK
                | LCCSCF_ALLOW_EXPIRED;
        }

        /* Otherwise use the given CA certificate to validate (if any) */
        else
            context_info.client_ssl_ca_filepath = settings->ca_cert_file;

        /* Certificate and key file for SSL/TLS client auth */
        context_info.client_ssl_cert_filepath = settings->client_cert_file;
        context_info.client_ssl_private_key_filepath = settings->client_key_file;

    }

    /* Create libwebsockets context */
    kubernetes_client->context = lws_create_context(&context_info);
    if (!kubernetes_client->context) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Initialization of libwebsockets failed");
        goto fail;
    }

    /* Generate path dynamically */
    connection_info.context = kubernetes_client->context;
    connection_info.path = endpoint_path;

    /* Open WebSocket connection to Kubernetes */
    kubernetes_client->wsi = lws_client_connect_via_info(&connection_info);
    if (kubernetes_client->wsi == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Connection via libwebsockets failed");
        goto fail;
    }

    /* Init outbound message buffer */
    pthread_mutex_init(&(kubernetes_client->outbound_message_lock), NULL);

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, guac_kubernetes_input_thread, (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to start input thread");
        goto fail;
    }

    /* Force a redraw of the attached display (there will be no content
     * otherwise, given the stream nature of attaching to a running
     * container) */
    guac_kubernetes_force_redraw(client);

    /* As long as client is connected, continue polling libwebsockets */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Cease polling libwebsockets if an error condition is signalled */
        if (lws_service(kubernetes_client->context,
                    GUAC_KUBERNETES_SERVICE_INTERVAL) < 0)
            break;

    }

    /* Kill client and Wait for input thread to die */
    guac_terminal_stop(kubernetes_client->term);
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

fail:

    /* Kill and free terminal, if allocated */
    if (kubernetes_client->term != NULL)
        guac_terminal_free(kubernetes_client->term);

    /* Clean up recording, if in progress */
    if (kubernetes_client->recording != NULL)
        guac_common_recording_free(kubernetes_client->recording);

    /* Free WebSocket context if successfully allocated */
    if (kubernetes_client->context != NULL)
        lws_context_destroy(kubernetes_client->context);

    guac_client_log(client, GUAC_LOG_INFO, "Kubernetes connection ended.");
    return NULL;

}

void guac_kubernetes_resize(guac_client* client, int rows, int columns) {

    char buffer[64];

    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    /* Send request only if different from last request */
    if (kubernetes_client->rows != rows ||
            kubernetes_client->columns != columns) {

        kubernetes_client->rows = rows;
        kubernetes_client->columns = columns;

        /* Construct terminal resize message for Kubernetes */
        int length = snprintf(buffer, sizeof(buffer),
                "{\"Width\":%i,\"Height\":%i}", columns, rows);

        /* Schedule message for sending */
        guac_kubernetes_send_message(client, GUAC_KUBERNETES_CHANNEL_RESIZE,
                buffer, length);

    }

}

void guac_kubernetes_force_redraw(guac_client* client) {

    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    /* Get current terminal dimensions */
    guac_terminal* term = kubernetes_client->term;
    int rows = term->term_height;
    int columns = term->term_width;

    /* Force a redraw by increasing the terminal size by one character in
     * each dimension and then resizing it back to normal (the same technique
     * used by kubectl */
    guac_kubernetes_resize(client, rows + 1, columns + 1);
    guac_kubernetes_resize(client, rows, columns);

}

