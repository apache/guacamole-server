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
        return -1;

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
            break;

        /* Data received via WebSocket */
        case LWS_CALLBACK_CLIENT_RECEIVE:
            guac_kubernetes_receive_data(client, (const char*) in, length);
            break;

        /* TODO: Only send data here. Request callback for writing via lws_callback_on_writable(some struct lws*) */
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            break;

        /* TODO: Add configure test */
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

    char buffer[8192];
    int bytes_read;

    /* Write all data read */
    while ((bytes_read = guac_terminal_read_stdin(kubernetes_client->term, buffer, sizeof(buffer))) > 0) {

        /* TODO: Send to Kubernetes */
        guac_terminal_write(kubernetes_client->term, buffer, bytes_read);

    }

    return NULL;

}

void* guac_kubernetes_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    guac_kubernetes_settings* settings = kubernetes_client->settings;

    pthread_t input_thread;

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
        return NULL;
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
    struct lws_context* context = lws_create_context(&context_info);
    if (!context) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Initialization of libwebsockets failed");
        return NULL;
    }

    /* FIXME: Generate path dynamically */
    connection_info.context = context;
    connection_info.path = "/api/v1/namespaces/default/pods/my-shell-68974bb7f7-rpjgr/attach?container=my-shell&stdin=true&stdout=true&tty=true";

    /* Open WebSocket connection to Kubernetes */
    kubernetes_client->wsi = lws_client_connect_via_info(&connection_info);
    if (kubernetes_client->wsi == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Connection via libwebsockets failed");
        return NULL;
    }

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, guac_kubernetes_input_thread, (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to start input thread");
        return NULL;
    }

    /* As long as client is connected, continue polling libwebsockets */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Cease polling libwebsockets if an error condition is signalled */
        if (lws_service(context, 1000) < 0)
            break;

    }

    /* Kill client and Wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    /* All done with libwebsockets */
    lws_context_destroy(context);

    guac_client_log(client, GUAC_LOG_INFO, "Kubernetes connection ended.");
    return NULL;

}

