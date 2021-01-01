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

#ifndef SPICE_CONSTANTS_H
#define SPICE_CONSTANTS_H

/**
 * The key used to store and retrieve Guacamole-related data from within the
 * Spice client structure.
 */
#define GUAC_SPICE_CLIENT_KEY "GUAC_SPICE"

/**
 * The default identifier of the pimary/main display.
 */
#define GUAC_SPICE_DEFAULT_DISPLAY_ID 0

/**
 * Error code returned when no more file IDs can be allocated.
 */
#define GUAC_SPICE_FOLDER_ENFILE -1

/**
 * Error code returned when no such file exists.
 */
#define GUAC_SPICE_FOLDER_ENOENT -2

/**
 * Error code returned when the operation required a directory
 * but the file was not a directory.
 */
#define GUAC_SPICE_FOLDER_ENOTDIR -3

/**
 * Error code returned when insufficient space exists to complete
 * the operation.
 */
#define GUAC_SPICE_FOLDER_ENOSPC -4

/**
 * Error code returned when the operation requires a normal file but
 * a directory was given.
 */
#define GUAC_SPICE_FOLDER_EISDIR -5

/**
 * Error code returned when permission is denied.
 */
#define GUAC_SPICE_FOLDER_EACCES -6

/**
 * Error code returned when the operation cannot be completed because the
 * file already exists.
 */
#define GUAC_SPICE_FOLDER_EEXIST -7

/**
 * Error code returned when invalid parameters were given.
 */
#define GUAC_SPICE_FOLDER_EINVAL -8

/**
 * Error code returned when the operation is not implemented.
 */
#define GUAC_SPICE_FOLDER_ENOSYS -9

/**
 * Error code returned when the operation is not supported.
 */
#define GUAC_SPICE_FOLDER_ENOTSUP -10

/**
 * The maximum number of events that can be monitored at a given time for
 * the Spice shared folder Download folder monitor.
 */
#define GUAC_SPICE_FOLDER_MAX_EVENTS 256

/**
 * The maximum length of a path in a shared folder.
 */
#define GUAC_SPICE_FOLDER_MAX_PATH 4096

/**
 * The maximum number of open files in a shared folder.
 */
#define GUAC_SPICE_FOLDER_MAX_FILES 128

/**
 * The maximum level of folder deptch in a shared folder.
 */
#define GUAC_SPICE_FOLDER_MAX_PATH_DEPTH 64

/**
 * The TLS verification value from Guacamole Client that indicates that hostname
 * verification should be done.
 */
#define GUAC_SPICE_PARAMETER_TLS_VERIFY_HOSTNAME "hostname"

/**
 * The TLS verification value from Guacamole Client that indicates that public
 * key verification should be performed.
 */
#define GUAC_SPICE_PARAMETER_TLS_VERIFY_PUBKEY "pubkey"

/**
 * The TLS verification value from Guacamole Client that indicates that subject
 * verification should be performed.
 */
#define GUAC_SPICE_PARAMETER_TLS_VERIFY_SUBJECT "subject"

/**
 * The property within a Spice client channel that indicates if the SPICE
 * agent is connected.
 */
#define SPICE_PROPERTY_AGENT_CONNECTED "agent-connected"

/**
 * The Spice client property that defines CA certificates used to validate
 * the TLS connection to the Spice server.
 */
#define SPICE_PROPERTY_CA "ca"

/**
 * The Spice client property that defines a path on the server running guacd
 * to the file containing the certificate authority certificates to use to
 * validate the TLS connection to the Spice server.
 */
#define SPICE_PROPERTY_CA_FILE "ca-file"

/**
 * The property that the Spice client uses to set the image cache size. If
 * undefined a default of 0 will be used.
 */
#define SPICE_PROPERTY_CACHE_SIZE "cache-size"

/**
 * The Spice client channel property that stores the identifier of the channel.
 */
#define SPICE_PROPERTY_CHANNEL_ID "channel-id"

/**
 * THe Spice client channel property that stores the type of the channel.
 */
#define SPICE_PROPERTY_CHANNEL_TYPE "channel-type"

/**
 * Spice library property that determines whether or not the sockets are provided
 * by the client.
 */
#define SPICE_PROPERTY_CLIENT_SOCKETS "client-sockets"

