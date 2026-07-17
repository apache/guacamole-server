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
#include "common/defaults.h"
#include "common/clipboard.h"
#include "settings.h"
#include "terminal/terminal.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/user.h>

#include <sys/types.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Client plugin arguments */
const char* GUAC_SERIAL_CLIENT_ARGS[] = {
    "serial-type",
    "network-protocol",
    "device",
    "hostname",
    "port",
    "timeout",
    "baud-rate",
    "data-bits",
    "stop-bits",
    "parity",
    "flow-control",
    "break-duration",
    "paste-delay",
    "allowed-devices",
    "read-only",
    GUAC_SERIAL_ARGV_FONT_NAME,
    GUAC_SERIAL_ARGV_FONT_SIZE,
    GUAC_SERIAL_ARGV_COLOR_SCHEME,
    "scrollback",
    "backspace",
    "terminal-type",
    "clipboard-buffer-size",
    "disable-copy",
    "disable-paste",
    "typescript-path",
    "typescript-name",
    "create-typescript-path",
    "typescript-write-existing",
    "recording-path",
    "recording-name",
    "recording-exclude-output",
    "recording-exclude-mouse",
    "recording-include-keys",
    "recording-include-clipboard",
    "create-recording-path",
    "recording-write-existing",
    "auto-reconnect",
    "line-ending",
    "local-echo",
    "hangup-on-close",
    "reverse-connect",
    "listen-timeout",
    "bind-address",
    NULL
};

enum SERIAL_ARGS_IDX {

    /**
     * The transport used to reach the serial line ("local" or "network").
     */
    IDX_SERIAL_TYPE,

    /**
     * The network protocol used when the transport type is "network" ("raw" or
     * "rfc2217").
     */
    IDX_NETWORK_PROTOCOL,

    /**
     * The absolute path of the local serial device to open (e.g.
     * "/dev/ttyUSB0"). Required for "local" connections.
     */
    IDX_DEVICE,

    /**
     * The hostname of the network serial server to connect to. Required for
     * "network" connections.
     */
    IDX_HOSTNAME,

    /**
     * The port of the network serial server to connect to. Required for
     * "network" connections.
     */
    IDX_PORT,

    /**
     * The number of seconds to wait for the remote server to respond. Optional.
     */
    IDX_TIMEOUT,

    /**
     * The serial line speed, in bits per second (e.g. 9600, 115200).
     */
    IDX_BAUD_RATE,

    /**
     * The number of data bits per character (5, 6, 7, or 8).
     */
    IDX_DATA_BITS,

    /**
     * The number of stop bits per character (1 or 2).
     */
    IDX_STOP_BITS,

    /**
     * The parity scheme ("none", "odd", "even", "mark", or "space").
     */
    IDX_PARITY,

    /**
     * The flow control scheme ("none", "rts-cts", or "xon-xoff").
     */
    IDX_FLOW_CONTROL,

    /**
     * The duration, in milliseconds, that the transmit line is held low when
     * sending a serial break.
     */
    IDX_BREAK_DURATION,

    /**
     * The delay, in milliseconds, inserted between each byte written to the
     * serial line. Zero disables pacing.
     */
    IDX_PASTE_DELAY,

    /**
     * An optional ":"-separated allowlist of local device paths or path
     * prefixes. If set, a "local" connection whose canonicalized device does
     * not equal or fall under one of the entries is rejected.
     */
    IDX_ALLOWED_DEVICES,

    /**
     * "true" if this connection should be read-only (user input should be
     * dropped), "false" or blank otherwise.
     */
    IDX_READ_ONLY,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

    /**
     * The color scheme to use, as a series of semicolon-separated color-value
     * pairs, or one of the special color scheme names.
     */
    IDX_COLOR_SCHEME,

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    IDX_SCROLLBACK,

    /**
     * ASCII code, as an integer to use for the backspace key, or
     * GUAC_TERMINAL_DEFAULT_BACKSPACE if not specified.
     */
    IDX_BACKSPACE,

    /**
     * The terminal emulator type that is used for rendering (e.g. "xterm" or
     * "xterm-256color"). "linux" is used if unspecified.
     */
    IDX_TERMINAL_TYPE,

