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

#include "client.h"
#include "settings.h"

#include <guacamole/user.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Client plugin arguments */
const char* GUAC_VNC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "read-only",
    "encodings",
    "password",
    "swap-red-blue",
    "color-depth",
    "cursor",
    "autoretry",
    "clipboard-encoding",

#ifdef ENABLE_VNC_REPEATER
    "dest-host",
    "dest-port",
#endif

#ifdef ENABLE_PULSE
    "enable-audio",
    "audio-servername",
#endif

#ifdef ENABLE_VNC_LISTEN
    "reverse-connect",
    "listen-timeout",
#endif

#ifdef ENABLE_COMMON_SSH
    "enable-sftp",
    "sftp-hostname",
    "sftp-port",
    "sftp-username",
    "sftp-password",
    "sftp-private-key",
    "sftp-passphrase",
    "sftp-directory",
    "sftp-server-alive-interval",
#endif

    "recording-path",
    "recording-name",
    "create-recording-path",

    NULL
};

enum VNC_ARGS_IDX {

    /**
     * The hostname of the VNC server (or repeater) to connect to.
     */
    IDX_HOSTNAME,

    /**
     * The port of the VNC server (or repeater) to connect to.
     */
    IDX_PORT,

    /**
     * "true" if this connection should be read-only (user input should be
     * dropped), "false" or blank otherwise.
     */
    IDX_READ_ONLY,

    /**
     * Space-separated list of encodings to use within the VNC session. If not
     * specified, this will be:
     *
     *     "zrle ultra copyrect hextile zlib corre rre raw".
     */
    IDX_ENCODINGS,

    /**
     * The password to send to the VNC server if authentication is requested.
     */
    IDX_PASSWORD,

    /**
     * "true" if the red and blue components of each color should be swapped,
     * "false" or blank otherwise. This is mainly used for VNC servers that do
     * not properly handle colors.
     */
    IDX_SWAP_RED_BLUE,

    /**
     * The color depth to request, in bits.
     */
    IDX_COLOR_DEPTH,

    /**
     * "remote" if the cursor should be rendered on the server instead of the
     * client. All other values will default to local rendering.
     */
    IDX_CURSOR,

    /**
     * The number of connection attempts to make before giving up. By default,
     * this will be 0.
     */
    IDX_AUTORETRY,

    /**
     * The encoding to use for clipboard data sent to the VNC server if we are
     * going to be deviating from the standard (which mandates ISO 8829-1).
     * Valid values are "ISO8829-1" (the only legal value with respect to the
     * VNC standard), "UTF-8", "UTF-16", and "CP2252".
     */
    IDX_CLIPBOARD_ENCODING,

#ifdef ENABLE_VNC_REPEATER
    /**
     * The VNC host to connect to, if using a repeater.
     */
    IDX_DEST_HOST,

    /**
     * The VNC port to connect to, if using a repeater.
     */
    IDX_DEST_PORT,
#endif

#ifdef ENABLE_PULSE
    /**
     * "true" if audio should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_AUDIO,

    /**
     * The name of the PulseAudio server to connect to. If left blank, the
     * default sink of the local machine will be used as the source for audio.
     */
    IDX_AUDIO_SERVERNAME,
#endif

#ifdef ENABLE_VNC_LISTEN
    /**
     * "true" if not actually connecting to a VNC server, but rather listening
     * for a connection from the VNC server (reverse connection), "false" or
     * blank otherwise.
     */
    IDX_REVERSE_CONNECT,

    /**
     * The maximum amount of time to wait when listening for connections, in
     * milliseconds. If unspecified, this will default to 5000.
     */
    IDX_LISTEN_TIMEOUT,
#endif

#ifdef ENABLE_COMMON_SSH
    /**
     * "true" if SFTP should be enabled for the VNC connection, "false" or
     * blank otherwise.
     */
    IDX_ENABLE_SFTP,

    /**
     * The hostname of the SSH server to connect to for SFTP. If blank, the
     * hostname of the VNC server will be used.
     */
    IDX_SFTP_HOSTNAME,

    /**
     * The port of the SSH server to connect to for SFTP. If blank, the default
     * SSH port of "22" will be used.
     */
    IDX_SFTP_PORT,

