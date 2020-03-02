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

#include "common/string.h"
#include "config.h"
#include "resolution.h"
#include "settings.h"

#include <freerdp/constants.h>
#include <freerdp/settings.h>
#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <guacamole/string.h>
#include <guacamole/user.h>
#include <winpr/crt.h>
#include <winpr/wtypes.h>

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Client plugin arguments */
const char* GUAC_RDP_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "domain",
    "username",
    "password",
    "width",
    "height",
    "dpi",
    "initial-program",
    "color-depth",
    "disable-audio",
    "enable-printing",
    "printer-name",
    "enable-drive",
    "drive-name",
    "drive-path",
    "create-drive-path",
    "console",
    "console-audio",
    "server-layout",
    "security",
    "ignore-cert",
    "disable-auth",
    "remote-app",
    "remote-app-dir",
    "remote-app-args",
    "static-channels",
    "client-name",
    "enable-wallpaper",
    "enable-theming",
    "enable-font-smoothing",
    "enable-full-window-drag",
    "enable-desktop-composition",
    "enable-menu-animations",
    "disable-bitmap-caching",
    "disable-offscreen-caching",
    "disable-glyph-caching",
    "preconnection-id",
    "preconnection-blob",
    "timezone",

#ifdef ENABLE_COMMON_SSH
    "enable-sftp",
    "sftp-hostname",
    "sftp-host-key",
    "sftp-port",
    "sftp-username",
    "sftp-password",
    "sftp-private-key",
    "sftp-passphrase",
    "sftp-directory",
    "sftp-root-directory",
    "sftp-server-alive-interval",
#endif

    "recording-path",
    "recording-name",
    "recording-exclude-output",
    "recording-exclude-mouse",
    "recording-include-keys",
    "create-recording-path",
    "resize-method",
    "enable-audio-input",
    "read-only",

    "gateway-hostname",
    "gateway-port",
    "gateway-domain",
    "gateway-username",
    "gateway-password",

    "load-balance-info",

    "disable-copy",
    "disable-paste",
    NULL
};

enum RDP_ARGS_IDX {
    
    /**
     * The hostname to connect to.
     */
    IDX_HOSTNAME,

    /**
     * The port to connect to. If omitted, the default RDP port of 3389 will be
     * used.
     */
    IDX_PORT,

    /**
     * The domain of the user logging in.
     */
    IDX_DOMAIN,

    /**
     * The username of the user logging in.
     */
    IDX_USERNAME,

    /**
     * The password of the user logging in.
     */
    IDX_PASSWORD,

    /**
     * The width of the display to request, in pixels. If omitted, a reasonable
     * value will be calculated based on the user's own display size and
     * resolution.
     */
    IDX_WIDTH,

    /**
     * The height of the display to request, in pixels. If omitted, a
     * reasonable value will be calculated based on the user's own display
     * size and resolution.
     */
    IDX_HEIGHT,

    /**
     * The resolution of the display to request, in DPI. If omitted, a
     * reasonable value will be calculated based on the user's own display
     * size and resolution.
     */
    IDX_DPI,

    /**
     * The initial program to run, if any.
     */
    IDX_INITIAL_PROGRAM,

    /**
     * The color depth of the display to request, in bits.
     */
    IDX_COLOR_DEPTH,

    /**
     * "true" if audio should be disabled, "false" or blank to leave audio
     * enabled.
     */
    IDX_DISABLE_AUDIO,

    /**
     * "true" if printing should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_PRINTING,

    /**
     * The name of the printer that will be passed through to the RDP server.
     */
    IDX_PRINTER_NAME,

    /**
     * "true" if the virtual drive should be enabled, "false" or blank
     * otherwise.
     */
    IDX_ENABLE_DRIVE,
    
    /**
     * The name of the virtual driver that will be passed through to the
     * RDP connection.
     */
    IDX_DRIVE_NAME,

    /**
     * The local system path which will be used to persist the
     * virtual drive. This must be specified if the virtual drive is enabled.
     */
    IDX_DRIVE_PATH,

    /**
     * "true" to automatically create the local system path used by the virtual
     * drive if it does not yet exist, "false" or blank otherwise.
     */
    IDX_CREATE_DRIVE_PATH,

    /**
     * "true" if this session is a console session, "false" or blank otherwise.
     */
    IDX_CONSOLE,

    /**
     * "true" if audio should be allowed in console sessions, "false" or blank
     * otherwise.
     */
    IDX_CONSOLE_AUDIO,

    /**
     * The name of the keymap chosen as the layout of the server. Legal names
     * are defined within the *.keymap files in the "keymaps" directory of the
     * source for Guacamole's RDP support.
     */
    IDX_SERVER_LAYOUT,

