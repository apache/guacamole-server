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

#ifndef GUAC_SERIAL_SETTINGS_H
#define GUAC_SERIAL_SETTINGS_H

#include <guacamole/user.h>

#include <termios.h>
#include <sys/types.h>
#include <stdbool.h>

/**
 * The port to connect to when initiating any network serial connection, if no
 * other port is specified.
 */
#define GUAC_SERIAL_DEFAULT_PORT "2000"

/**
 * The protocol label included in the process title (the first argument passed
 * to guac_process_title_set_endpoint()), as seen in `ps`/`top`.
 */
#define GUAC_SERIAL_PROCESS_TITLE_NAME "serial"

/**
 * The default number of seconds to wait for a successful connection before
 * timing out.
 */
#define GUAC_SERIAL_DEFAULT_TIMEOUT 10

/**
 * The default duration, in milliseconds, that the transmit line is held low
 * when sending a serial break.
 */
#define GUAC_SERIAL_DEFAULT_BREAK_DURATION 500

/**
 * The default number of seconds to wait for an inbound connection in reverse
 * mode before failing.
 */
#define GUAC_SERIAL_DEFAULT_LISTEN_TIMEOUT 60

/**
 * The interval, in seconds, between automatic reconnection attempts after the
 * serial connection is dropped.
 */
#define GUAC_SERIAL_RECONNECT_INTERVAL 2

/**
 * The filename to use for the typescript, if not specified.
 */
#define GUAC_SERIAL_DEFAULT_TYPESCRIPT_NAME "typescript"

/**
 * The filename to use for the screen recording, if not specified.
 */
#define GUAC_SERIAL_DEFAULT_RECORDING_NAME "recording"

/**
 * The transport used to reach the serial line.
 */
typedef enum guac_serial_type {

    /**
     * A local serial device accessed directly via a device node (e.g.
     * "/dev/ttyUSB0").
     */
    GUAC_SERIAL_TYPE_LOCAL,

    /**
     * A remote serial line reached over the network (raw TCP or RFC2217).
     */
    GUAC_SERIAL_TYPE_NETWORK

} guac_serial_type;

/**
 * The network protocol used when the transport type is
 * GUAC_SERIAL_TYPE_NETWORK.
 */
typedef enum guac_serial_network_protocol {

    /**
     * A raw byte stream, as exposed by ser2net in "raw" mode. Serial line
     * parameters cannot be negotiated and are purely informational.
     */
    GUAC_SERIAL_NETWORK_PROTOCOL_RAW,

    /**
     * The RFC2217 (telnet COM-PORT-OPTION) protocol, allowing serial line
     * parameters and break signalling to be negotiated with the remote end.
     */
    GUAC_SERIAL_NETWORK_PROTOCOL_RFC2217

} guac_serial_network_protocol;

/**
 * The parity scheme applied to the serial line.
 */
typedef enum guac_serial_parity {
    GUAC_SERIAL_PARITY_NONE,
    GUAC_SERIAL_PARITY_ODD,
    GUAC_SERIAL_PARITY_EVEN,
    GUAC_SERIAL_PARITY_MARK,
    GUAC_SERIAL_PARITY_SPACE
} guac_serial_parity;

/**
 * The flow control scheme applied to the serial line.
 */
typedef enum guac_serial_flow_control {
    GUAC_SERIAL_FLOW_NONE,
    GUAC_SERIAL_FLOW_RTS_CTS,
    GUAC_SERIAL_FLOW_XON_XOFF
} guac_serial_flow_control;

/**
 * The line ending written to the serial line in place of the operator's
 * outgoing carriage returns/newlines.
 */
typedef enum guac_serial_line_ending {

    /**
     * A single carriage return ("\r"). This is what most network device
     * consoles (e.g. Cisco) expect, and is the default.
     */
    GUAC_SERIAL_LINE_ENDING_CR,

    /**
     * A single line feed ("\n").
     */
    GUAC_SERIAL_LINE_ENDING_LF,

    /**
     * A carriage return followed by a line feed ("\r\n").
     */
    GUAC_SERIAL_LINE_ENDING_CRLF

} guac_serial_line_ending;

/**
 * Settings for the serial connection. The values for this structure are parsed
 * from the arguments given during the Guacamole protocol handshake using the
 * guac_serial_parse_args() function.
 */