    /**
     * The maximum number of bytes to allow within the clipboard.
     */
    IDX_CLIPBOARD_BUFFER_SIZE,

    /**
     * Whether outbound clipboard access should be blocked.
     */
    IDX_DISABLE_COPY,

    /**
     * Whether inbound clipboard access should be blocked.
     */
    IDX_DISABLE_PASTE,

    /**
     * The full absolute path to the directory in which typescripts should be
     * written.
     */
    IDX_TYPESCRIPT_PATH,

    /**
     * The name that should be given to typescripts which are written in the
     * given path.
     */
    IDX_TYPESCRIPT_NAME,

    /**
     * Whether the specified typescript path should automatically be created if
     * it does not yet exist.
     */
    IDX_CREATE_TYPESCRIPT_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * typescript. Disabled by default.
     */
    IDX_TYPESCRIPT_WRITE_EXISTING,

    /**
     * The full absolute path to the directory in which screen recordings
     * should be written.
     */
    IDX_RECORDING_PATH,

    /**
     * The name that should be given to screen recordings which are written in
     * the given path.
     */
    IDX_RECORDING_NAME,

    /**
     * Whether output which is broadcast to each connected client should NOT be
     * included in the session recording.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether changes to mouse state should NOT be included in the session
     * recording.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the session
     * recording.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether clipboard data should be included in the session recording.
     */
    IDX_RECORDING_INCLUDE_CLIPBOARD,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * recording. Disabled by default.
     */
    IDX_RECORDING_WRITE_EXISTING,

    /**
     * Whether the connection should automatically reconnect if the serial line
     * is dropped. Defaults to true.
     */
    IDX_AUTO_RECONNECT,

    /**
     * The line ending written to the serial line for the operator's outgoing
     * newlines ("cr", "lf", or "crlf"). Defaults to "cr".
     */
    IDX_LINE_ENDING,

    /**
     * Whether user input should be echoed to the terminal locally. Defaults to
     * false.
     */
    IDX_LOCAL_ECHO,

    /**
     * Whether the modem control lines should be lowered (HUPCL) when the local
     * device is closed. Defaults to false.
     */
    IDX_HANGUP_ON_CLOSE,

    /**
     * Whether guacd should listen for an inbound connection from the device
     * (reverse mode) instead of dialing out. Defaults to false.
     */
    IDX_REVERSE_CONNECT,

    /**
     * The number of seconds to wait for an inbound connection in reverse mode
     * before failing. Zero or negative means wait indefinitely.
     */
    IDX_LISTEN_TIMEOUT,

    /**
     * The local address to bind when listening in reverse mode. Defaults to
     * "127.0.0.1".
     */
    IDX_BIND_ADDRESS,

    SERIAL_ARGS_COUNT
};

speed_t guac_serial_baud_to_speed(int baud_rate) {

    switch (baud_rate) {
        case 300:    return B300;
        case 1200:   return B1200;
        case 2400:   return B2400;
        case 4800:   return B4800;
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        default:     return (speed_t) -1;
    }

}

/**
 * Parses the given transport type string into a guac_serial_type value.
 *
 * @param str
 *     The transport type string ("local" or "network").
 *
 * @param type
 *     A pointer to the guac_serial_type value that should receive the parsed
 *     result.
 *
 * @return
 *     Zero if the string was successfully parsed, non-zero otherwise.
 */
static int guac_serial_parse_type(const char* str, guac_serial_type* type) {
    if (strcmp(str, "local") == 0)   { *type = GUAC_SERIAL_TYPE_LOCAL;   return 0; }
    if (strcmp(str, "network") == 0) { *type = GUAC_SERIAL_TYPE_NETWORK; return 0; }
    return 1;
}

/**
 * Parses the given network protocol string into a
 * guac_serial_network_protocol value.
 *
 * @param str
 *     The network protocol string ("raw" or "rfc2217").
 *
 * @param protocol
 *     A pointer to the value that should receive the parsed result.
 *
 * @return
 *     Zero if the string was successfully parsed, non-zero otherwise.
 */
