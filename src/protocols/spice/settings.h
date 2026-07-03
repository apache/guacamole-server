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

#include <guacamole/user.h>

#include <stdbool.h>

/**
 * The protocol label included in the process title (the first argument passed
 * to guac_process_title_set_endpoint()), as seen in `ps`/`top`.
 */
#define GUAC_SPICE_PROCESS_TITLE_NAME "spice"

/**
 * The filename to use for the screen recording, if not specified.
 */
#define GUAC_SPICE_DEFAULT_RECORDING_NAME "recording"

/**
 * The default SPICE server port to connect to if no port is specified.
 */
#define GUAC_SPICE_DEFAULT_PORT 5900

/**
 * The default number of seconds to attempt to connect to the SFTP server.
 */
#define GUAC_SPICE_DEFAULT_SFTP_TIMEOUT 10

/**
 * The default maximum number of bytes to allow within the clipboard.
 */
#define GUAC_SPICE_CLIPBOARD_DEFAULT_BUFFER_SIZE 262144

/**
 * SPICE-specific connection settings, parsed from the arguments provided when
 * a user joins the connection.
 */
typedef struct guac_spice_settings {

    /**
     * The hostname of the SPICE server to connect to.
     */
    char* hostname;

    /**
     * The port of the SPICE server to connect to, used for plaintext
     * connections.
     */
    char* port;

    /**
     * The port of the SPICE server to connect to using TLS, or NULL if TLS is
     * not in use.
     */
    char* tls_port;

    /**
     * The password to use when authenticating with the SPICE server (the SPICE
     * "ticket"), or NULL if no password should be sent.
     */
    char* password;

    /**
     * The username given in the arguments, retained for process-title and
     * logging purposes (SPICE itself authenticates using a ticket/password).
     */
    char* username;

    /**
     * Whether TLS should be used to encrypt the connection. If set, tls_port
     * (or port) is used to establish an encrypted SPICE connection.
     */
    bool tls;

    /**
     * The path to a PEM-encoded certificate authority file used to verify the
     * SPICE server's TLS certificate, or NULL if no CA file was provided.
     */
    char* ca_file;

    /**
     * The expected subject of the SPICE server's TLS certificate, or NULL if
     * the subject should not be explicitly verified.
     */
    char* cert_subject;

    /**
     * Whether verification of the SPICE server's TLS certificate should be
     * disabled. This is typically required for the self-signed certificates
     * used by default by QEMU/libvirt.
     */
    bool ignore_cert;

    /**
     * The base64-encoded (DER) public key the SPICE server's TLS certificate is
     * expected to present, for public-key pinning, or NULL if pinning should
     * not be used.
     */
    char* pubkey;

    /**
     * The proxy server, if any, through which the SPICE connection should be
     * established, or NULL if no proxy should be used.
     */
    char* proxy;

    /**
     * The color depth to request, in bits. SPICE negotiates its own surface
     * format; this value is retained for informational/compatibility purposes.
     */
    int color_depth;

    /**
     * Whether the red and blue color channels of the remote framebuffer should
     * be swapped when copied into the Guacamole display.
     */
    bool swap_red_blue;

    /**
     * The preferred image compression for the SPICE session (one of "off",
     * "auto-glz", "auto-lz", "quic", "glz", "lz", "lz4"), or NULL to let the
     * server decide.
     */
    char* preferred_compression;

    /**
     * The name of the keyboard layout used to translate keysyms into SPICE
     * scancodes (e.g. "en-us-qwerty", "es-es-qwerty", "es-latam-qwerty").
     */
    char* server_layout;

    /**
     * Whether this connection is read-only, and user input should be dropped.
     */
    bool read_only;

    /**
     * Whether the SPICE client should request that the remote display resize
     * to match the client resolution should be disabled.
     */
    bool disable_display_resize;

    /**
     * The maximum number of bytes to allow within the clipboard.
     */
    int clipboard_buffer_size;

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

    /**
     * Whether the clipboard (guest agent) integration should be disabled
     * entirely.
     */
    bool disable_clipboard;

    /**
     * Whether audio playback from the SPICE server should be enabled.
     */
    bool audio_enabled;

    /**
     * Whether audio input (e.g. microphone) from the connected user should be
     * forwarded to the SPICE server via the record channel.
     */
    bool audio_input_enabled;

    /**
     * Whether the Opus audio codec should be disabled. When set, spice-gtk
     * stops advertising Opus, so the SPICE server falls back to its next
     * available audio mode: raw/lossless on modern servers (which no longer
     * support the legacy CELT codec), or CELT on older servers. On a LAN this
     * is a simple way to obtain lossless raw audio, at the cost of bandwidth.
     */
    bool disable_audio_opus;

    /**
     * The video codec the SPICE server should be asked to prefer for streamed
     * video regions ("h264", "vp9", "vp8", or "mjpeg"), or NULL to leave the
     * server's default preference untouched. The server's default lists MJPEG
     * first, so an explicit preference is required for it to use a GStreamer
     * codec. NULL by default because requesting a non-MJPEG codec relies on the
     * server having a working GStreamer encoder for it (some spice-server
     * builds crash when encoding H.264/VP8/VP9).
     */
    char* preferred_video_codec;

    /**
     * Whether folder sharing (shared directory via the SPICE WebDAV channel)
     * should be enabled.
     */
    bool enable_drive;

    /**
     * The local directory to expose to the SPICE server as a shared folder, or
     * NULL if folder sharing is disabled.
     */
    char* drive_path;

    /**
     * Whether the shared folder should be exposed read-only.
     */
    bool drive_read_only;

    /**
     * Whether the Guacamole web-UI file browser (upload/download/ls) over the
     * shared folder should be enabled.
     */
    bool file_transfer;

    /**
     * The host-side path of the shared folder exposed to the file browser, or
     * NULL if file transfer is disabled.
     */
    char* file_directory;

    /**
     * Whether the shared folder should be automatically created if it does not
     * already exist.
     */
    bool file_transfer_create_folder;

    /**
     * Whether the shared folder should be exposed read-only to the SPICE
     * server.
     */
    bool file_transfer_ro;

    /**
     * Whether file downloads from the shared folder to the client should be
     * disabled.
     */
    bool disable_download;

    /**
     * Whether file uploads from the client to the shared folder should be
     * disabled.
     */
    bool disable_upload;

#ifdef ENABLE_COMMON_SSH
    /**
     * Whether SFTP should be enabled for the SPICE connection.
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
     * The number of seconds to attempt to connect to the SFTP server.
     */
    int sftp_timeout;

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
     * The base64-encoded public key to use when authenticating with the SSH
     * server for SFTP using key-based authentication.
     */
    char* sftp_public_key;

    /**
     * The default location for file uploads within the SSH server.
     */
    char* sftp_directory;

    /**
     * The path of the directory within the SSH server to expose as a
     * filesystem guac_object.
     */
    char* sftp_root_directory;

    /**
     * The interval at which SSH keepalive messages are sent to the server for
     * SFTP connections.
     */
    int sftp_server_alive_interval;

    /**
     * Whether file downloads over SFTP should be blocked.
     */
    bool sftp_disable_download;

    /**
     * Whether file uploads over SFTP should be blocked.
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
     * Whether output which is broadcast to each connected client should NOT be
     * included in the session recording.
     */
    bool recording_exclude_output;

    /**
     * Whether changes to mouse state should NOT be included in the session
     * recording.
     */
    bool recording_exclude_mouse;

    /**
     * Whether keys pressed and released should be included in the session
     * recording.
     */
    bool recording_include_keys;

    /**
     * Whether existing files should be appended to when creating a new
     * recording.
     */
    bool recording_write_existing;

    /**
     * Whether or not to send the magic Wake-on-LAN (WoL) packet prior to
     * trying to connect to the remote host.
     */
    bool wol_send_packet;

    /**
     * The MAC address to place in the magic WoL packet to wake the remote
     * host.
     */
    char* wol_mac_addr;

    /**
     * The broadcast address to which to send the magic WoL packet.
     */
    char* wol_broadcast_addr;

    /**
     * The UDP port to use when sending the WoL packet.
     */
    unsigned short wol_udp_port;

    /**
     * The number of seconds after sending the magic WoL packet to wait before
     * attempting to connect to the remote host.
     */
    int wol_wait_time;

} guac_spice_settings;

/**
 * Parses all given args, storing them in a newly-allocated settings object. If
 * the args fail to parse, NULL is returned.
 *
 * @param user
 *     The user who submitted the given arguments while joining the connection.
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

#endif
