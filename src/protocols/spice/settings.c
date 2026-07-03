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
#include "keymap.h"
#include "settings.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/user.h>
#include <guacamole/wol-constants.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Client plugin arguments */
const char* GUAC_SPICE_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "tls-port",
    GUAC_SPICE_ARGV_USERNAME,
    GUAC_SPICE_ARGV_PASSWORD,
    "tls",
    "ca-cert",
    "ignore-cert",
    "cert-subject",
    "pubkey",
    "proxy",
    "color-depth",
    "swap-red-blue",
    "preferred-compression",
    "server-layout",
    "read-only",
    "disable-display-resize",
    "disable-copy",
    "disable-paste",
    "disable-clipboard",
    "clipboard-buffer-size",
    "enable-audio",
    "enable-audio-input",
    "disable-audio-opus",
    "enable-drive",
    "drive-path",
    "drive-read-only",
    "file-transfer",
    "file-directory",
    "file-transfer-create-folder",
    "file-transfer-ro",
    "disable-download",
    "disable-upload",

#ifdef ENABLE_COMMON_SSH
    "enable-sftp",
    "sftp-hostname",
    "sftp-host-key",
    "sftp-port",
    "sftp-timeout",
    "sftp-username",
    "sftp-password",
    "sftp-private-key",
    "sftp-passphrase",
    "sftp-public-key",
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
    "recording-write-existing",
    "wol-send-packet",
    "wol-mac-addr",
    "wol-broadcast-addr",
    "wol-udp-port",
    "wol-wait-time",
    "preferred-video-codec",

    NULL
};

enum SPICE_ARGS_IDX {

    /**
     * The hostname of the SPICE server to connect to.
     */
    IDX_HOSTNAME,

    /**
     * The port of the SPICE server to connect to (plaintext).
     */
    IDX_PORT,

    /**
     * The TLS port of the SPICE server to connect to.
     */
    IDX_TLS_PORT,

    /**
     * The username to provide, retained for logging purposes.
     */
    IDX_USERNAME,

    /**
     * The password (ticket) to use when authenticating.
     */
    IDX_PASSWORD,

    /**
     * "true" if TLS should be used to secure the connection, "false" or blank
     * otherwise.
     */
    IDX_TLS,

    /**
     * The path to a PEM-encoded certificate authority file used to verify the
     * SPICE server's TLS certificate.
     */
    IDX_CA_CERT,

    /**
     * "true" if verification of the SPICE server's TLS certificate should be
     * disabled, "false" or blank otherwise.
     */
    IDX_IGNORE_CERT,

    /**
     * The expected subject of the SPICE server's TLS certificate.
     */
    IDX_CERT_SUBJECT,

    /**
     * The base64-encoded (DER) public key which the SPICE server's TLS
     * certificate is expected to present, for public-key pinning.
     */
    IDX_PUBKEY,

    /**
     * The proxy server, if any, to connect through when reaching the SPICE
     * server (e.g. "http://proxy.example.com:3128").
     */
    IDX_PROXY,

    /**
     * The color depth to request, in bits.
     */
    IDX_COLOR_DEPTH,

    /**
     * "true" if the red and blue color channels of the remote framebuffer
     * should be swapped, "false" or blank otherwise. Needed for the rare SPICE
     * server which reports its surface in BGR rather than RGB order.
     */
    IDX_SWAP_RED_BLUE,

    /**
     * The preferred image compression for the SPICE session (one of "off",
     * "auto-glz", "auto-lz", "quic", "glz", "lz", "lz4"), or blank to let the
     * server decide.
     */
    IDX_PREFERRED_COMPRESSION,

    /**
     * The name of the keyboard layout to use when translating keysyms into
     * SPICE scancodes (e.g. "en-us-qwerty", "es-es-qwerty", "es-latam-qwerty").
     */
    IDX_SERVER_LAYOUT,

    /**
     * "true" if this connection should be read-only, "false" or blank
     * otherwise.
     */
    IDX_READ_ONLY,