    /**
     * The username to provide when authenticating with the SSH server for
     * SFTP.
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
     * The interval at which SSH keepalive messages are sent to the server for
     * SFTP connections.  The default is 0 (disabling keepalives), and a value
     * of 1 is automatically incremented to 2 by libssh2 to avoid busy loop corner
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
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    VNC_ARGS_COUNT
};

guac_vnc_settings* guac_vnc_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != VNC_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                VNC_ARGS_COUNT, argc);
        return NULL;
    }

    guac_vnc_settings* settings = calloc(1, sizeof(guac_vnc_settings));

    settings->hostname =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    settings->port =
        guac_user_parse_args_int(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_PORT, 0);

    settings->password =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_PASSWORD, ""); /* NOTE: freed by libvncclient */

    /* Remote cursor */
    if (strcmp(argv[IDX_CURSOR], "remote") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Cursor rendering: remote");
        settings->remote_cursor = true;
    }

    /* Local cursor */
    else {
        guac_user_log(user, GUAC_LOG_INFO, "Cursor rendering: local");
        settings->remote_cursor = false;
    }

    /* Swap red/blue (for buggy VNC servers) */
    settings->swap_red_blue =
        guac_user_parse_args_boolean(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SWAP_RED_BLUE, false);

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Parse color depth */
    settings->color_depth =
        guac_user_parse_args_int(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_COLOR_DEPTH, 0);

#ifdef ENABLE_VNC_REPEATER
    /* Set repeater parameters if specified */
    settings->dest_host =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_DEST_HOST, NULL);

    /* VNC repeater port */
    settings->dest_port =
        guac_user_parse_args_int(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_DEST_PORT, 0);
#endif

    /* Set encodings if specified */
    settings->encodings =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_ENCODINGS,
                "zrle ultra copyrect hextile zlib corre rre raw");

    /* Parse autoretry */
    settings->retries =
        guac_user_parse_args_int(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_AUTORETRY, 0);

#ifdef ENABLE_VNC_LISTEN
    /* Set reverse-connection flag */
    settings->reverse_connect =
        guac_user_parse_args_boolean(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_REVERSE_CONNECT, false);

    /* Parse listen timeout */
    settings->listen_timeout =
        guac_user_parse_args_int(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_LISTEN_TIMEOUT, 5000);
#endif

#ifdef ENABLE_PULSE
    /* Audio enable/disable */
    settings->audio_enabled =
        guac_user_parse_args_boolean(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_ENABLE_AUDIO, false);

    /* Load servername if specified and applicable */
    if (settings->audio_enabled)
        settings->pa_servername =
            guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                    IDX_AUDIO_SERVERNAME, NULL);
#endif

    /* Set clipboard encoding if specified */
    settings->clipboard_encoding =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_CLIPBOARD_ENCODING, NULL);

#ifdef ENABLE_COMMON_SSH
    /* SFTP enable/disable */
    settings->enable_sftp =
        guac_user_parse_args_boolean(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_ENABLE_SFTP, false);

    /* Hostname for SFTP connection */
    settings->sftp_hostname =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_HOSTNAME, settings->hostname);

    /* Port for SFTP connection */
    settings->sftp_port =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_PORT, "22");

    /* Username for SSH/SFTP authentication */
    settings->sftp_username =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_USERNAME, "");

    /* Password for SFTP (if not using private key) */
    settings->sftp_password =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_PASSWORD, "");

    /* Private key for SFTP (if not using password) */
    settings->sftp_private_key =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_PRIVATE_KEY, NULL);

    /* Passphrase for decrypting the SFTP private key (if applicable */
    settings->sftp_passphrase =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_PASSPHRASE, "");

    /* Default upload directory */
    settings->sftp_directory =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_DIRECTORY, NULL);

    /* Default keepalive value */
    settings->sftp_server_alive_interval =
        guac_user_parse_args_int(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_SFTP_SERVER_ALIVE_INTERVAL, 0);
#endif

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_VNC_DEFAULT_RECORDING_NAME);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_VNC_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    return settings;

}

void guac_vnc_settings_free(guac_vnc_settings* settings) {

    /* Free settings strings */
    free(settings->clipboard_encoding);
    free(settings->encodings);
    free(settings->hostname);
    free(settings->recording_name);
    free(settings->recording_path);

#ifdef ENABLE_VNC_REPEATER
    /* Free VNC repeater settings */
    free(settings->dest_host);
#endif

#ifdef ENABLE_COMMON_SSH
    /* Free SFTP settings */
    free(settings->sftp_directory);
    free(settings->sftp_hostname);
    free(settings->sftp_passphrase);
    free(settings->sftp_password);
    free(settings->sftp_port);
    free(settings->sftp_private_key);
    free(settings->sftp_username);
#endif

#ifdef ENABLE_PULSE
    /* Free PulseAudio settings */
    free(settings->pa_servername);
#endif

    /* Free settings structure */
    free(settings);

}