static int guac_serial_parse_network_protocol(const char* str,
        guac_serial_network_protocol* protocol) {
    if (strcmp(str, "raw") == 0)     { *protocol = GUAC_SERIAL_NETWORK_PROTOCOL_RAW;     return 0; }
    if (strcmp(str, "rfc2217") == 0) { *protocol = GUAC_SERIAL_NETWORK_PROTOCOL_RFC2217; return 0; }
    return 1;
}

/**
 * Parses the given parity string into a guac_serial_parity value.
 *
 * @param str
 *     The parity string ("none", "odd", "even", "mark", or "space").
 *
 * @param parity
 *     A pointer to the value that should receive the parsed result.
 *
 * @return
 *     Zero if the string was successfully parsed, non-zero otherwise.
 */
static int guac_serial_parse_parity(const char* str, guac_serial_parity* parity) {
    if (strcmp(str, "none") == 0)  { *parity = GUAC_SERIAL_PARITY_NONE;  return 0; }
    if (strcmp(str, "odd") == 0)   { *parity = GUAC_SERIAL_PARITY_ODD;   return 0; }
    if (strcmp(str, "even") == 0)  { *parity = GUAC_SERIAL_PARITY_EVEN;  return 0; }
    if (strcmp(str, "mark") == 0)  { *parity = GUAC_SERIAL_PARITY_MARK;  return 0; }
    if (strcmp(str, "space") == 0) { *parity = GUAC_SERIAL_PARITY_SPACE; return 0; }
    return 1;
}

/**
 * Parses the given flow control string into a guac_serial_flow_control value.
 *
 * @param str
 *     The flow control string ("none", "rts-cts", or "xon-xoff").
 *
 * @param flow
 *     A pointer to the value that should receive the parsed result.
 *
 * @return
 *     Zero if the string was successfully parsed, non-zero otherwise.
 */
static int guac_serial_parse_flow(const char* str, guac_serial_flow_control* flow) {
    if (strcmp(str, "none") == 0)     { *flow = GUAC_SERIAL_FLOW_NONE;     return 0; }
    if (strcmp(str, "rts-cts") == 0)  { *flow = GUAC_SERIAL_FLOW_RTS_CTS;  return 0; }
    if (strcmp(str, "xon-xoff") == 0) { *flow = GUAC_SERIAL_FLOW_XON_XOFF; return 0; }
    return 1;
}

/**
 * Parses the given line-ending string into a guac_serial_line_ending value.
 *
 * @param str
 *     The line-ending string ("cr", "lf", or "crlf").
 *
 * @param line_ending
 *     A pointer to the value that should receive the parsed result.
 *
 * @return
 *     Zero if the string was successfully parsed, non-zero otherwise.
 */
static int guac_serial_parse_line_ending(const char* str,
        guac_serial_line_ending* line_ending) {
    if (strcmp(str, "cr") == 0)   { *line_ending = GUAC_SERIAL_LINE_ENDING_CR;   return 0; }
    if (strcmp(str, "lf") == 0)   { *line_ending = GUAC_SERIAL_LINE_ENDING_LF;   return 0; }
    if (strcmp(str, "crlf") == 0) { *line_ending = GUAC_SERIAL_LINE_ENDING_CRLF; return 0; }
    return 1;
}

bool guac_serial_device_permitted(const char* device, const char* allowed) {

    char real_device[PATH_MAX];
    if (realpath(device, real_device) == NULL)
        return false;

    /* Duplicate the list so it may be tokenized in place */
    char* list = guac_strdup(allowed);
    char* saveptr = NULL;
    bool permitted = false;

    for (char* token = strtok_r(list, ":", &saveptr); token != NULL;
            token = strtok_r(NULL, ":", &saveptr)) {

        /* Skip empty entries */
        if (token[0] == '\0')
            continue;

        /* Canonicalize the allowlist entry; skip entries that do not resolve */
        char real_entry[PATH_MAX];
        if (realpath(token, real_entry) == NULL)
            continue;

        size_t entry_len = strlen(real_entry);

        /* Permit an exact match */
        if (strcmp(real_device, real_entry) == 0) {
            permitted = true;
            break;
        }

        /* Permit a device that falls beneath a permitted prefix */
        if (strncmp(real_device, real_entry, entry_len) == 0
                && real_device[entry_len] == '/') {
            permitted = true;
            break;
        }

    }

    guac_mem_free(list);
    return permitted;

}