    /**
     * The type of security to use for the connection. Valid values are "rdp",
     * "tls", "nla", "nla-ext", or "any". By default, the security mode is
     * negotiated ("any").
     */
    IDX_SECURITY,

    /**
     * "true" if validity of the RDP server's certificate should be ignored,
     * "false" or blank if invalid certificates should result in a failure to
     * connect.
     */
    IDX_IGNORE_CERT,

    /**
     * "true" if authentication should be disabled, "false" or blank otherwise.
     * This is different from the authentication that takes place when a user
     * provides their username and password. Authentication is required by
     * definition for NLA.
     */
    IDX_DISABLE_AUTH,

    /**
     * The application to launch, if RemoteApp is in use.
     */
    IDX_REMOTE_APP,

    /**
     * The working directory of the remote application, if RemoteApp is in use.
     */
    IDX_REMOTE_APP_DIR,

    /**
     * The arguments to pass to the remote application, if RemoteApp is in use.
     */
    IDX_REMOTE_APP_ARGS,

    /**
     * Comma-separated list of the names of all static virtual channels that
     * should be connected to and exposed as Guacamole pipe streams, or blank
     * if no static virtual channels should be used. 
     */
    IDX_STATIC_CHANNELS,

    /**
     * The name of the client to submit to the RDP server upon connection.
     */
    IDX_CLIENT_NAME,

    /**
     * "true" if the desktop wallpaper should be visible, "false" or blank if
     * the desktop wallpaper should be hidden.
     */
    IDX_ENABLE_WALLPAPER,

    /**
     * "true" if desktop and window theming should be allowed, "false" or blank
     * if theming should be temporarily disabled on the desktop of the RDP
     * server for the sake of performance.
     */
    IDX_ENABLE_THEMING,

    /**
     * "true" if glyphs should be smoothed with antialiasing (ClearType),
     * "false" or blank if glyphs should be rendered with sharp edges and using
     * single colors, effectively 1-bit images.
     */
    IDX_ENABLE_FONT_SMOOTHING,

    /**
     * "true" if windows' contents should be shown as they are moved, "false"
     * or blank if only a window border should be shown during window move
     * operations.
     */
    IDX_ENABLE_FULL_WINDOW_DRAG,

    /**
     * "true" if desktop composition (Aero) should be enabled during the
     * session, "false" or blank otherwise. As desktop composition provides
     * alpha blending and other special effects, this increases the amount of
     * bandwidth used.
     */
    IDX_ENABLE_DESKTOP_COMPOSITION,

    /**
     * "true" if menu animations should be shown, "false" or blank menus should
     * not be animated.
     */
    IDX_ENABLE_MENU_ANIMATIONS,

    /**
     * "true" if bitmap caching should be disabled, "false" if bitmap caching
     * should remain enabled.
     */
    IDX_DISABLE_BITMAP_CACHING,

    /**
     * "true" if the offscreen caching should be disabled, false if offscren
     * caching should remain enabled.
     */
    IDX_DISABLE_OFFSCREEN_CACHING,

    /**
     * "true" if glyph caching should be disabled, false if glyph caching should
     * remain enabled.
     */
    IDX_DISABLE_GLYPH_CACHING,

    /**
     * The preconnection ID to send within the preconnection PDU when
     * initiating an RDP connection, if any.
     */
    IDX_PRECONNECTION_ID,

    /**
     * The preconnection BLOB (PCB) to send to the RDP server prior to full RDP
     * connection negotiation. This value is used by Hyper-V to select the
     * destination VM.
     */
    IDX_PRECONNECTION_BLOB,

    /**
     * The timezone to pass through to the RDP connection, in IANA format, which
     * will be translated into Windows formats. See the following page for
     * information and list of valid values:
     * https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
     */
    IDX_TIMEZONE,

#ifdef ENABLE_COMMON_SSH
    /**
     * "true" if SFTP should be enabled for the RDP connection, "false" or
     * blank otherwise.
     */
    IDX_ENABLE_SFTP,

    /**
     * The hostname of the SSH server to connect to for SFTP. If blank, the
     * hostname of the RDP server will be used.
     */
    IDX_SFTP_HOSTNAME,

    /**
     * The public SSH host key of the SFTP server. Optional.
     */
    IDX_SFTP_HOST_KEY,

    /**
     * The port of the SSH server to connect to for SFTP. If blank, the default
     * SSH port of "22" will be used.
     */
    IDX_SFTP_PORT,

    /**
     * The username to provide when authenticating with the SSH server for
     * SFTP. If blank, the username provided for the RDP user will be used.
     */
    IDX_SFTP_USERNAME,

    /**
     * The password to provide when authenticating with the SSH server for
     * SFTP (if not using a private key).
     */
    IDX_SFTP_PASSWORD,