    /**
     * "true" if remote display resize should be disabled, "false" or blank
     * otherwise.
     */
    IDX_DISABLE_DISPLAY_RESIZE,

    /**
     * "true" if outbound (remote-to-client) clipboard access should be
     * blocked, "false" or blank otherwise.
     */
    IDX_DISABLE_COPY,

    /**
     * "true" if inbound (client-to-remote) clipboard access should be blocked,
     * "false" or blank otherwise.
     */
    IDX_DISABLE_PASTE,

    /**
     * "true" if clipboard integration should be disabled entirely, "false" or
     * blank otherwise.
     */
    IDX_DISABLE_CLIPBOARD,

    /**
     * The maximum number of bytes to allow within the clipboard.
     */
    IDX_CLIPBOARD_BUFFER_SIZE,

    /**
     * "true" if audio playback should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_AUDIO,

    /**
     * "true" if audio input (e.g. microphone) should be enabled, "false" or
     * blank otherwise.
     */
    IDX_ENABLE_AUDIO_INPUT,

    /**
     * "true" if the Opus audio codec should be disabled, causing the SPICE
     * server to fall back to its next available audio mode (raw on modern
     * servers, or the legacy CELT codec on older ones), "false" or blank
     * otherwise.
     */
    IDX_DISABLE_AUDIO_OPUS,

    /**
     * "true" if folder sharing should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_DRIVE,

    /**
     * The local directory to expose as a shared folder.
     */
    IDX_DRIVE_PATH,

    /**
     * "true" if the shared folder should be read-only, "false" or blank
     * otherwise.
     */
    IDX_DRIVE_READ_ONLY,

    /**
     * "true" if the web-UI file browser over the shared folder should be
     * enabled, "false" or blank otherwise.
     */
    IDX_FILE_TRANSFER,

    /**
     * The host-side path of the shared folder exposed to the file browser.
     */
    IDX_FILE_DIRECTORY,

    /**
     * "true" if the shared folder should be created if it does not yet exist,
     * "false" or blank otherwise.
     */
    IDX_FILE_TRANSFER_CREATE_FOLDER,

    /**
     * "true" if the shared folder should be exposed read-only to the SPICE
     * server, "false" or blank otherwise.
     */
    IDX_FILE_TRANSFER_RO,

    /**
     * "true" if file downloads from the shared folder should be disabled,
     * "false" or blank otherwise.
     */
    IDX_DISABLE_DOWNLOAD,

    /**
     * "true" if file uploads to the shared folder should be disabled, "false"
     * or blank otherwise.
     */
    IDX_DISABLE_UPLOAD,

#ifdef ENABLE_COMMON_SSH
    /**
     * "true" if SFTP should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_SFTP,

    /**
     * The hostname of the SSH server to connect to for SFTP.
     */
    IDX_SFTP_HOSTNAME,

    /**
     * The public SSH host key of the SFTP server.
     */
    IDX_SFTP_HOST_KEY,

    /**
     * The port of the SSH server to connect to for SFTP.
     */
    IDX_SFTP_PORT,

    /**
     * The number of seconds to attempt to connect to the SSH server before
     * giving up.
     */
    IDX_SFTP_TIMEOUT,

    /**
     * The username to provide when authenticating with the SSH server for
     * SFTP.
     */
    IDX_SFTP_USERNAME,

    /**
     * The password to provide when authenticating with the SSH server for
     * SFTP.
     */
    IDX_SFTP_PASSWORD,

    /**
     * The base64-encoded private key to use for SFTP authentication.
     */
    IDX_SFTP_PRIVATE_KEY,

    /**
     * The passphrase to use to decrypt the private key.
     */
    IDX_SFTP_PASSPHRASE,

    /**
     * The base64-encoded public key to use for SFTP authentication.
     */
    IDX_SFTP_PUBLIC_KEY,

    /**
     * The default upload directory within the SSH server.
     */
    IDX_SFTP_DIRECTORY,