typedef struct guac_serial_settings {

    /**
     * The transport used to reach the serial line.
     */
    guac_serial_type type;

    /**
     * The network protocol used when type is GUAC_SERIAL_TYPE_NETWORK.
     */
    guac_serial_network_protocol network_protocol;

    /**
     * The absolute path of the local serial device to open when type is
     * GUAC_SERIAL_TYPE_LOCAL, e.g. "/dev/ttyUSB0". NULL if not a local
     * connection.
     */
    char* device;

    /**
     * An optional ":"-separated allowlist of permitted local device paths or
     * path prefixes, or NULL if no allowlist was configured. Retained so the
     * device can be re-validated on every (re)open, guarding against a symlink
     * being repointed at a disallowed device between reconnects.
     */
    char* allowed_devices;

    /**
     * The hostname of the network serial server to connect to when type is
     * GUAC_SERIAL_TYPE_NETWORK. NULL if not a network connection.
     */
    char* hostname;

    /**
     * The port of the network serial server to connect to when type is
     * GUAC_SERIAL_TYPE_NETWORK.
     */
    char* port;

    /**
     * Whether guacd should listen for an inbound connection from the device
     * (reverse mode) instead of dialing out. Only meaningful for network type.
     */
    bool reverse_connect;

    /**
     * Seconds to wait for an inbound connection in reverse mode before failing.
     * Zero or negative means wait indefinitely (bounded by client shutdown).
     */
    int listen_timeout;

    /**
     * Local address to bind when listening in reverse mode. NULL/empty =>
     * 127.0.0.1.
     */
    char* bind_address;

    /**
     * The number of seconds to wait for a connection before timing out.
     */
    int timeout;

    /**
     * The serial line speed in bits per second (e.g. 9600, 115200).
     */
    int baud_rate;

    /**
     * The number of data bits per character (5, 6, 7, or 8).
     */
    int data_bits;

    /**
     * The number of stop bits per character (1 or 2).
     */
    int stop_bits;

    /**
     * The parity scheme applied to the serial line.
     */
    guac_serial_parity parity;

    /**
     * The flow control scheme applied to the serial line.
     */
    guac_serial_flow_control flow_control;

    /**
     * The duration, in milliseconds, that the transmit line is held low when
     * sending a serial break.
     */
    int break_duration;

    /**
     * The delay, in milliseconds, inserted between each byte written to the
     * serial line. Zero disables pacing entirely. Used to avoid FIFO overrun
     * when pasting to a low-baud line with no flow control.
     */
    int paste_delay;

    /**
     * Whether the connection should automatically attempt to reconnect if the
     * serial line is dropped, keeping the terminal and its scrollback intact.
     */
    bool auto_reconnect;

    /**
     * The line ending written to the serial line in place of the operator's
     * outgoing carriage returns/newlines.
     */
    guac_serial_line_ending line_ending;

    /**
     * Whether user input should be echoed to the terminal locally. Useful for
     * devices or bootloaders which do not echo typed characters themselves.
     */
    bool local_echo;

    /**
     * Whether the modem control lines should be lowered (HUPCL) when the local
     * device is closed, which resets many attached devices.
     */
    bool hangup_on_close;

    /**
     * Whether this connection is read-only, and user input should be dropped.
     */
    bool read_only;

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    int max_scrollback;

    /**
     * The name of the font to use for display rendering.
     */
    char* font_name;

    /**
     * The size of the font to use, in points.
     */
    int font_size;

    /**
     * The name of the color scheme to use.
     */
    char* color_scheme;

    /**
     * The desired width of the terminal display, in pixels.
     */
    int width;

    /**
     * The desired height of the terminal display, in pixels.
     */
    int height;

    /**
     * The desired screen resolution, in DPI.
     */
    int resolution;

    /**
     * The maximum number of bytes to allow within the clipboard.
     */
    int clipboard_buffer_size;

    /**
     * Whether outbound clipboard access should be blocked. If set, it will not
     * be possible to copy data from the terminal to the client using the
     * clipboard.
     */
    bool disable_copy;

    /**
     * Whether inbound clipboard access should be blocked. If set, it will not
     * be possible to paste data from the client to the terminal using the
     * clipboard.
     */
    bool disable_paste;

    /**
     * The path in which the typescript should be saved, if enabled. If no
     * typescript should be saved, this will be NULL.
     */
    char* typescript_path;

    /**
     * The filename to use for the typescript, if enabled.
     */
    char* typescript_name;

    /**
     * Whether the typescript path should be automatically created if it does
     * not already exist.
     */
    bool create_typescript_path;

    /**
     * Whether existing files should be appended to when creating a new
     * typescript. Disabled by default.
     */
    bool typescript_write_existing;

    /**
     * The path in which the screen recording should be saved, if enabled. If
     * no screen recording should be saved, this will be NULL.
     */
    char* recording_path;

    /**
     * The filename to use for the screen recording, if enabled.
     */
    char* recording_name;

    /**
     * Whether the screen recording path should be automatically created if it
     * does not already exist.
     */
    bool create_recording_path;

    /**
     * Whether output which is broadcast to each connected client (graphics,
     * streams, etc.) should NOT be included in the session recording. Output
     * is included by default, as it is necessary for any recording which must
     * later be viewable as video.
     */
    bool recording_exclude_output;

    /**
     * Whether changes to mouse state, such as position and buttons pressed or
     * released, should NOT be included in the session recording. Mouse state
     * is included by default, as it is necessary for the mouse cursor to be
     * rendered in any resulting video.
     */
    bool recording_exclude_mouse;

    /**
     * Whether keys pressed and released should be included in the session
     * recording. Key events are NOT included by default within the recording,
     * as doing so has privacy and security implications.  Including key events
     * may be necessary in certain auditing contexts, but should only be done
     * with caution. Key events can easily contain sensitive information, such
     * as passwords, credit card numbers, etc.
     */
    bool recording_include_keys;

    /**
     * Whether clipboard data should be included in the session recording.
     * Clipboard data is NOT included by default within the recording,
     * as doing so has privacy and security implications. Including clipboard
     * data may be necessary in certain auditing contexts, but should only be
     * done with caution. Clipboard data can easily contain sensitive
     * information, such as passwords, credit card numbers, etc.
     */
    bool recording_include_clipboard;

    /**
     * Whether existing files should be appended to when creating a new
     * recording. Disabled by default.
     */
    bool recording_write_existing;

    /**
     * The ASCII code, as an integer, that the terminal will use when the
     * backspace key is pressed.  By default, this is 127, ASCII delete, if
     * not specified in the client settings.
     */
    int backspace;

    /**
     * The terminal emulator type that is used for rendering.
     */
    char* terminal_type;

} guac_serial_settings;

