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

#include "argv.h"
#include "chassis.h"
#include "ipmi.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/mem.h>
#include <guacamole/proctitle.h>
#include <guacamole/protocol.h>
#include <guacamole/recording.h>
#include <guacamole/timestamp.h>
#include <ipmiconsole.h>

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Write the entire buffer given to the specified file descriptor, retrying the
 * write automatically if necessary.
 *
 * @param fd
 *     The file descriptor to write to.
 *
 * @param buffer
 *     The buffer to write.
 *
 * @param size
 *     The number of bytes from the buffer to write.
 *
 * @return
 *     The number of bytes written (equal to size) on success, or a value not
 *     equal to size if an error occurs which prevents all future writes.
 */
static int __guac_ipmi_write_all(int fd, const char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        /* Attempt to write data */
        int ret_val;
        GUAC_RETRY_EINTR(ret_val, write(fd, buffer, remaining));
        if (ret_val <= 0)
            return -1;

        /* If successful, continue with what data remains (if any) */
        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

/**
 * Waits for data on the given file descriptor for up to one second. The return
 * value is identical to that of poll(): 0 on timeout, < 0 on error, and > 0 on
 * success.
 *
 * @param console_fd
 *     The file descriptor to wait for.
 *
 * @return
 *     A value greater than zero on success, zero on timeout, and less than
 *     zero on error.
 */
static int __guac_ipmi_wait(int console_fd) {

    /* Build array of file descriptors */
    struct pollfd fds[] = {{
        .fd      = console_fd,
        .events  = POLLIN,
        .revents = 0,
    }};

    int wait_result;

    /* Wait for one second */
    GUAC_RETRY_EINTR(wait_result, poll(fds, 1, 1000));

    return wait_result;

}

/**
 * Input thread, started by the main IPMI client thread. This thread
 * continuously reads from the terminal's STDIN and transfers all read data to
 * the SOL session.
 *
 * @param data
 *     The current guac_client instance.
 *
 * @return
 *     Always NULL.
 */
static void* __guac_ipmi_input_thread(void* data) {

    /* Thread name ipmi-stdin: reads terminal STDIN and forwards it to the SOL
     * session. */
    guac_thread_name_set("ipmi-stdin");

    guac_client* client = (guac_client*) data;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    char buffer[8192];
    int bytes_read;

    /* Forward all data read from the terminal to the serial console */
    while ((bytes_read = guac_terminal_read_stdin(ipmi_client->term, buffer,
                    sizeof(buffer))) > 0) {
        if (__guac_ipmi_write_all(ipmi_client->console_fd, buffer, bytes_read)
                != bytes_read)
            break;
    }

    return NULL;

}

/**
 * Establishes the IPMI 2.0 Serial-over-LAN session described by the given
 * client's settings, blocking until the session has been established or an
 * error occurs. The libipmiconsole engine must already have been initialized.
 *
 * @param client
 *     The guac_client whose settings describe the SOL session to establish.
 *
 * @return
 *     The established libipmiconsole context on success, or NULL on failure
 *     (with the connection aborted).
 */
static ipmiconsole_ctx_t __guac_ipmi_create_session(guac_client* client) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_ipmi_settings* settings = ipmi_client->settings;

    /* IPMI authentication and protocol configuration. Values of < 0 (or NULL)
     * request the libipmiconsole defaults. */
    struct ipmiconsole_ipmi_config ipmi_config = {
        .username         = settings->username,
        .password         = settings->password,
        .k_g              = (unsigned char*) settings->k_g,
        .k_g_len          = settings->k_g != NULL ? strlen(settings->k_g) : 0,
        .privilege_level  = settings->privilege_level,
        .cipher_suite_id  = settings->cipher_suite,
        .workaround_flags = 0
    };

    struct ipmiconsole_protocol_config protocol_config = {
        .session_timeout_len                  =
            settings->timeout > 0 ? settings->timeout * 1000 : -1,
        .retransmission_timeout_len           = -1,
        .retransmission_backoff_count         = -1,
        .keepalive_timeout_len                = -1,
        .retransmission_keepalive_timeout_len = -1,
        .acceptable_packet_errors_count       = -1,
        .maximum_retransmission_count         = -1
    };

    struct ipmiconsole_engine_config engine_config = {
        .engine_flags   = 0,
        .behavior_flags = 0,
        .debug_flags    = 0
    };

    /* Create the SOL context */
    ipmiconsole_ctx_t ctx = ipmiconsole_ctx_create(settings->hostname,
            &ipmi_config, &protocol_config, &engine_config);
    if (ctx == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "IPMI console context allocation failed.");
        return NULL;
    }

    /* Select the requested SOL payload instance */
    unsigned int instance = settings->sol_payload_instance;
    if (ipmiconsole_ctx_set_config(ctx,
                IPMICONSOLE_CTX_CONFIG_OPTION_SOL_PAYLOAD_INSTANCE,
                &instance) < 0)
        guac_client_log(client, GUAC_LOG_WARNING, "Unable to set SOL payload "
                "instance; using the default.");

    /* Establish the SOL session, blocking until it is established or fails.
     * libipmiconsole will, by default, attempt to deactivate any pre-existing
     * SOL session on the BMC ("SOL already active"). */
    if (ipmiconsole_engine_submit_block(ctx) < 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                "Unable to establish IPMI SOL session: %s",
                ipmiconsole_ctx_errormsg(ctx));
        ipmiconsole_ctx_destroy(ctx);
        return NULL;
    }

    return ctx;

}