    /**
     * The directory to expose as a filesystem guac_object.
     */
    IDX_SFTP_ROOT_DIRECTORY,

    /**
     * The SSH keepalive interval, in seconds.
     */
    IDX_SFTP_SERVER_ALIVE_INTERVAL,

    /**
     * "true" if SFTP downloads should be blocked, "false" or blank otherwise.
     */
    IDX_SFTP_DISABLE_DOWNLOAD,

    /**
     * "true" if SFTP uploads should be blocked, "false" or blank otherwise.
     */
    IDX_SFTP_DISABLE_UPLOAD,
#endif

    /**
     * The full absolute path to the directory in which screen recordings
     * should be written.
     */
    IDX_RECORDING_PATH,

    /**
     * The name of the recording which should be written.
     */
    IDX_RECORDING_NAME,

    /**
     * Whether output should NOT be included in the screen recording.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether mouse state should NOT be included in the screen recording.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the screen
     * recording.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * recording.
     */
    IDX_RECORDING_WRITE_EXISTING,

    /**
     * Whether to send the magic Wake-on-LAN (WoL) packet prior to connecting.
     */
    IDX_WOL_SEND_PACKET,

    /**
     * The MAC address to place in the magic WoL packet.
     */
    IDX_WOL_MAC_ADDR,

    /**
     * The broadcast address to which to send the magic WoL packet.
     */
    IDX_WOL_BROADCAST_ADDR,

    /**
     * The UDP port to use when sending the WoL packet.
     */
    IDX_WOL_UDP_PORT,

    /**
     * The number of seconds to wait after sending the WoL packet before
     * attempting to connect.
     */
    IDX_WOL_WAIT_TIME,

    /**
     * The video codec the SPICE server should prefer for streamed video
     * regions ("h264", "vp9", "vp8", or "mjpeg"), or blank to leave the
     * server's default preference (which starts with MJPEG) untouched.
     * Requesting a non-MJPEG codec depends on the server being built with, and
     * having, a working GStreamer encoder for it.
     */
    IDX_PREFERRED_VIDEO_CODEC,

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
                IDX_HOSTNAME, "");

    settings->port =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PORT, "");

    settings->tls_port =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_TLS_PORT, "");

    settings->username =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);

    settings->password =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    settings->tls =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_TLS, false);

    settings->ca_file =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CA_CERT, NULL);

    settings->ignore_cert =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_IGNORE_CERT, false);

    settings->cert_subject =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CERT_SUBJECT, NULL);

    settings->pubkey =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PUBKEY, NULL);

    settings->proxy =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PROXY, NULL);

    settings->color_depth =
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_COLOR_DEPTH, 0);

    settings->swap_red_blue =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SWAP_RED_BLUE, false);

    settings->preferred_compression =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PREFERRED_COMPRESSION, NULL);

    settings->server_layout =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SERVER_LAYOUT, GUAC_SPICE_DEFAULT_KEYMAP);

    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    settings->disable_display_resize =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_DISPLAY_RESIZE, false);

    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, false);

    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, false);

    settings->disable_clipboard =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_CLIPBOARD, false);

    settings->clipboard_buffer_size =
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CLIPBOARD_BUFFER_SIZE,
                GUAC_SPICE_CLIPBOARD_DEFAULT_BUFFER_SIZE);

    settings->audio_enabled =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENABLE_AUDIO, false);

    settings->audio_input_enabled =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENABLE_AUDIO_INPUT, false);

    settings->disable_audio_opus =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_AUDIO_OPUS, false);

    settings->preferred_video_codec =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_PREFERRED_VIDEO_CODEC, NULL);

    settings->enable_drive =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENABLE_DRIVE, false);

    settings->drive_path =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DRIVE_PATH, NULL);

    settings->drive_read_only =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DRIVE_READ_ONLY, false);

    settings->file_transfer =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER, false);

    settings->file_directory =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_DIRECTORY, NULL);

    settings->file_transfer_create_folder =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER_CREATE_FOLDER, false);

    settings->file_transfer_ro =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER_RO, false);

    settings->disable_download =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_DOWNLOAD, false);

    settings->disable_upload =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_DISABLE_UPLOAD, false);

    /* If neither a plaintext nor a TLS port was provided, fall back to the
     * default SPICE port for the appropriate transport */
    if (strcmp(settings->port, "") == 0 && strcmp(settings->tls_port, "") == 0) {
        if (settings->tls) {
            guac_mem_free(settings->tls_port);
            settings->tls_port = guac_strdup("5900");
        }
        else {
            guac_mem_free(settings->port);
            settings->port = guac_strdup("5900");
        }
    }