guac_serial_settings* guac_serial_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != SERIAL_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                SERIAL_ARGS_COUNT, argc);
        return NULL;
    }

    guac_serial_settings* settings = guac_mem_zalloc(sizeof(guac_serial_settings));

    /* Parse transport type */
    char* type_str = guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS,
            argv, IDX_SERIAL_TYPE, "local");
    if (guac_serial_parse_type(type_str, &settings->type)) {
        guac_user_log(user, GUAC_LOG_ERROR, "Invalid serial-type \"%s\": must "
                "be \"local\" or \"network\".", type_str);
        guac_mem_free(type_str);
        guac_mem_free(settings);
        return NULL;
    }
    guac_mem_free(type_str);

    /* Parse network protocol */
    char* proto_str = guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS,
            argv, IDX_NETWORK_PROTOCOL, "raw");
    if (guac_serial_parse_network_protocol(proto_str, &settings->network_protocol)) {
        guac_user_log(user, GUAC_LOG_ERROR, "Invalid network-protocol \"%s\": "
                "must be \"raw\" or \"rfc2217\".", proto_str);
        guac_mem_free(proto_str);
        guac_mem_free(settings);
        return NULL;
    }
    guac_mem_free(proto_str);

    /* Parse local device path */
    settings->device =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_DEVICE, NULL);

    /* Parse network connection information */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_HOSTNAME, NULL);

    settings->port =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_PORT, NULL);

    settings->timeout =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_TIMEOUT, GUAC_SERIAL_DEFAULT_TIMEOUT);

    /* Parse reverse (listen) mode flag */
    settings->reverse_connect =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_REVERSE_CONNECT, false);

    /* Parse listen timeout, treating negative values as "wait indefinitely" */
    settings->listen_timeout =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_LISTEN_TIMEOUT, GUAC_SERIAL_DEFAULT_LISTEN_TIMEOUT);
    if (settings->listen_timeout < 0)
        settings->listen_timeout = 0;

    /* Parse local bind address used when listening in reverse mode */
    settings->bind_address =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_BIND_ADDRESS, "127.0.0.1");

    /* Reverse mode is meaningless for local connections */
    if (settings->reverse_connect && settings->type == GUAC_SERIAL_TYPE_LOCAL) {
        guac_user_log(user, GUAC_LOG_WARNING, "reverse-connect has no effect on "
                "local serial connections; ignoring.");
        settings->reverse_connect = false;
    }

    /* Validate transport-specific requirements */
    if (settings->type == GUAC_SERIAL_TYPE_LOCAL) {
        if (settings->device == NULL || settings->device[0] == '\0') {
            guac_user_log(user, GUAC_LOG_ERROR, "A \"device\" is required for "
                    "local serial connections.");
            guac_serial_settings_free(settings);
            return NULL;
        }
    }
    else {
        /* In reverse mode guacd binds and listens rather than dialing out, so
         * a hostname is only required when dialing out */
        if (!settings->reverse_connect
                && (settings->hostname == NULL || settings->hostname[0] == '\0')) {
            guac_user_log(user, GUAC_LOG_ERROR, "A \"hostname\" is required for "
                    "network serial connections.");
            guac_serial_settings_free(settings);
            return NULL;
        }
        if (settings->port == NULL || settings->port[0] == '\0') {
            guac_user_log(user, GUAC_LOG_ERROR, "A \"port\" is required for "
                    "network serial connections.");
            guac_serial_settings_free(settings);
            return NULL;
        }
    }

#ifndef ENABLE_SERIAL_RFC2217
    /* Reject RFC2217 if libtelnet support was not compiled in */
    if (settings->type == GUAC_SERIAL_TYPE_NETWORK
            && settings->network_protocol == GUAC_SERIAL_NETWORK_PROTOCOL_RFC2217) {
        guac_user_log(user, GUAC_LOG_ERROR, "RFC2217 transport requested but "
                "not compiled in; rebuild with libtelnet, or use "
                "network-protocol=raw.");
        guac_serial_settings_free(settings);
        return NULL;
    }