    /**
     * The base64-encoded private key to use when authenticating with the SSH
     * server for SFTP (if not using a password).
     */
    IDX_SFTP_PRIVATE_KEY,

    /**
     * The passphrase to use to decrypt the provided base64-encoded private
     * key.
     */
    IDX_SFTP_PASSPHRASE,

    /**
     * The default location for file uploads within the SSH server. This will
     * apply only to uploads which do not use the filesystem guac_object (where
     * the destination directory is otherwise ambiguous).
     */
    IDX_SFTP_DIRECTORY,

    /**
     * The path of the directory within the SSH server to expose as a
     * filesystem guac_object. If omitted, "/" will be used by default.
     */
    IDX_SFTP_ROOT_DIRECTORY,

    /**
     * The interval at which SSH keepalive messages are sent to the server for
     * SFTP connections. The default is 0 (disabling keepalives), and a value
     * of 1 is automatically increased to 2 by libssh2 to avoid busy loop corner
     * cases.
     */
    IDX_SFTP_SERVER_ALIVE_INTERVAL,
#endif

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
     * Whether output which is broadcast to each connected client (graphics,
     * streams, etc.) should NOT be included in the session recording. Output
     * is included by default, as it is necessary for any recording which must
     * later be viewable as video.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether changes to mouse state, such as position and buttons pressed or
     * released, should NOT be included in the session recording. Mouse state
     * is included by default, as it is necessary for the mouse cursor to be
     * rendered in any resulting video.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the session
     * recording. Key events are NOT included by default within the recording,
     * as doing so has privacy and security implications. Including key events
     * may be necessary in certain auditing contexts, but should only be done
     * with caution. Key events can easily contain sensitive information, such
     * as passwords, credit card numbers, etc.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    /**
     * The method to use to apply screen size changes requested by the user.
     * Valid values are blank, "display-update", and "reconnect".
     */
    IDX_RESIZE_METHOD,

    /**
     * "true" if audio input (microphone) should be enabled for the RDP
     * connection, "false" or blank otherwise.
     */
    IDX_ENABLE_AUDIO_INPUT,

    /**
     * "true" if this connection should be read-only (user input should be
     * dropped), "false" or blank otherwise.
     */
    IDX_READ_ONLY,

    /**
     * The hostname of the remote desktop gateway that should be used as an
     * intermediary for the remote desktop connection. If omitted, a gateway
     * will not be used.
     */
    IDX_GATEWAY_HOSTNAME,

    /**
     * The port of the remote desktop gateway that should be used as an
     * intermediary for the remote desktop connection. By default, this will be
     * 443.
     *
     * NOTE: If using a version of FreeRDP prior to 1.2, this setting has no
     * effect. FreeRDP instead uses a hard-coded value of 443.
     */
    IDX_GATEWAY_PORT,

    /**
     * The domain of the user authenticating with the remote desktop gateway,
     * if a gateway is being used. This is not necessarily the same as the
     * user actually using the remote desktop connection.
     */
    IDX_GATEWAY_DOMAIN,

    /**
     * The username of the user authenticating with the remote desktop gateway,
     * if a gateway is being used. This is not necessarily the same as the
     * user actually using the remote desktop connection.
     */
    IDX_GATEWAY_USERNAME,

    /**
     * The password to provide when authenticating with the remote desktop
     * gateway, if a gateway is being used.
     */
    IDX_GATEWAY_PASSWORD,

    /**
     * The load balancing information/cookie which should be provided to
     * the connection broker, if a connection broker is being used.
     */
    IDX_LOAD_BALANCE_INFO,

    /**
     * Whether outbound clipboard access should be blocked. If set to "true",
     * it will not be possible to copy data from the remote desktop to the
     * client using the clipboard. By default, clipboard access is not blocked.
     */
    IDX_DISABLE_COPY,

    /**
     * Whether inbound clipboard access should be blocked. If set to "true", it
     * will not be possible to paste data from the client to the remote desktop
     * using the clipboard. By default, clipboard access is not blocked.
     */
    IDX_DISABLE_PASTE,

    RDP_ARGS_COUNT
};

