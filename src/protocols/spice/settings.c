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

#include "argv.h"
#include "common/clipboard.h"
#include "keymap.h"
#include "settings.h"
#include "spice.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/user.h>
#include <guacamole/wol-constants.h>
#include <spice-client.h>

#include <limits.h>
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
    "recording-include-clipboard",
    "create-recording-path",
    "recording-write-existing",
    "wol-send-packet",
    "wol-mac-addr",
    "wol-broadcast-addr",
    "wol-udp-port",
    "wol-wait-time",
    "preferred-video-codec",
    "file-transfer-mode",
    "file-transfer-max-size",

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
     * Whether clipboard state changes should be included in the screen
     * recording.
     */
    IDX_RECORDING_INCLUDE_CLIPBOARD,

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

    /**
     * How files dragged/uploaded onto the session are transferred to the
     * guest: "none", "agent" (SPICE agent push to the guest Downloads folder,
     * client-to-guest only), "drive" (the bidirectional WebDAV shared folder),
     * or "both" (drive browser for download, agent for the drag-drop upload).
     * When blank, the mode is derived from the legacy "file-transfer" param for
     * backwards compatibility.
     */
    IDX_FILE_TRANSFER_MODE,

    /**
     * The maximum size, in megabytes, of a single file uploaded to the guest
     * via the SPICE agent ("agent" or "both" file-transfer mode). Such uploads
     * are written to a temporary file on the guacd host before being pushed
     * into the guest, so this limits the disk space a single upload may
     * consume on that host. Zero means no limit. If blank, the default of
     * GUAC_SPICE_DEFAULT_MAX_UPLOAD_SIZE megabytes is used.
     */
    IDX_FILE_TRANSFER_MAX_SIZE,

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
        guac_user_parse_args_int_bounded(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_CLIPBOARD_BUFFER_SIZE,
                GUAC_SPICE_CLIPBOARD_DEFAULT_BUFFER_SIZE,
                1, GUAC_COMMON_CLIPBOARD_MAX_LENGTH);

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

    /* Resolve the file-transfer mode, which decides where dragged/uploaded
     * files go. When left blank, fall back to the legacy behaviour: the shared
     * folder browser (if file-transfer was enabled) claimed the upload
     * gesture. */
    char* transfer_mode =
        guac_user_parse_args_string(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER_MODE, NULL);

    if (transfer_mode == NULL || strcmp(transfer_mode, "") == 0)
        settings->file_transfer_mode = settings->file_transfer
                ? GUAC_SPICE_FILE_TRANSFER_DRIVE
                : GUAC_SPICE_FILE_TRANSFER_NONE;
    else if (strcmp(transfer_mode, "agent") == 0)
        settings->file_transfer_mode = GUAC_SPICE_FILE_TRANSFER_AGENT;
    else if (strcmp(transfer_mode, "drive") == 0)
        settings->file_transfer_mode = GUAC_SPICE_FILE_TRANSFER_DRIVE;
    else if (strcmp(transfer_mode, "both") == 0)
        settings->file_transfer_mode = GUAC_SPICE_FILE_TRANSFER_BOTH;
    else if (strcmp(transfer_mode, "none") == 0)
        settings->file_transfer_mode = GUAC_SPICE_FILE_TRANSFER_NONE;
    else {
        guac_user_log(user, GUAC_LOG_WARNING, "Unknown file-transfer-mode "
                "\"%s\"; file transfer disabled.", transfer_mode);
        settings->file_transfer_mode = GUAC_SPICE_FILE_TRANSFER_NONE;
    }

    guac_mem_free(transfer_mode);

    /* Maximum size of a file uploaded via the SPICE agent, in megabytes */
    settings->max_upload_size =
        guac_user_parse_args_int_bounded(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_FILE_TRANSFER_MAX_SIZE, GUAC_SPICE_DEFAULT_MAX_UPLOAD_SIZE,
                0, INT_MAX);

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

    settings->recording_include_clipboard =
        guac_user_parse_args_boolean(user, GUAC_SPICE_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_CLIPBOARD, false);

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
    if (settings->password != NULL)
        explicit_bzero(settings->password, strlen(settings->password));
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
    if (settings->sftp_passphrase != NULL)
        explicit_bzero(settings->sftp_passphrase, strlen(settings->sftp_passphrase));
    guac_mem_free(settings->sftp_passphrase);

    if (settings->sftp_password != NULL)
        explicit_bzero(settings->sftp_password, strlen(settings->sftp_password));
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