/**
 * The property that tells the Spice client the color depth to use when
 * allocating new displays.
 */
#define SPICE_PROPERTY_COLOR_DEPTH "color-depth"

/**
 * The property that tells the Spice client to enable audio playback and
 * recording. The SPICE client default is TRUE.
 */
#define SPICE_PROPERTY_ENABLE_AUDIO "enable-audio"

/**
 * Property that enables or disables USB redirection.
 */
#define SPICE_PROPERTY_ENABLE_USBREDIR "enable-usbredir"

/**
 * The property that contains the hostname, IP address, or URL of the Spice
 * server that the client should attempt to connect to.
 */
#define SPICE_PROPERTY_HOST "host"

/**
 * A read-only property exposed by the Spice client library indicating the
 * current state of key modifiers - such as lock keys - on the server.
 */
#define SPICE_PROPERTY_KEY_MODIFIERS "key-modifiers"

/**
 * The property that indicates the minimum latency for audio playback.
 */
#define SPICE_PROPERTY_MIN_LATENCY "min-latency"

/**
 * The property used to toggle the playback and/or record
 * mute status on the Spice server.
 */
#define SPICE_PROPERTY_MUTE "mute"

/**
 * The property used to get or set the number of audio playback and/or recording
 * channels that will be available between the Spice server and client.
 */
#define SPICE_PROPERTY_NUM_CHANNELS "nchannels"

/**
 * The property used to tell the Spice client the password to send on to the
 * SPICE server for authentication.
 */
#define SPICE_PROPERTY_PASSWORD "password"

/**
 * The property used to set the unencrypted communication port for communicating
 * with the Spice server.
 */
#define SPICE_PROPERTY_PORT "port"

/**
 * The property that the Spice client uses to set the proxy server that is used
 * to connect to the Spice server.
 */
#define SPICE_PROPERTY_PROXY "proxy"

/**
 * The property used by the Spice client to tell the server that the session
 * should be read-only.
 */
#define SPICE_PROPERTY_READ_ONLY "read-only"

/**
 * The property that the Spice client uses to determine a local (to guacd)
 * directory that will be shared with the Spice server.
 */
#define SPICE_PROPERTY_SHARED_DIR "shared-dir"

/**
 * The property that tells the Spice client that the shared directory should be
 * read-only to the Spice server and should not allow writes.
 */
#define SPICE_PROPERTY_SHARED_DIR_RO "share-dir-ro"

/**
 * The property within the Spice client that is used to set the port used for
 * secure, TLS-based communication with the Spice server.
 */
#define SPICE_PROPERTY_TLS_PORT "tls-port"

/**
 * The property that is used to set the username that the Spice client will use
 * to authenticate with the server.
 */
#define SPICE_PROPERTY_USERNAME "username"

/**
 * The property that tells the Spiec client whether or not to verify the
 * certificate presented by the Spice server in TLS communications.
 */
#define SPICE_PROPERTY_VERIFY "verify"

/**
 * The property used to get or set the playback and/or recording volume of audio
 * on the Spice server to the remote client.
 */
#define SPICE_PROPERTY_VOLUME "volume"

/**
 * The signal sent by the Spice client when a new channel is created.
 */
#define SPICE_SIGNAL_CHANNEL_NEW "channel-new"

/**
 * The signal sent by the Spice client when a channel is destroyed.
 */
#define SPICE_SIGNAL_CHANNEL_DESTROY "channel-destroy"

/**
 * The signal sent by the Spice client when an event occurs on a channel.
 */
#define SPICE_SIGNAL_CHANNEL_EVENT "channel-event"

/**
 * A signal that indicates that the cursor should be hidden from the display
 * area.
 */
#define SPICE_SIGNAL_CURSOR_HIDE "cursor-hide"

/**
 * A signal that indicates a change in position of the cursor in the display
 * area.
 */
#define SPICE_SIGNAL_CURSOR_MOVE "cursor-move"

/**
 * A signal that indicates the cursor should be reset to its default context.
 */
#define SPICE_SIGNAL_CURSOR_RESET "cursor-reset"

/**
 * A signal sent to modify cursor aspect and position within the display area.
 */
#define SPICE_SIGNAL_CURSOR_SET "cursor-set"

/**
 * The signal sent by the Spice client when the client is disconnected from
 * the server.
 */