guac_rdp_settings* guac_rdp_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != RDP_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                RDP_ARGS_COUNT, argc);
        return NULL;
    }

    guac_rdp_settings* settings = calloc(1, sizeof(guac_rdp_settings));

    /* Use console */
    settings->console =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CONSOLE, 0);

    /* Enable/disable console audio */
    settings->console_audio =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CONSOLE_AUDIO, 0);

    /* Ignore SSL/TLS certificate */
    settings->ignore_certificate =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_IGNORE_CERT, 0);

    /* Disable authentication */
    settings->disable_authentication =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_AUTH, 0);

    /* NLA security */
    if (strcmp(argv[IDX_SECURITY], "nla") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: NLA");
        settings->security_mode = GUAC_SECURITY_NLA;
    }

    /* Extended NLA security */
    else if (strcmp(argv[IDX_SECURITY], "nla-ext") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: Extended NLA");
        settings->security_mode = GUAC_SECURITY_EXTENDED_NLA;
    }

    /* TLS security */
    else if (strcmp(argv[IDX_SECURITY], "tls") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: TLS");
        settings->security_mode = GUAC_SECURITY_TLS;
    }

    /* RDP security */
    else if (strcmp(argv[IDX_SECURITY], "rdp") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: RDP");
        settings->security_mode = GUAC_SECURITY_RDP;
    }

    /* Negotiate security (allow server to choose) */
    else if (strcmp(argv[IDX_SECURITY], "any") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: Negotiate (ANY)");
        settings->security_mode = GUAC_SECURITY_ANY;
    }

    /* If nothing given, default to RDP */
    else {
        guac_user_log(user, GUAC_LOG_INFO, "No security mode specified. Defaulting to security mode negotiation with server.");
        settings->security_mode = GUAC_SECURITY_ANY;
    }

    /* Set hostname */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    /* If port specified, use it */
    settings->port =
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_PORT, RDP_DEFAULT_PORT);

    guac_user_log(user, GUAC_LOG_DEBUG,
            "User resolution is %ix%i at %i DPI",
            user->info.optimal_width,
            user->info.optimal_height,
            user->info.optimal_resolution);

    /* Use suggested resolution unless overridden */
    settings->resolution =
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DPI, guac_rdp_suggest_resolution(user));

    /* Use optimal width unless overridden */
    settings->width = user->info.optimal_width
                    * settings->resolution
                    / user->info.optimal_resolution;

    if (argv[IDX_WIDTH][0] != '\0')
        settings->width = atoi(argv[IDX_WIDTH]);

    /* Use default width if given width is invalid. */
    if (settings->width <= 0) {
        settings->width = RDP_DEFAULT_WIDTH;
        guac_user_log(user, GUAC_LOG_ERROR,
                "Invalid width: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->width);
    }

    /* Round width down to nearest multiple of 4 */
    settings->width = settings->width & ~0x3;

    /* Use optimal height unless overridden */
    settings->height = user->info.optimal_height
                     * settings->resolution
                     / user->info.optimal_resolution;

    if (argv[IDX_HEIGHT][0] != '\0')
        settings->height = atoi(argv[IDX_HEIGHT]);

    /* Use default height if given height is invalid. */
    if (settings->height <= 0) {
        settings->height = RDP_DEFAULT_HEIGHT;
        guac_user_log(user, GUAC_LOG_ERROR,
                "Invalid height: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->height);
    }

    guac_user_log(user, GUAC_LOG_DEBUG,
            "Using resolution of %ix%i at %i DPI",
            settings->width,
            settings->height,
            settings->resolution);

    /* Domain */
    settings->domain =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DOMAIN, NULL);

    /* Username */
    settings->username =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);

    /* Password */
    settings->password =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_READ_ONLY, 0);

    /* Client name */
    settings->client_name =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CLIENT_NAME, "Guacamole RDP");

    /* Initial program */
    settings->initial_program =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_INITIAL_PROGRAM, NULL);

    /* RemoteApp program */
    settings->remote_app =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_REMOTE_APP, NULL);

    /* RemoteApp working directory */
    settings->remote_app_dir =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_REMOTE_APP_DIR, NULL);

    /* RemoteApp arguments */
    settings->remote_app_args =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_REMOTE_APP_ARGS, NULL);

    /* Static virtual channels */
    settings->svc_names = NULL;
    if (argv[IDX_STATIC_CHANNELS][0] != '\0')
        settings->svc_names = guac_split(argv[IDX_STATIC_CHANNELS], ',');

    /*
     * Performance flags
     */

    settings->wallpaper_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_WALLPAPER, 0);

    settings->theming_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_THEMING, 0);

    settings->font_smoothing_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_FONT_SMOOTHING, 0);

    settings->full_window_drag_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_FULL_WINDOW_DRAG, 0);

    settings->desktop_composition_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_DESKTOP_COMPOSITION, 0);

    settings->menu_animations_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_MENU_ANIMATIONS, 0);

    settings->disable_bitmap_caching =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_BITMAP_CACHING, 0);

    settings->disable_offscreen_caching =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_OFFSCREEN_CACHING, 0);

    settings->disable_glyph_caching =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_GLYPH_CACHING, 0);

    /* Session color depth */
    settings->color_depth = 
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_COLOR_DEPTH, RDP_DEFAULT_DEPTH);

    /* Preconnection ID */
    settings->preconnection_id = -1;
    if (argv[IDX_PRECONNECTION_ID][0] != '\0') {

        /* Parse preconnection ID, warn if invalid */
        int preconnection_id = atoi(argv[IDX_PRECONNECTION_ID]);
        if (preconnection_id < 0)
            guac_user_log(user, GUAC_LOG_WARNING,
                    "Ignoring invalid preconnection ID: %i",
                    preconnection_id);

        /* Otherwise, assign specified ID */
        else {
            settings->preconnection_id = preconnection_id;
            guac_user_log(user, GUAC_LOG_DEBUG,
                    "Preconnection ID: %i", settings->preconnection_id);
        }

    }

    /* Preconnection BLOB */
    settings->preconnection_blob = NULL;
    if (argv[IDX_PRECONNECTION_BLOB][0] != '\0') {
        settings->preconnection_blob = strdup(argv[IDX_PRECONNECTION_BLOB]);
        guac_user_log(user, GUAC_LOG_DEBUG,
                "Preconnection BLOB: \"%s\"", settings->preconnection_blob);
    }

    /* Warn if support for the preconnection BLOB / ID is absent */
    if (settings->preconnection_blob != NULL
            || settings->preconnection_id != -1) {
        guac_user_log(user, GUAC_LOG_WARNING,
                "Installed version of FreeRDP lacks support for the "
                "preconnection PDU. The specified preconnection BLOB and/or "
                "ID will be ignored.");
    }

    /* Audio enable/disable */
    settings->audio_enabled =
        !guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_AUDIO, 0);

    /* Printing enable/disable */
    settings->printing_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_PRINTING, 0);

    /* Name of redirected printer */
    settings->printer_name =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_PRINTER_NAME, "Guacamole Printer");

    /* Drive enable/disable */
    settings->drive_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_DRIVE, 0);
    
    /* Name of the drive being passed through */
    settings->drive_name =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DRIVE_NAME, "Guacamole Filesystem");

    settings->drive_path =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DRIVE_PATH, "");

    settings->create_drive_path =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CREATE_DRIVE_PATH, 0);

    /* Pick keymap based on argument */
    settings->server_layout = NULL;
    if (argv[IDX_SERVER_LAYOUT][0] != '\0')
        settings->server_layout =
            guac_rdp_keymap_find(argv[IDX_SERVER_LAYOUT]);

    /* If no keymap requested, use default */
    if (settings->server_layout == NULL)
        settings->server_layout = guac_rdp_keymap_find(GUAC_DEFAULT_KEYMAP);

    /* Timezone if provided by client, or use handshake version */
    settings->timezone =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_TIMEZONE, user->info.timezone);