#ifdef ENABLE_COMMON_SSH
    settings->enable_sftp =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_ENABLE_SFTP, false);

    settings->sftp_hostname =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_HOSTNAME, settings->hostname);

    settings->sftp_host_key =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_HOST_KEY, NULL);

    settings->sftp_port =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PORT, "22");

    settings->sftp_timeout =
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_TIMEOUT, GUAC_SPICE_DEFAULT_SFTP_TIMEOUT);

    settings->sftp_username =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_USERNAME, "");

    settings->sftp_password =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PASSWORD, "");

    settings->sftp_private_key =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PRIVATE_KEY, NULL);

    settings->sftp_passphrase =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PASSPHRASE, "");

    settings->sftp_public_key =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_PUBLIC_KEY, NULL);

    settings->sftp_directory =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_DIRECTORY, NULL);

    settings->sftp_root_directory =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_SFTP_ROOT_DIRECTORY, "/");

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

    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_SPICE_DEFAULT_RECORDING_NAME);

    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    settings->recording_write_existing =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_WRITE_EXISTING, false);

    settings->wol_send_packet =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_WOL_SEND_PACKET, false);

    settings->wol_mac_addr =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_WOL_MAC_ADDR, NULL);

    settings->wol_broadcast_addr =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_WOL_BROADCAST_ADDR, GUAC_WOL_LOCAL_IPV4_BROADCAST);

    settings->wol_udp_port = (unsigned short)
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_WOL_UDP_PORT, GUAC_WOL_PORT);

    settings->wol_wait_time =
        guac_user_parse_args_int(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_WOL_WAIT_TIME, 0);

    return settings;

}

void guac_spice_settings_free(guac_spice_settings* settings) {

    /* Free settings strings */
    guac_mem_free(settings->hostname);
    guac_mem_free(settings->preferred_video_codec);
    guac_mem_free(settings->port);
    guac_mem_free(settings->tls_port);
    guac_mem_free(settings->username);
    guac_mem_free(settings->password);
    guac_mem_free(settings->ca_file);
    guac_mem_free(settings->cert_subject);
    guac_mem_free(settings->pubkey);
    guac_mem_free(settings->proxy);
    guac_mem_free(settings->preferred_compression);
    guac_mem_free(settings->server_layout);
    guac_mem_free(settings->drive_path);
    guac_mem_free(settings->file_directory);
    guac_mem_free(settings->recording_name);
    guac_mem_free(settings->recording_path);

#ifdef ENABLE_COMMON_SSH
    /* Free SFTP settings */
    guac_mem_free(settings->sftp_directory);
    guac_mem_free(settings->sftp_root_directory);
    guac_mem_free(settings->sftp_host_key);
    guac_mem_free(settings->sftp_hostname);
    guac_mem_free(settings->sftp_passphrase);
    guac_mem_free(settings->sftp_password);
    guac_mem_free(settings->sftp_port);
    guac_mem_free(settings->sftp_private_key);
    guac_mem_free(settings->sftp_public_key);
    guac_mem_free(settings->sftp_username);
#endif

    /* Free Wake-on-LAN strings */
    guac_mem_free(settings->wol_mac_addr);
    guac_mem_free(settings->wol_broadcast_addr);

    /* Free settings structure */
    guac_mem_free(settings);

}