#endif

    /* Parse serial line speed */
    settings->baud_rate =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_BAUD_RATE, 9600);
    if (guac_serial_baud_to_speed(settings->baud_rate) == (speed_t) -1) {
        guac_user_log(user, GUAC_LOG_ERROR, "Unsupported baud-rate: %i.",
                settings->baud_rate);
        guac_serial_settings_free(settings);
        return NULL;
    }

    /* Parse data bits */
    settings->data_bits =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_DATA_BITS, 8);
    if (settings->data_bits < 5 || settings->data_bits > 8) {
        guac_user_log(user, GUAC_LOG_ERROR, "Invalid data-bits: %i (must be "
                "5, 6, 7, or 8).", settings->data_bits);
        guac_serial_settings_free(settings);
        return NULL;
    }

    /* Parse stop bits */
    settings->stop_bits =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_STOP_BITS, 1);
    if (settings->stop_bits != 1 && settings->stop_bits != 2) {
        guac_user_log(user, GUAC_LOG_ERROR, "Invalid stop-bits: %i (must be "
                "1 or 2).", settings->stop_bits);
        guac_serial_settings_free(settings);
        return NULL;
    }

    /* Parse parity */
    char* parity_str = guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS,
            argv, IDX_PARITY, "none");
    if (guac_serial_parse_parity(parity_str, &settings->parity)) {
        guac_user_log(user, GUAC_LOG_ERROR, "Invalid parity \"%s\": must be "
                "\"none\", \"odd\", \"even\", \"mark\", or \"space\".",
                parity_str);
        guac_mem_free(parity_str);
        guac_serial_settings_free(settings);
        return NULL;
    }
    guac_mem_free(parity_str);

    /* Parse flow control */
    char* flow_str = guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS,
            argv, IDX_FLOW_CONTROL, "none");
    if (guac_serial_parse_flow(flow_str, &settings->flow_control)) {
        guac_user_log(user, GUAC_LOG_ERROR, "Invalid flow-control \"%s\": must "
                "be \"none\", \"rts-cts\", or \"xon-xoff\".", flow_str);
        guac_mem_free(flow_str);
        guac_serial_settings_free(settings);
        return NULL;
    }
    guac_mem_free(flow_str);

    /* Parse break duration */
    settings->break_duration =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_BREAK_DURATION, GUAC_SERIAL_DEFAULT_BREAK_DURATION);
    if (settings->break_duration < 0)
        settings->break_duration = GUAC_SERIAL_DEFAULT_BREAK_DURATION;

    /* Parse paste delay */
    settings->paste_delay =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_PASTE_DELAY, 0);
    if (settings->paste_delay < 0)
        settings->paste_delay = 0;

    /* Parse auto-reconnect flag */
    settings->auto_reconnect =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_AUTO_RECONNECT, true);

    /* Parse line ending */
    char* line_ending_str = guac_user_parse_args_string(user,
            GUAC_SERIAL_CLIENT_ARGS, argv, IDX_LINE_ENDING, "cr");
    if (guac_serial_parse_line_ending(line_ending_str, &settings->line_ending)) {
        guac_user_log(user, GUAC_LOG_ERROR, "Invalid line-ending \"%s\": must "
                "be \"cr\", \"lf\", or \"crlf\".", line_ending_str);
        guac_mem_free(line_ending_str);
        guac_serial_settings_free(settings);
        return NULL;
    }
    guac_mem_free(line_ending_str);

    /* Parse local echo flag */
    settings->local_echo =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_LOCAL_ECHO, false);

    /* Parse hangup-on-close flag */
    settings->hangup_on_close =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_HANGUP_ON_CLOSE, false);

    /* Retain the device allowlist so it can be re-checked on every (re)open */
    settings->allowed_devices =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_ALLOWED_DEVICES, NULL);

    /* Enforce local device allowlist at parse time, if configured */
    if (settings->type == GUAC_SERIAL_TYPE_LOCAL
            && settings->allowed_devices != NULL
            && settings->allowed_devices[0] != '\0') {
        if (!guac_serial_device_permitted(settings->device,
                settings->allowed_devices)) {
            guac_user_log(user, GUAC_LOG_ERROR, "Serial device \"%s\" is not "
                    "permitted by the configured allowed-devices list, or "
                    "could not be resolved.", settings->device);
            guac_serial_settings_free(settings);
            return NULL;
        }
    }

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Read maximum scrollback size */
    settings->max_scrollback =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_SCROLLBACK, GUAC_TERMINAL_DEFAULT_MAX_SCROLLBACK);

    /* Read font name */
    settings->font_name =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_FONT_NAME, GUAC_TERMINAL_DEFAULT_FONT_NAME);

    /* Read font size */
    settings->font_size =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_FONT_SIZE, GUAC_TERMINAL_DEFAULT_FONT_SIZE);

    /* Copy requested color scheme */
    settings->color_scheme =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_COLOR_SCHEME, GUAC_TERMINAL_DEFAULT_COLOR_SCHEME);

    /* Pull width/height/resolution directly from user */
    settings->width      = user->info.optimal_width;
    settings->height     = user->info.optimal_height;
    settings->resolution = user->info.optimal_resolution;

    /* Parse backspace key code */
    settings->backspace =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_BACKSPACE, GUAC_TERMINAL_DEFAULT_BACKSPACE);

    /* Read terminal emulator type. */
    settings->terminal_type =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_TERMINAL_TYPE, "linux");

    /* Set the maximum number of bytes to allow within the clipboard. */
    settings->clipboard_buffer_size =
        guac_user_parse_args_int(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_CLIPBOARD_BUFFER_SIZE, 0);

    /* Use default clipboard buffer size if given one is invalid. */
    if (settings->clipboard_buffer_size < GUAC_COMMON_CLIPBOARD_MIN_LENGTH) {
        settings->clipboard_buffer_size = GUAC_COMMON_CLIPBOARD_MIN_LENGTH;
        guac_user_log(user, GUAC_LOG_INFO, "Unspecified or invalid clipboard buffer "
                "size: \"%s\". Using the default minimum size: %i.",
                argv[IDX_CLIPBOARD_BUFFER_SIZE],
                settings->clipboard_buffer_size);
    }
    else if (settings->clipboard_buffer_size > GUAC_COMMON_CLIPBOARD_MAX_LENGTH) {
        settings->clipboard_buffer_size = GUAC_COMMON_CLIPBOARD_MAX_LENGTH;
        guac_user_log(user, GUAC_LOG_WARNING, "Invalid clipboard buffer "
                "size: \"%s\". Using the default maximum size: %i.",
                argv[IDX_CLIPBOARD_BUFFER_SIZE],
                settings->clipboard_buffer_size);
    }

    /* Parse clipboard copy disable flag */
    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, false);

    /* Parse clipboard paste disable flag */
    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, false);

    /* Read typescript path */
    settings->typescript_path =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_PATH, NULL);

    /* Read typescript name */
    settings->typescript_name =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_NAME, GUAC_SERIAL_DEFAULT_TYPESCRIPT_NAME);

    /* Parse path creation flag */
    settings->create_typescript_path =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_CREATE_TYPESCRIPT_PATH, false);

    /* Parse allow write existing file flag */
    settings->typescript_write_existing =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_WRITE_EXISTING, false);

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_SERIAL_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse clipboard inclusion flag */
    settings->recording_include_clipboard =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_CLIPBOARD, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    /* Parse allow write existing file flag */
    settings->recording_write_existing =
        guac_user_parse_args_boolean(user, GUAC_SERIAL_CLIENT_ARGS, argv,
                IDX_RECORDING_WRITE_EXISTING, false);

    /* Parsing was successful */
    return settings;

}

void guac_serial_settings_free(guac_serial_settings* settings) {

    /* Free transport connection information */
    guac_mem_free(settings->device);
    guac_mem_free(settings->allowed_devices);
    guac_mem_free(settings->hostname);
    guac_mem_free(settings->port);
    guac_mem_free(settings->bind_address);

    /* Free display preferences */
    guac_mem_free(settings->font_name);
    guac_mem_free(settings->color_scheme);

    /* Free typescript settings */
    guac_mem_free(settings->typescript_name);
    guac_mem_free(settings->typescript_path);

    /* Free screen recording settings */
    guac_mem_free(settings->recording_name);
    guac_mem_free(settings->recording_path);

    /* Free terminal emulator type. */
    guac_mem_free(settings->terminal_type);

    /* Free overall structure */
    guac_mem_free(settings);

}