/**
 * Parses all given args, storing them in a newly-allocated settings object. If
 * the args fail to parse, NULL is returned.
 *
 * @param user
 *     The user who submitted the given arguments while joining the
 *     connection.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @return
 *     A newly-allocated settings object which must be freed with
 *     guac_serial_settings_free() when no longer needed. If the arguments fail
 *     to parse, NULL is returned.
 */
guac_serial_settings* guac_serial_parse_args(guac_user* user,
        int argc, const char** argv);

/**
 * Maps the given serial line speed, in bits per second, to the corresponding
 * termios speed_t constant (e.g. 9600 -> B9600).
 *
 * @param baud_rate
 *     The serial line speed, in bits per second.
 *
 * @return
 *     The termios speed_t constant corresponding to the given speed, or
 *     (speed_t) -1 if the given speed is not supported.
 */
speed_t guac_serial_baud_to_speed(int baud_rate);

/**
 * Returns whether the given local device is permitted by the given ":"-
 * separated allowlist of device paths or path prefixes. Both the device and
 * each allowlist entry are canonicalized with realpath() before comparison; a
 * device is permitted if its canonical path equals a canonicalized entry or
 * falls beneath one of the canonicalized entries. If the device cannot be
 * resolved (e.g. it does not currently exist), it is not permitted.
 *
 * This performs no logging so that it may be reused both at parse time and on
 * every (re)open.
 *
 * @param device
 *     The local device path to test.
 *
 * @param allowed
 *     The ":"-separated allowlist of permitted device paths or prefixes.
 *
 * @return
 *     true if the device is permitted, false otherwise.
 */
bool guac_serial_device_permitted(const char* device, const char* allowed);

/**
 * Frees the given guac_serial_settings object, having been previously
 * allocated via guac_serial_parse_args().
 *
 * @param settings
 *     The settings object to free.
 */
void guac_serial_settings_free(guac_serial_settings* settings);

/**
 * NULL-terminated array of accepted client args.
 */
extern const char* GUAC_SERIAL_CLIENT_ARGS[];

#endif
