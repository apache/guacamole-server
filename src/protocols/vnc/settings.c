/*
 * Copyright (C) 2014 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
#endif

    NULL
};

enum VNC_ARGS_IDX {

    IDX_HOSTNAME,
    IDX_PORT,
    IDX_READ_ONLY,
    IDX_ENCODINGS,
    IDX_PASSWORD,
    IDX_SWAP_RED_BLUE,
    IDX_COLOR_DEPTH,
    IDX_CURSOR,
    IDX_AUTORETRY,
    IDX_CLIPBOARD_ENCODING,

#ifdef ENABLE_VNC_REPEATER
    IDX_DEST_HOST,
    IDX_DEST_PORT,
#endif

#ifdef ENABLE_PULSE
    IDX_ENABLE_AUDIO,
    IDX_AUDIO_SERVERNAME,
#endif

#ifdef ENABLE_VNC_LISTEN
    IDX_REVERSE_CONNECT,
    IDX_LISTEN_TIMEOUT,
#endif

#ifdef ENABLE_COMMON_SSH
    IDX_ENABLE_SFTP,
    IDX_SFTP_HOSTNAME,
    IDX_SFTP_PORT,
    IDX_SFTP_USERNAME,
    IDX_SFTP_PASSWORD,
    IDX_SFTP_PRIVATE_KEY,
    IDX_SFTP_PASSPHRASE,
    IDX_SFTP_DIRECTORY,
#endif

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
                IDX_ENCODINGS, NULL);

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
#endif


    return settings;

}

void guac_vnc_settings_free(guac_vnc_settings* settings) {

    /* Free settings strings */
    free(settings->clipboard_encoding);
    free(settings->dest_host);
    free(settings->encodings);
    free(settings->hostname);

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