/**
 * Applies any configured pre-connect boot device override and power action,
 * now that the SOL session has been established. Establishing the console
 * before powering the system on allows the user to observe the system from the
 * earliest stages of boot.
 *
 * @param client
 *     The guac_client whose settings describe the actions to perform.
 */
static void __guac_ipmi_apply_connect_actions(guac_client* client) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_ipmi_settings* settings = ipmi_client->settings;

    /* Apply any boot device override prior to powering the system on */
    if (settings->boot_device != GUAC_IPMI_BOOT_NONE) {
        if (guac_ipmi_chassis_set_boot_device(client, settings->boot_device,
                    false) == 0)
            guac_client_log(client, GUAC_LOG_INFO,
                    "Applied boot device override.");
    }

    /* Perform any requested power action */
    if (settings->power_on_connect != GUAC_IPMI_POWER_NONE) {
        if (guac_ipmi_chassis_power(client, settings->power_on_connect) == 0)
            guac_client_log(client, GUAC_LOG_INFO,
                    "Applied power-on-connect action.");
    }

}

void* guac_ipmi_client_thread(void* data) {

    /* Thread name ipmi-worker: main IPMI client thread; runs the SOL session
     * and reads its output into the terminal. */
    guac_thread_name_set("ipmi-worker");

    guac_client* client = (guac_client*) data;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_ipmi_settings* settings = ipmi_client->settings;

    pthread_t input_thread;
    char buffer[8192];
    int wait_result;

    guac_process_title_set_endpoint(GUAC_IPMI_PROCESS_TITLE_NAME,
            settings->username, settings->hostname, settings->port);

    /* Set up screen recording, if requested */
    if (settings->recording_path != NULL) {
        ipmi_client->recording = guac_recording_create(client,
                settings->recording_path,
                settings->recording_name,
                settings->create_recording_path,
                !settings->recording_exclude_output,
                !settings->recording_exclude_mouse,
                0, /* Touch events not supported */
                settings->recording_include_keys,
                settings->recording_write_existing,
                settings->recording_include_clipboard);
    }

    /* Create terminal options with required parameters */
    guac_terminal_options* options = guac_terminal_options_create(
            settings->width, settings->height, settings->resolution);

    /* Set optional parameters */
    options->disable_copy = settings->disable_copy;
    options->max_scrollback = settings->max_scrollback;
    options->font_name = settings->font_name;
    options->font_size = settings->font_size;
    options->color_scheme = settings->color_scheme;
    options->backspace = settings->backspace;
    options->linux_console_keys = (strcmp(settings->terminal_type, "linux") == 0);

    /* Create terminal */
    ipmi_client->term = guac_terminal_create(client, options);

    /* Free options struct now that it's been used */
    guac_mem_free(options);

    /* Fail if terminal init failed */
    if (ipmi_client->term == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Terminal initialization failed");
        return NULL;
    }

    /* Send current values of exposed arguments to owner only */
    guac_client_for_owner(client, guac_ipmi_send_current_argv, ipmi_client);

    /* Set up typescript, if requested */
    if (settings->typescript_path != NULL) {
        guac_terminal_create_typescript(ipmi_client->term,
                settings->typescript_path,
                settings->typescript_name,
                settings->create_typescript_path,
                settings->typescript_write_existing);
    }

    /* Initialize the libipmiconsole engine */
    if (ipmiconsole_engine_init(0, 0) < 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to initialize IPMI console engine.");
        return NULL;
    }

    /* Establish the SOL session */
    ipmi_client->console_ctx = __guac_ipmi_create_session(client);
    if (ipmi_client->console_ctx == NULL) {
        /* Already aborted within __guac_ipmi_create_session() */
        ipmiconsole_engine_teardown(0);
        return NULL;
    }

    /* Retrieve the file descriptor used to read/write the serial console */
    ipmi_client->console_fd = ipmiconsole_ctx_fd(ipmi_client->console_ctx);
    if (ipmi_client->console_fd < 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                "Unable to retrieve IPMI SOL file descriptor.");
        return NULL;
    }

    guac_client_log(client, GUAC_LOG_INFO, "IPMI SOL session established.");

    /* Allow the terminal to begin rendering immediately, before any
     * (potentially slow) pre-connect chassis actions are applied. SOL output
     * produced during those actions is buffered by the libipmiconsole engine
     * and rendered once the read loop below begins. */
    guac_terminal_start(ipmi_client->term);

    /* Apply any configured pre-connect boot/power actions */
    __guac_ipmi_apply_connect_actions(client);

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, __guac_ipmi_input_thread,
                (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to start input thread");
        return NULL;
    }

    /* While data is available, write SOL output to the terminal */
    while ((wait_result = __guac_ipmi_wait(ipmi_client->console_fd)) >= 0) {

        /* Resume waiting if no data is available */
        if (wait_result == 0)
            continue;

        int bytes_read;
        GUAC_RETRY_EINTR(bytes_read, read(ipmi_client->console_fd, buffer,
                    sizeof(buffer)));
        if (bytes_read <= 0)
            break;

        guac_terminal_write(ipmi_client->term, buffer, bytes_read);

    }

    /* Kill client and wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    guac_client_log(client, GUAC_LOG_INFO, "IPMI connection ended.");
    return NULL;

}
