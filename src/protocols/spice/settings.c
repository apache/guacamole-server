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
#include "client.h"
#include "common/defaults.h"
#include "settings.h"
#include "spice-defaults.h"
#include "spice-constants.h"

#include <guacamole/mem.h>
#include <guacamole/user.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <spice-client-glib-2.0/spice-client.h>

/* Client plugin arguments */
const char* GUAC_SPICE_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "tls",
    "tls-verify",
    "ca",
    "ca-file",
    "pubkey",
    "proxy",
    "read-only",
    "encodings",
    GUAC_SPICE_ARGV_USERNAME,
    GUAC_SPICE_ARGV_PASSWORD,
    "swap-red-blue",
    "color-depth",
    "cursor",
    "autoretry",
    "clipboard-encoding",

    "enable-audio",
    "enable-audio-input",
    "file-transfer",
    "file-directory",
    "file-transfer-ro",
    "file-transfer-create-folder",
    "disable-download",
    "disable-upload",
    "server-layout",

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
    "sftp-disable-download",
    "sftp-disable-upload",
#endif

    "recording-path",
    "recording-name",
    "recording-exclude-output",
    "recording-exclude-mouse",
    "recording-include-keys",
    "create-recording-path",
    "disable-copy",
    "disable-paste",
    
    NULL
};

enum SPICE_ARGS_IDX {
    
    /**
     * The hostname of the Spice server to connect to.
     */
    IDX_HOSTNAME,

    /**
     * The port of the Spice server to connect to.
     */
    IDX_PORT,
    
    /**
     * Whether or not the connection to the Spice server should be made via
     * TLS.
     */
    IDX_TLS,
    
    /**
     * The verification mode that should be used to validate TLS connections
     * to the Spice server.
     */
    IDX_TLS_VERIFY,
    
    /**
     * One or more Base64-encoded certificates that will be used for TLS
     * verification.
     */
    IDX_CA,
    
    /**
     * A path to a file containing one or more certificates that will be used
     * when validating TLS connections.
     */
    IDX_CA_FILE,
    
    /**
     * The public key of the host for TLS verification.
     */
    IDX_PUBKEY,
    
    /**
     * The proxy server to connect through when connecting to the Spice server.
     */
    IDX_PROXY,

    /**
     * "true" if this connection should be read-only (user input should be
     * dropped), "false" or blank otherwise.
     */
    IDX_READ_ONLY,

    /**
     * Space-separated list of encodings to use within the Spice session. If not
     * specified, this will be:
     *
     *     "zrle ultra copyrect hextile zlib corre rre raw".
     */
    IDX_ENCODINGS,

    /**
     * The username to send to the Spice server if authentication is requested.
     */
    IDX_USERNAME,
    
    /**
     * The password to send to the Spice server if authentication is requested.
     */
    IDX_PASSWORD,

    /**
     * "true" if the red and blue components of each color should be swapped,
     * "false" or blank otherwise. This is mainly used for Spice servers that do
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
     * The encoding to use for clipboard data sent to the Spice server if we are
     * going to be deviating from the standard (which mandates ISO 8829-1).
     * Valid values are "ISO8829-1" (the only legal value with respect to the
     * Spice standard), "UTF-8", "UTF-16", and "CP2252".
     */
    IDX_CLIPBOARD_ENCODING,

    /**
     * "true" if audio should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_AUDIO,

    /**
     * "true" if audio input should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_AUDIO_INPUT,

    /**
     * "true" if file transfer should be enabled, "false" or blank otherwise.
     */
    IDX_FILE_TRANSFER,

    /**
     * The absolute path to the directory that should be shared from the system
     * running guacd to the spice server.
     */
    IDX_FILE_DIRECTORY,
    
    /**
     * Whether or not the shared directory should be read-only to the Spice
     * server.
     */
    IDX_FILE_TRANSFER_RO,

    /**
     * Whether or not Guacamole should attempt to create the shared folder
     * if it does not already exist.
     */
    IDX_FILE_TRANSFER_CREATE_FOLDER,

    /**
     * "true" if downloads from the remote server to Guacamole client should
     * be disabled, otherwise false or blank.
     */
    IDX_DISABLE_DOWNLOAD,

    /**
     * "true" if uploads from Guacamole Client to the shared folder should be
     * disabled, otherwise false or blank.
     */
    IDX_DISABLE_UPLOAD,

    /**
     * The name of the keymap chosen as the layout of the server. Legal names
     * are defined within the *.keymap files in the "keymaps" directory of the
     * source for Guacamole's Spice support.
     */
    IDX_SERVER_LAYOUT,

#ifdef ENABLE_COMMON_SSH
    /**
     * "true" if SFTP should be enabled for the Spice connection, "false" or
     * blank otherwise.
     */
    IDX_ENABLE_SFTP,

    /**
     * The hostname of the SSH server to connect to for SFTP. If blank, the
     * hostname of the Spice server will be used.
     */
    IDX_SFTP_HOSTNAME,

    /**
     * The public SSH host key to identify the SFTP server.
     */
    IDX_SFTP_HOST_KEY,

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
     * The path of the directory within the SSH server to expose as a
     * filesystem guac_object. If omitted, "/" will be used by default.
     */
    IDX_SFTP_ROOT_DIRECTORY,

    /**
     * The interval at which SSH keepalive messages are sent to the server for
     * SFTP connections.  The default is 0 (disabling keepalives), and a value
     * of 1 is automatically incremented to 2 by libssh2 to avoid busy loop corner
     * cases.
     */
    IDX_SFTP_SERVER_ALIVE_INTERVAL,
    
    /**
     * If set to "true", file downloads over SFTP will be blocked.  If set to
     * "false" or not set, file downloads will be allowed.
     */
    IDX_SFTP_DISABLE_DOWNLOAD,
    