#define SPICE_SIGNAL_DISCONNECTED "disconnected"

/**
 * The signal sent to indicate that a region of the display should be updated.
 */
#define SPICE_SIGNAL_DISPLAY_INVALIDATE "display-invalidate"

/**
 * The signal that indicates that a display is ready to be exposed to the client.
 */
#define SPICE_SIGNAL_DISPLAY_MARK "display-mark"

/**
 * The signal indicating when the primary display data/buffer is ready.
 */
#define SPICE_SIGNAL_DISPLAY_PRIMARY_CREATE "display-primary-create"

/**
 * The signal indicating when the primary display surface should be freed and
 * not made available anymore.
 */
#define SPICE_SIGNAL_DISPLAY_PRIMARY_DESTROY "display-primary-destroy"

/**
 * The signal indicating that a rectangular region of he display is updated
 * and should be redrawn.
 */
#define SPICE_SIGNAL_GL_DRAW "gl-draw"

/**
 * The signal sent to indicate that the keyboard modifiers - such as lock keys -
 * have changed and should be updated.
 */
#define SPICE_SIGNAL_INPUTS_MODIFIERS "inputs-modifiers"

/**
 * The signal sent by the Spice client when the connected status or capabilities
 * of a channel change.
 */
#define SPICE_SIGNAL_MAIN_AGENT_UPDATE "main-agent-update"

/**
 * Signal fired by the Spice client when clipboard selection data is available.
 */
#define SPICE_SIGNAL_MAIN_CLIPBOARD_SELECTION "main-clipboard-selection"

/**
 * A signal fired by the Spice client when clipboard selection data is available
 * from the guest, and of what type.
 */
#define SPICE_SIGNAL_MAIN_CLIPBOARD_SELECTION_GRAB "main-clipboard-selection-grab"

/**
 * A signal fired by the Spice client when clipboard selection data is no longer
 * available from the guest.
 */
#define SPICE_SIGNAL_MAIN_CLIPBOARD_SELECTION_RELEASE "main-clipboard-selection-release"

/**
 * A signal used to request clipboard data from the client.
 */
#define SPICE_SIGNAL_MAIN_CLIPBOARD_SELECTION_REQUEST "main-clipboard-selection-request"

/**
 * A signal used to indicate that the mouse mode has changed.
 */
#define SPICE_SIGNAL_MAIN_MOUSE_UPDATE "main-mouse-update"

/**
 * A signal sent by the Spice client when the server has indicated that live
 * migration has started.
 */
#define SPICE_SIGNAL_MIGRATION_STARTED "migration-started"

/**
 * The signal sent by the Spice client when a MM time discontinuity is
 * detected.
 */
#define SPICE_SIGNAL_MM_TIME_RESET "mm-time-reset"

/**
 * The signal fired by the Spice client when a new file transfer task has been
 * initiated.
 */
#define SPICE_SIGNAL_NEW_FILE_TRANSFER "new-file-transfer"

/**
 * The signal fired when data is available to be played on the client, which
 * contains a pointer to the data to be played.
 */
#define SPICE_SIGNAL_PLAYBACK_DATA "playback-data"

/**
 * A signal sent when the server is requesting the current audio playback delay.
 */
#define SPICE_SIGNAL_PLAYBACK_GET_DELAY "playback-get-delay"

/**
 * A signal sent when the server is notifying the client that audio playback
 * should begin, which also contains characteristics of that audio data.
 */
#define SPICE_SIGNAL_PLAYBACK_START "playback-start"

/**
 * A signal sent when audio playback should cease.
 */
#define SPICE_SIGNAL_PLAYBACK_STOP "playback-stop"

/**
 * A signal indicating that the Spice server would like to capture audio data
 * from the client, along with the required format of that data.
 */
#define SPICE_SIGNAL_RECORD_START "record-start"

/**
 * A signal indicating that audio capture should cease.
 */
#define SPICE_SIGNAL_RECORD_STOP "record-stop"

/**
 * A signal indicating that a share folder is available.
 */
#define SPICE_SIGNAL_SHARE_FOLDER "notify::share-folder"

/**
 * The signal indicating that the Spice server has gone to streaming mode.
 */
#define SPICE_SIGNAL_STREAMING_MODE "streaming-mode"

#endif /* SPICE_CONSTANTS_H */