#ifdef ENABLE_COMMON_SSH
    /* SFTP enable/disable */
    settings->enable_sftp =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_SFTP, 0);

    /* Hostname for SFTP connection */
    settings->sftp_hostname =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_HOSTNAME, settings->hostname);

    /* The public SSH host key. */
    settings->sftp_host_key =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_HOST_KEY, NULL);

    /* Port for SFTP connection */
    settings->sftp_port =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PORT, "22");

    /* Username for SSH/SFTP authentication */
    settings->sftp_username =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_USERNAME,
                settings->username != NULL ? settings->username : "");

    /* Password for SFTP (if not using private key) */
    settings->sftp_password =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PASSWORD, "");

    /* Private key for SFTP (if not using password) */
    settings->sftp_private_key =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PRIVATE_KEY, NULL);

    /* Passphrase for decrypting the SFTP private key (if applicable */
    settings->sftp_passphrase =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PASSPHRASE, "");

    /* Default upload directory */
    settings->sftp_directory =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_DIRECTORY, NULL);

    /* SFTP root directory */
    settings->sftp_root_directory =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_ROOT_DIRECTORY, "/");

    /* Default keepalive value */
    settings->sftp_server_alive_interval =
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_SERVER_ALIVE_INTERVAL, 0);
#endif

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_RDP_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, 0);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, 0);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, 0);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, 0);

    /* No resize method */
    if (strcmp(argv[IDX_RESIZE_METHOD], "") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Resize method: none");
        settings->resize_method = GUAC_RESIZE_NONE;
    }

    /* Resize method: "reconnect" */
    else if (strcmp(argv[IDX_RESIZE_METHOD], "reconnect") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Resize method: reconnect");
        settings->resize_method = GUAC_RESIZE_RECONNECT;
    }

    /* Resize method: "display-update" */
    else if (strcmp(argv[IDX_RESIZE_METHOD], "display-update") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Resize method: display-update");
        settings->resize_method = GUAC_RESIZE_DISPLAY_UPDATE;
    }

    /* Default to no resize method if invalid */
    else {
        guac_user_log(user, GUAC_LOG_INFO, "Resize method \"%s\" invalid. ",
                "Defaulting to no resize method.", argv[IDX_RESIZE_METHOD]);
        settings->resize_method = GUAC_RESIZE_NONE;
    }

    /* Audio input enable/disable */
    settings->enable_audio_input =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_AUDIO_INPUT, 0);

    /* Set gateway hostname */
    settings->gateway_hostname =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_GATEWAY_HOSTNAME, NULL);

    /* If gateway port specified, use it */
    settings->gateway_port =
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_GATEWAY_PORT, 443);

    /* Set gateway domain */
    settings->gateway_domain =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_GATEWAY_DOMAIN, NULL);

    /* Set gateway username */
    settings->gateway_username =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_GATEWAY_USERNAME, NULL);

    /* Set gateway password */
    settings->gateway_password =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_GATEWAY_PASSWORD, NULL);

    /* Set load balance info */
    settings->load_balance_info =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_LOAD_BALANCE_INFO, NULL);

    /* Parse clipboard copy disable flag */
    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, 0);

    /* Parse clipboard paste disable flag */
    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, 0);

    /* Success */
    return settings;

}