    /**
     * If set to "true", file uploads over SFTP will be blocked.  If set to
     * "false" or not set, file uploads will be allowed.
     */
    IDX_SFTP_DISABLE_UPLOAD,
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
     * as doing so has privacy and security implications.  Including key events
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

    SPICE_ARGS_COUNT
};

guac_spice_settings* guac_spice_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != SPICE_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                SPICE_ARGS_COUNT, argc);
        return NULL;
    }

    guac_spice_settings* settings = guac_mem_zalloc(sizeof(guac_spice_settings));

    settings->hostname =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_HOSTNAME, SPICE_DEFAULT_HOST);

    settings->port =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PORT, SPICE_DEFAULT_PORT);
    
    settings->tls =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_TLS, false);
    
    char* verify_mode = guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_TLS_VERIFY, NULL);
    
    if (verify_mode != NULL) {
        if (strcmp(verify_mode, GUAC_SPICE_PARAMETER_TLS_VERIFY_PUBKEY) == 0)
            settings->tls_verify = SPICE_SESSION_VERIFY_PUBKEY;
        else if (strcmp(verify_mode, GUAC_SPICE_PARAMETER_TLS_VERIFY_SUBJECT) == 0)
            settings->tls_verify = SPICE_SESSION_VERIFY_SUBJECT;    
    }
    
    else {
        settings->tls_verify = SPICE_SESSION_VERIFY_HOSTNAME;
    }
    
    free(verify_mode);
    
    settings->ca =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CA, NULL);
    
    settings->ca_file =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CA_FILE, NULL);
    
    settings->pubkey =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PUBKEY, NULL);
    
    settings->proxy =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PROXY, NULL);

    settings->username =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);
    
    settings->password =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Parse color depth */
    settings->color_depth =
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_COLOR_DEPTH, 0);

    /* Set encodings if specified */
    settings->encodings =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENCODINGS,
                SPICE_DEFAULT_ENCODINGS);

    /* Parse autoretry */
    settings->retries =
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_AUTORETRY, 0);

    /* Audio enable/disable */
    settings->audio_enabled =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENABLE_AUDIO, false);

    /* Audio input enable/disable */
    settings->audio_input_enabled =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENABLE_AUDIO_INPUT, false);

    /* File transfer enable/disable */
    settings->file_transfer =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER, false);
    
    /* The directory on the guacd server to share */
    settings->file_directory =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_DIRECTORY, NULL);
    
    /* Whether or not the share should be read-only. */
    settings->file_transfer_ro =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER_RO, false);

    /* Whether or not Guacamole should attempt to create a non-existent folder. */
    settings->file_transfer_create_folder =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER_CREATE_FOLDER, false);

    /* Whether or not downloads (Server -> Client) should be disabled. */
    settings->disable_download =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_DOWNLOAD, false);

    /* Whether or not uploads (Client -> Server) should be disabled. */
    settings->disable_upload =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_UPLOAD, false);

    /* Pick keymap based on argument */
    settings->server_layout = NULL;
    if (argv[IDX_SERVER_LAYOUT][0] != '\0')
        settings->server_layout =
            guac_spice_keymap_find(argv[IDX_SERVER_LAYOUT]);

    /* If no keymap requested, use default */
    if (settings->server_layout == NULL)
        settings->server_layout = guac_spice_keymap_find(GUAC_SPICE_DEFAULT_KEYMAP);

    /* Set clipboard encoding if specified */
    settings->clipboard_encoding =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CLIPBOARD_ENCODING, NULL);

#ifdef ENABLE_COMMON_SSH
    /* SFTP enable/disable */
    settings->enable_sftp =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENABLE_SFTP, false);

    /* Hostname for SFTP connection */
    settings->sftp_hostname =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_HOSTNAME, settings->hostname);

    /* The public SSH host key. */
    settings->sftp_host_key =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_HOST_KEY, NULL);

    /* Port for SFTP connection */
    settings->sftp_port =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PORT, SPICE_DEFAULT_SFTP_PORT);

    /* Username for SSH/SFTP authentication */
    settings->sftp_username =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_USERNAME, "");

    /* Password for SFTP (if not using private key) */
    settings->sftp_password =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PASSWORD, "");

    /* Private key for SFTP (if not using password) */
    settings->sftp_private_key =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PRIVATE_KEY, NULL);

    /* Passphrase for decrypting the SFTP private key (if applicable */
    settings->sftp_passphrase =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PASSPHRASE, "");

    /* Default upload directory */
    settings->sftp_directory =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_DIRECTORY, NULL);

    /* SFTP root directory */
    settings->sftp_root_directory =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_ROOT_DIRECTORY, SPICE_DEFAULT_SFTP_ROOT);

    /* Default keepalive value */
    settings->sftp_server_alive_interval =
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_SERVER_ALIVE_INTERVAL, 0);
    
    settings->sftp_disable_download =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_DISABLE_DOWNLOAD, false);
    
    settings->sftp_disable_upload =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_DISABLE_UPLOAD, false);
#endif

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_SPICE_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    /* Parse clipboard copy disable flag */
    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, false);

    /* Parse clipboard paste disable flag */
    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, false);

    return settings;

}

void guac_spice_settings_free(guac_spice_settings* settings) {

    /* Free settings strings */
    free(settings->clipboard_encoding);
    free(settings->encodings);
    free(settings->hostname);
    free(settings->password);
    free(settings->recording_name);
    free(settings->recording_path);
    free(settings->username);

#ifdef ENABLE_SPICE_REPEATER
    /* Free Spice repeater settings */
    free(settings->dest_host);
#endif

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

    /* Free settings structure */
    free(settings);

}

