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


#ifndef GUAC_SPICE_SETTINGS_H
#define GUAC_SPICE_SETTINGS_H

#include "config.h"
#include "keymap.h"

#include <spice-client-glib-2.0/spice-client.h>

#include <stdbool.h>

/**
 * The filename to use for the screen recording, if not specified.
 */
#define GUAC_SPICE_DEFAULT_RECORDING_NAME "recording"

/**
 * Spice-specific client data.
 */
typedef struct guac_spice_settings {

    /**
     * The hostname of the Spice server (or repeater) to connect to.
     */
    char* hostname;

    /**
     * The port of the Spice server (or repeater) to connect to.
     */
    char* port;
    
    /**
     * Whether or not TLS should be used to connect to the SPICE server.
     */
    bool tls;
    
    /**
     * The type of TLS validation that should be done for encrypted connections
     * to Spice servers.
     */
    SpiceSessionVerify tls_verify;
    
    /**
     * One or more Base64-encoded certificates to use to validate TLS
     * connections to the Spice server.
     */
    char* ca;
    
    /**
     * A path to a file containing one more certificates that will be used to
     * validate TLS connections.
     */
    char* ca_file;
    
    /**
     * The public key of the Spice server for TLS verification.
     */
    char* pubkey;
    
    /**
     * Spice supports connecting to remote servers via a proxy server. You can
     * specify the proxy server to use in this property.
     */
    char* proxy;
    
    /**
     * The username given in the arguments.
     */
    char* username;
    
    /**
     * The password given in the arguments.
     */
    char* password;

    /**
     * Space-separated list of encodings to use within the Spice session.
     */
    char* encodings;

    /**
     * The color depth to request, in bits.
     */
    int color_depth;

    /**
     * Whether this connection is read-only, and user input should be dropped.
     */
    bool read_only;
   
    /**
     * Whether audio is enabled.
     */
    bool audio_enabled;

    /**
     * Whether audio input is enabled.
     */
    bool audio_input_enabled;
    
    /**
     * If file transfer capability should be enabled.
     */
    bool file_transfer;
    
    /**
     * The directory on the server where guacd is running that should be
     * shared.
     */
    char* file_directory;
    
    /**
     * If file transfer capability should be limited to read-only.
     */
    bool file_transfer_ro;

    /**
     * If the folder does not exist and this setting is set to True, guacd
     * will attempt to create the folder.
     */
    bool file_transfer_create_folder;

    /**
     * True if downloads (Remote Server -> Guacamole Client) should be
     * disabled.
     */
    bool disable_download;

    /**
     * True if uploads (Guacamole Client -> Remote Server) should be disabled.
     */
    bool disable_upload;

    /**
     * The keymap chosen as the layout of the server.
     */
    const guac_spice_keymap* server_layout;

    /**
     * The number of connection attempts to make before giving up.
     */
    int retries;

    /**
     * The encoding to use for clipboard data sent to the Spice server, or NULL
     * to use the encoding required by the Spice standard.
     */
    char* clipboard_encoding;

    /**
     * Whether outbound clipboard access should be blocked. If set, it will not
     * be possible to copy data from the remote desktop to the client using the
     * clipboard.
     */
    bool disable_copy;

    /**
     * Whether inbound clipboard access should be blocked. If set, it will not
     * be possible to paste data from the client to the remote desktop using
     * the clipboard.
     */
    bool disable_paste;

#ifdef ENABLE_COMMON_SSH
    /**
     * Whether SFTP should be enabled for the Spice connection.
     */
    bool enable_sftp;

    /**
     * The hostname of the SSH server to connect to for SFTP.
     */
    char* sftp_hostname;

    /**
     * The public SSH host key.
     */
    char* sftp_host_key;

    /**
     * The port of the SSH server to connect to for SFTP.
     */
    char* sftp_port;

    /**
     * The username to provide when authenticating with the SSH server for
     * SFTP.
     */
    char* sftp_username;

    /**
     * The password to provide when authenticating with the SSH server for
     * SFTP (if not using a private key).
     */
    char* sftp_password;

    /**
     * The base64-encoded private key to use when authenticating with the SSH
     * server for SFTP (if not using a password).
     */
    char* sftp_private_key;

    /**
     * The passphrase to use to decrypt the provided base64-encoded private
     * key.
     */
    char* sftp_passphrase;

    /**
     * The default location for file uploads within the SSH server. This will
     * apply only to uploads which do not use the filesystem guac_object (where
     * the destination directory is otherwise ambiguous).
     */
    char* sftp_directory;

    /**
     * The path of the directory within the SSH server to expose as a
     * filesystem guac_object.
     */
    char* sftp_root_directory;

    /**
     * The interval at which SSH keepalive messages are sent to the server for
     * SFTP connections.  The default is 0 (disabling keepalives), and a value
     * of 1 is automatically increased to 2 by libssh2 to avoid busy loop corner
     * cases.
     */
    int sftp_server_alive_interval;
    
    /**
     * Whether file downloads over SFTP should be blocked.  If set to "true",
     * the local client will not be able to download files from the SFTP server.
     * If set to "false" or not set, file downloads will be allowed.
     */
    bool sftp_disable_download;
    
    /**
     * Whether file uploads over SFTP should be blocked.  If set to "true", the
     * local client will not be able to upload files to the SFTP server.  If set
     * to "false" or not set, file uploads will be allowed.
     */
    bool sftp_disable_upload;
#endif

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

} guac_spice_settings;

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
 *     guac_spice_settings_free() when no longer needed. If the arguments fail
 *     to parse, NULL is returned.
 */
guac_spice_settings* guac_spice_parse_args(guac_user* user,
        int argc, const char** argv);

/**
 * Frees the given guac_spice_settings object, having been previously allocated
 * via guac_spice_parse_args().
 *
 * @param settings
 *     The settings object to free.
 */
void guac_spice_settings_free(guac_spice_settings* settings);

/**
 * NULL-terminated array of accepted client args.
 */
extern const char* GUAC_SPICE_CLIENT_ARGS[];

#endif /* SPICE_SETTINGS_H */