void guac_rdp_settings_free(guac_rdp_settings* settings) {

    /* Free settings strings */
    free(settings->client_name);
    free(settings->domain);
    free(settings->drive_name);
    free(settings->drive_path);
    free(settings->hostname);
    free(settings->initial_program);
    free(settings->password);
    free(settings->preconnection_blob);
    free(settings->recording_name);
    free(settings->recording_path);
    free(settings->remote_app);
    free(settings->remote_app_args);
    free(settings->remote_app_dir);
    free(settings->timezone);
    free(settings->username);
    free(settings->printer_name);

    /* Free channel name array */
    if (settings->svc_names != NULL) {

        /* Free all elements of array */
        char** current = &(settings->svc_names[0]);
        while (*current != NULL) {
            free(*current);
            current++;
        }

        /* Free array itself */
        free(settings->svc_names);

    }

#ifdef ENABLE_COMMON_SSH
    /* Free SFTP settings */
    free(settings->sftp_directory);
    free(settings->sftp_root_directory);
    free(settings->sftp_host_key);
    free(settings->sftp_hostname);
    free(settings->sftp_passphrase);
    free(settings->sftp_password);
    free(settings->sftp_port);
    free(settings->sftp_private_key);
    free(settings->sftp_username);
#endif

    /* Free RD gateway information */
    free(settings->gateway_hostname);
    free(settings->gateway_domain);
    free(settings->gateway_username);
    free(settings->gateway_password);

    /* Free load balancer information string */
    free(settings->load_balance_info);

    /* Free settings structure */
    free(settings);

}

int guac_rdp_get_width(freerdp* rdp) {
    return rdp->settings->DesktopWidth;
}

int guac_rdp_get_height(freerdp* rdp) {
    return rdp->settings->DesktopHeight;
}

int guac_rdp_get_depth(freerdp* rdp) {
    return rdp->settings->ColorDepth;
}

/**
 * Given the settings structure of the Guacamole RDP client, calculates the
 * standard performance flag value to send to the RDP server. The value of
 * these flags is dictated by the RDP standard.
 *
 * @param guac_settings
 *     The settings structure to read performance settings from.
 *
 * @returns
 *     The standard RDP performance flag value representing the union of all
 *     performance settings within the given settings structure.
 */
static int guac_rdp_get_performance_flags(guac_rdp_settings* guac_settings) {

    /* No performance flags initially */
    int flags = PERF_FLAG_NONE;

    /* Desktop wallpaper */
    if (!guac_settings->wallpaper_enabled)
        flags |= PERF_DISABLE_WALLPAPER;

    /* Theming of desktop/windows */
    if (!guac_settings->theming_enabled)
        flags |= PERF_DISABLE_THEMING;

    /* Font smoothing (ClearType) */
    if (guac_settings->font_smoothing_enabled)
        flags |= PERF_ENABLE_FONT_SMOOTHING;

    /* Full-window drag */
    if (!guac_settings->full_window_drag_enabled)
        flags |= PERF_DISABLE_FULLWINDOWDRAG;

    /* Desktop composition (Aero) */
    if (guac_settings->desktop_composition_enabled)
        flags |= PERF_ENABLE_DESKTOP_COMPOSITION;

    /* Menu animations */
    if (!guac_settings->menu_animations_enabled)
        flags |= PERF_DISABLE_MENUANIMATIONS;

    return flags;

}

/**
 * Simple wrapper for strdup() which behaves identically to standard strdup(),
 * execpt that NULL will be returned if the provided string is NULL.
 *
 * @param str
 *     The string to duplicate as a newly-allocated string.
 *
 * @return
 *     A newly-allocated string containing identically the same content as the
 *     given string, or NULL if the given string was NULL.
 */