void guac_spice_push_settings(guac_client* client, SpiceSession* session) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_settings* settings = spice_client->settings;

    /* Expected certificate subject, if any (may be overridden by a pinned
     * public key, below) */
    const char* cert_subject = settings->cert_subject;

    /* Always configure the target host */
    g_object_set(session, "host", settings->hostname, NULL);

    /* Configure plaintext port, if provided */
    if (settings->port != NULL)
        g_object_set(session, "port", settings->port, NULL);

    /* Connect through a proxy, if one was specified */
    if (settings->proxy != NULL)
        g_object_set(session, "proxy", settings->proxy, NULL);

    /* Configure TLS port and verification policy, if TLS is in use */
    if (settings->tls && settings->tls_port != NULL) {

        /* Certificate-subject matching and public-key pinning are mutually
         * exclusive identity checks (see the verify-policy comment below): only
         * one SPICE_SESSION_VERIFY_* identity flag takes effect. If both are
         * configured, use the stricter public-key pin and ignore the subject. */
        if (!settings->ignore_cert && cert_subject != NULL
                && settings->pubkey != NULL) {
            guac_client_log(client, GUAC_LOG_WARNING, "Both a TLS certificate "
                    "subject and a public key were provided. These are "
                    "mutually exclusive: the public key will be used and the "
                    "certificate subject will be ignored.");
            cert_subject = NULL;
        }

        g_object_set(session, "tls-port", settings->tls_port, NULL);

        /* Provide CA certificate for verification, if given */
        if (settings->ca_file != NULL)
            g_object_set(session, "ca-file", settings->ca_file, NULL);

        /* Provide expected certificate subject, if given */
        if (cert_subject != NULL)
            g_object_set(session, "cert-subject", cert_subject, NULL);

        /* Pin the server's expected public key, if given */
        if (!settings->ignore_cert && settings->pubkey != NULL) {
            gsize pubkey_len = 0;
            guchar* pubkey_der = g_base64_decode(settings->pubkey, &pubkey_len);
            GByteArray* pubkey = g_byte_array_new_take(pubkey_der, pubkey_len);
            g_object_set(session, "pubkey", pubkey, NULL);
            g_byte_array_unref(pubkey);
        }

        /* Determine the certificate verification policy. In spice-gtk each
         * SPICE_SESSION_VERIFY_* flag is an identity check layered on top of
         * mandatory CA-chain validation (see openssl_verify() in
         * spice-common/common/ssl_verify.c): the hostname and subject checks
         * both still require the certificate to chain to a trusted CA, and
         * subject is evaluated in preference to hostname (they are mutually
         * exclusive, not additive). A pinned public key (VERIFY_PUBKEY) is the
         * sole exception — it accepts a self-signed certificate whose key
         * matches without a trusted chain. Self-signed certificates (as used by
         * default by QEMU/libvirt) therefore require either a pinned key or
         * verification disabled entirely (ignore_cert). */
        SpiceSessionVerify verify;
        if (settings->ignore_cert)
            /* Explicit opt-in: no verification at all. */
            verify = 0;
        else if (cert_subject != NULL)
            /* Match the full certificate subject DN on top of CA-chain
             * validation. spice-gtk checks the subject in preference to the
             * hostname, so also requesting VERIFY_HOSTNAME would have no effect;
             * chain validation is enforced regardless. */
            verify = SPICE_SESSION_VERIFY_SUBJECT;
        else if (settings->pubkey != NULL)
            /* Pin the server's public key (exact-key match). Adding
             * VERIFY_HOSTNAME would weaken this by also accepting any other
             * CA-valid certificate for the host, defeating the pin. */
            verify = SPICE_SESSION_VERIFY_PUBKEY;
        else
            /* Default: validate the CA chain and match the hostname. No key is
             * pinned, so VERIFY_PUBKEY must not be set here. */
            verify = SPICE_SESSION_VERIFY_HOSTNAME;

        g_object_set(session, "verify", verify, NULL);

    }

    /* Configure the authentication ticket (password) and username, if
     * provided. These credentials can be replaced at runtime via the argv
     * callback (guac_spice_argv_callback), which frees and reassigns the
     * settings strings under message_lock. Read them under the same lock and
     * copy into locals so a concurrent update cannot free a string mid-read
     * (use-after-free / data race); the copies are then applied to the session
     * without holding the lock and freed afterward. */
    pthread_mutex_lock(&spice_client->message_lock);
    char* password = guac_strdup(settings->password);
    char* username = guac_strdup(settings->username);
    pthread_mutex_unlock(&spice_client->message_lock);

    if (password != NULL)
        g_object_set(session, "password", password, NULL);

    if (username != NULL)
        g_object_set(session, "username", username, NULL);

    guac_mem_free(password);
    guac_mem_free(username);

    /* Configure preferred image compression, if requested */
    if (settings->preferred_compression != NULL) {

        static const struct {
            const char* name;
            SpiceImageCompression value;
        } compression_types[] = {
            { "off",      SPICE_IMAGE_COMPRESSION_OFF      },
            { "auto-glz", SPICE_IMAGE_COMPRESSION_AUTO_GLZ },
            { "auto-lz",  SPICE_IMAGE_COMPRESSION_AUTO_LZ  },
            { "quic",     SPICE_IMAGE_COMPRESSION_QUIC     },
            { "glz",      SPICE_IMAGE_COMPRESSION_GLZ      },
            { "lz",       SPICE_IMAGE_COMPRESSION_LZ       },
            { "lz4",      SPICE_IMAGE_COMPRESSION_LZ4      },
            { NULL,       SPICE_IMAGE_COMPRESSION_INVALID  }
        };

        SpiceImageCompression compression = SPICE_IMAGE_COMPRESSION_INVALID;
        for (int i = 0; compression_types[i].name != NULL; i++) {
            if (strcmp(settings->preferred_compression,
                        compression_types[i].name) == 0) {
                compression = compression_types[i].value;
                break;
            }
        }

        if (compression == SPICE_IMAGE_COMPRESSION_INVALID)
            guac_client_log(client, GUAC_LOG_WARNING, "Ignoring unknown "
                    "preferred image compression \"%s\".",
                    settings->preferred_compression);
        else
            g_object_set(session, "preferred-compression", compression, NULL);

    }

    /* Reflect read-only state to the session */
    g_object_set(session, "read-only", settings->read_only, NULL);

    /* Enable audio negotiation if either playback or input (record) is
     * requested; spice-gtk's "enable-audio" governs both audio channels. */
    g_object_set(session, "enable-audio",
            settings->audio_enabled || settings->audio_input_enabled, NULL);

    /* USB redirection and smartcard passthrough are not meaningful for a
     * headless proxy such as guacd, which has no local physical devices to
     * redirect. Disable both to avoid advertising unsupported channels. */
    g_object_set(session, "enable-usbredir", FALSE, NULL);
    g_object_set(session, "enable-smartcard", FALSE, NULL);

#ifdef ENABLE_SPICE_WEBDAV
    /* Expose shared folder via the SPICE WebDAV channel, if requested. The
     * SPICE client library (spice-gtk) drives the underlying phodav-based
     * WebDAV server automatically once "shared-dir" is set. */
    if (settings->enable_drive && settings->drive_path != NULL) {
        g_object_set(session, "shared-dir", settings->drive_path, NULL);
        g_object_set(session, "share-dir-ro", settings->drive_read_only, NULL);
        guac_client_log(client, GUAC_LOG_INFO,
                "Sharing directory \"%s\" with the SPICE server%s.",
                settings->drive_path,
                settings->drive_read_only ? " (read-only)" : "");
    }
#else
    if (settings->enable_drive)
        guac_client_log(client, GUAC_LOG_WARNING, "Folder sharing was "
                "requested but support for the SPICE WebDAV channel "
                "(libphodav) was not available at build time. Folder sharing "
                "will be disabled.");
#endif

}