static char* guac_rdp_strdup(const char* str) {

    /* Return NULL if no string provided */
    if (str == NULL)
        return NULL;

    /* Otherwise just invoke strdup() */
    return strdup(str);

}

void guac_rdp_push_settings(guac_client* client,
        guac_rdp_settings* guac_settings, freerdp* rdp) {

    rdpSettings* rdp_settings = rdp->settings;

    /* Authentication */
    rdp_settings->Domain = guac_rdp_strdup(guac_settings->domain);
    rdp_settings->Username = guac_rdp_strdup(guac_settings->username);
    rdp_settings->Password = guac_rdp_strdup(guac_settings->password);

    /* Connection */
    rdp_settings->ServerHostname = guac_rdp_strdup(guac_settings->hostname);
    rdp_settings->ServerPort = guac_settings->port;

    /* Session */
    rdp_settings->ColorDepth = guac_settings->color_depth;
    rdp_settings->DesktopWidth = guac_settings->width;
    rdp_settings->DesktopHeight = guac_settings->height;
    rdp_settings->AlternateShell = guac_rdp_strdup(guac_settings->initial_program);
    rdp_settings->KeyboardLayout = guac_settings->server_layout->freerdp_keyboard_layout;

    /* Performance flags */
    /* Explicitly set flag value */
    rdp_settings->PerformanceFlags = guac_rdp_get_performance_flags(guac_settings);

    /* Set individual flags - some FreeRDP versions overwrite the above */
    rdp_settings->AllowFontSmoothing = guac_settings->font_smoothing_enabled;
    rdp_settings->DisableWallpaper = !guac_settings->wallpaper_enabled;
    rdp_settings->DisableFullWindowDrag = !guac_settings->full_window_drag_enabled;
    rdp_settings->DisableMenuAnims = !guac_settings->menu_animations_enabled;
    rdp_settings->DisableThemes = !guac_settings->theming_enabled;
    rdp_settings->AllowDesktopComposition = guac_settings->desktop_composition_enabled;

    /* Client name */
    if (guac_settings->client_name != NULL) {
        guac_strlcpy(rdp_settings->ClientHostname, guac_settings->client_name,
                RDP_CLIENT_HOSTNAME_SIZE);
    }

    /* Console */
    rdp_settings->ConsoleSession = guac_settings->console;
    rdp_settings->RemoteConsoleAudio = guac_settings->console_audio;

    /* Audio */
    rdp_settings->AudioPlayback = guac_settings->audio_enabled;

    /* Audio capture */
    rdp_settings->AudioCapture = guac_settings->enable_audio_input;

    /* Display Update channel */
    rdp_settings->SupportDisplayControl =
        (guac_settings->resize_method == GUAC_RESIZE_DISPLAY_UPDATE);

    /* Timezone redirection */
    if (guac_settings->timezone) {
        if (setenv("TZ", guac_settings->timezone, 1)) {
            guac_client_log(client, GUAC_LOG_WARNING,
                "Unable to forward timezone: TZ environment variable "
                "could not be set: %s", strerror(errno));
        }
    }

    /* Device redirection */
    rdp_settings->DeviceRedirection =  guac_settings->audio_enabled
                                    || guac_settings->drive_enabled
                                    || guac_settings->printing_enabled;

    /* Security */
    switch (guac_settings->security_mode) {

        /* Legacy RDP encryption */
        case GUAC_SECURITY_RDP:
            rdp_settings->RdpSecurity = TRUE;
            rdp_settings->TlsSecurity = FALSE;
            rdp_settings->NlaSecurity = FALSE;
            rdp_settings->ExtSecurity = FALSE;
            rdp_settings->UseRdpSecurityLayer = TRUE;
            rdp_settings->EncryptionLevel = ENCRYPTION_LEVEL_CLIENT_COMPATIBLE;
            rdp_settings->EncryptionMethods =
                  ENCRYPTION_METHOD_40BIT
                | ENCRYPTION_METHOD_128BIT 
                | ENCRYPTION_METHOD_FIPS;
            break;

        /* TLS encryption */
        case GUAC_SECURITY_TLS:
            rdp_settings->RdpSecurity = FALSE;
            rdp_settings->TlsSecurity = TRUE;
            rdp_settings->NlaSecurity = FALSE;
            rdp_settings->ExtSecurity = FALSE;
            break;

        /* Network level authentication */
        case GUAC_SECURITY_NLA:
            rdp_settings->RdpSecurity = FALSE;
            rdp_settings->TlsSecurity = FALSE;
            rdp_settings->NlaSecurity = TRUE;
            rdp_settings->ExtSecurity = FALSE;
            break;

        /* Extended network level authentication */
        case GUAC_SECURITY_EXTENDED_NLA:
            rdp_settings->RdpSecurity = FALSE;
            rdp_settings->TlsSecurity = FALSE;
            rdp_settings->NlaSecurity = FALSE;
            rdp_settings->ExtSecurity = TRUE;
            break;

        /* All security types */
        case GUAC_SECURITY_ANY:
            rdp_settings->RdpSecurity = TRUE;
            rdp_settings->TlsSecurity = TRUE;
            rdp_settings->NlaSecurity = guac_settings->username && guac_settings->password;
            rdp_settings->ExtSecurity = FALSE;
            break;

    }

    /* Authentication */
    rdp_settings->Authentication = !guac_settings->disable_authentication;
    rdp_settings->IgnoreCertificate = guac_settings->ignore_certificate;

    /* RemoteApp */
    if (guac_settings->remote_app != NULL) {
        rdp_settings->Workarea = TRUE;
        rdp_settings->RemoteApplicationMode = TRUE;
        rdp_settings->RemoteAppLanguageBarSupported = TRUE;
        rdp_settings->RemoteApplicationProgram = guac_rdp_strdup(guac_settings->remote_app);
        rdp_settings->ShellWorkingDirectory = guac_rdp_strdup(guac_settings->remote_app_dir);
        rdp_settings->RemoteApplicationCmdLine = guac_rdp_strdup(guac_settings->remote_app_args);
    }

    /* Preconnection ID */
    if (guac_settings->preconnection_id != -1) {
        rdp_settings->NegotiateSecurityLayer = FALSE;
        rdp_settings->SendPreconnectionPdu = TRUE;
        rdp_settings->PreconnectionId = guac_settings->preconnection_id;
    }

    /* Preconnection BLOB */
    if (guac_settings->preconnection_blob != NULL) {
        rdp_settings->NegotiateSecurityLayer = FALSE;
        rdp_settings->SendPreconnectionPdu = TRUE;
        rdp_settings->PreconnectionBlob = guac_rdp_strdup(guac_settings->preconnection_blob);
    }

    /* Enable use of RD gateway if a gateway hostname is provided */
    if (guac_settings->gateway_hostname != NULL) {

        /* Enable RD gateway */
        rdp_settings->GatewayEnabled = TRUE;

        /* RD gateway connection details */
        rdp_settings->GatewayHostname = guac_rdp_strdup(guac_settings->gateway_hostname);
        rdp_settings->GatewayPort = guac_settings->gateway_port;

        /* RD gateway credentials */
        rdp_settings->GatewayUseSameCredentials = FALSE;
        rdp_settings->GatewayDomain = guac_rdp_strdup(guac_settings->gateway_domain);
        rdp_settings->GatewayUsername = guac_rdp_strdup(guac_settings->gateway_username);
        rdp_settings->GatewayPassword = guac_rdp_strdup(guac_settings->gateway_password);

    }

    /* Store load balance info (and calculate length) if provided */
    if (guac_settings->load_balance_info != NULL) {
        rdp_settings->LoadBalanceInfo = (BYTE*) guac_rdp_strdup(guac_settings->load_balance_info);
        rdp_settings->LoadBalanceInfoLength = strlen(guac_settings->load_balance_info);
    }

    rdp_settings->BitmapCacheEnabled = !guac_settings->disable_bitmap_caching;
    rdp_settings->OffscreenSupportLevel = !guac_settings->disable_offscreen_caching;
    rdp_settings->GlyphSupportLevel = !guac_settings->disable_glyph_caching ? GLYPH_SUPPORT_FULL : GLYPH_SUPPORT_NONE;
    rdp_settings->OsMajorType = OSMAJORTYPE_UNSPECIFIED;
    rdp_settings->OsMinorType = OSMINORTYPE_UNSPECIFIED;
    rdp_settings->DesktopResize = TRUE;

    /* Claim support only for specific updates, independent of FreeRDP defaults */
    ZeroMemory(rdp_settings->OrderSupport, GUAC_RDP_ORDER_SUPPORT_LENGTH);
    rdp_settings->OrderSupport[NEG_DSTBLT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_SCRBLT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_MEMBLT_INDEX] = !guac_settings->disable_bitmap_caching;
    rdp_settings->OrderSupport[NEG_MEMBLT_V2_INDEX] = !guac_settings->disable_bitmap_caching;
    rdp_settings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = !guac_settings->disable_glyph_caching;
    rdp_settings->OrderSupport[NEG_FAST_INDEX_INDEX] = !guac_settings->disable_glyph_caching;
    rdp_settings->OrderSupport[NEG_FAST_GLYPH_INDEX] = !guac_settings->disable_glyph_caching;

#ifdef HAVE_RDPSETTINGS_ALLOWUNANOUNCEDORDERSFROMSERVER
    /* Do not consider server use of unannounced orders to be a fatal error */
    rdp_settings->AllowUnanouncedOrdersFromServer = TRUE;
#endif

}

