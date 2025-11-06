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

#include "channels/rdpecam/rdpecam_sink.h"
#include "plugins/guacrdpecam/guacrdpecam.h"
#include "plugins/guacrdpecam/rdpecam_proto.h"
#include "plugins/ptr-string.h"
#include "rdp.h"

#include <freerdp/dvc.h>
#include <freerdp/settings.h>
#include <guacamole/argv.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/user.h>
#include <winpr/stream.h>
#include <winpr/wtsapi.h>
#include <winpr/wtypes.h>
#include <winpr/collections.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>

/**
 * Credits per SampleRequest. Set to 1 to enforce strict request-response
 * behavior. Each SampleRequest from Windows grants exactly 1 credit, and each
 * SampleResponse consumes 1 credit, ensuring 1:1 frame delivery.
 */
#define GUAC_RDPECAM_SAMPLE_CREDITS 1u

#define GUAC_RDPECAM_DEFAULT_WIDTH 640u
#define GUAC_RDPECAM_DEFAULT_HEIGHT 480u
#define GUAC_RDPECAM_DEFAULT_FPS_NUM 30u
#define GUAC_RDPECAM_DEFAULT_FPS_DEN 1u

/**
 * Represents a mapping between a browser device ID and the dynamically
 * assigned Windows channel name. Stored both in the lookup hash table and in
 * a linked list to allow deterministic cleanup of allocated memory.
 */
typedef struct guac_rdp_rdpecam_device_mapping {
    /** Pointer to the browser device ID string used as the hash key. */
    const char* device_id_key;
    /** Copy of the channel name advertised to Windows. */
    char* channel_name;
    /** Next entry in the linked list. */
    struct guac_rdp_rdpecam_device_mapping* next;
} guac_rdp_rdpecam_device_mapping;

static void guac_rdp_rdpecam_mapping_clear(
        guac_rdp_rdpecam_plugin* plugin);
static void guac_rdp_rdpecam_mapping_remove_by_channel(
        guac_rdp_rdpecam_plugin* plugin, const char* channel_name);
static void guac_rdp_rdpecam_mapping_remove_by_device_id(
        guac_rdp_rdpecam_plugin* plugin, const char* device_id);
static BOOL guac_rdp_rdpecam_mapping_add(
        guac_rdp_rdpecam_plugin* plugin, const char* device_id,
        const char* channel_name);

/**
 * Returns true if RDPECAM hexdump logging is enabled. RDPECAM traffic is always
 * dumped to the log (subject to the overall guacd log level filtering).
 *
 * @return
 *     true if hexdump logging should be emitted, false otherwise.
 */
static bool guac_rdp_rdpecam_should_hexdump(void) {
    static bool initialized = false;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("GUAC_RDPECAM_HEXDUMP");
        if (env && *env) {
            if (!strcasecmp(env, "1") || !strcasecmp(env, "true") ||
                !strcasecmp(env, "yes") || !strcasecmp(env, "on"))
                enabled = true;
        }
        initialized = true;
    }
    return enabled;
}


/**
 * Logs a hexadecimal dump of the provided buffer if hexdump logging is
 * enabled. Output roughly matches the format used by winpr_HexDump to aid in
 * side-by-side comparison against FreeRDP traces.
 *
 * @param client
 *     The client whose log handler should receive the output.
 * @param direction
 *     Direction label ("TX" or "RX") to prefix each line, or NULL for none.
 * @param channel_name
 *     The RDPECAM channel name associated with the payload.
 * @param channel_id
 *     The FreeRDP channel identifier.
 * @param data
 *     Pointer to the binary data to dump.
 * @param length
 *     Length of the binary data in bytes.
 */
static void guac_rdp_rdpecam_log_hexdump(guac_client* client, const char* direction,
        const char* channel_name, UINT32 channel_id, const BYTE* data, size_t length) {

    if (!client || !data || length == 0)
        return;

    if (!guac_rdp_rdpecam_should_hexdump())
        return;

    const size_t max_dump = 256;
    const size_t dump_len = (length > max_dump) ? max_dump : length;

    if (length > max_dump) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM %s %s[id=%" PRIu32 "] hexdump length=%zu truncated to %zu bytes",
                direction ? direction : "",
                channel_name ? channel_name : "rdpecam",
                channel_id,
                length,
                dump_len);
    }

    char ascii[17];
    ascii[16] = '\0';

    for (size_t offset = 0; offset < dump_len; offset += 16) {
        const size_t chunk = (dump_len - offset < 16) ? (dump_len - offset) : 16;
        char hexbuf[(16 * 3) + 1];
        size_t pos = 0;

        for (size_t i = 0; i < chunk && pos < sizeof(hexbuf); i++)
            pos += (size_t)snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "%02" PRIx8 " ",
                    data[offset + i]);

        for (size_t i = chunk; i < 16 && pos < sizeof(hexbuf); i++)
            pos += (size_t)snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "   ");

        hexbuf[sizeof(hexbuf) - 1] = '\0';

        for (size_t i = 0; i < chunk; i++)
            ascii[i] = isprint(data[offset + i]) ? (char)data[offset + i] : '.';

        for (size_t i = chunk; i < 16; i++)
            ascii[i] = ' ';

        ascii[16] = '\0';

        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM %s %s[id=%" PRIu32 "] %04" PRIx64 "  %-48s %s",
                direction ? direction : "",
                channel_name ? channel_name : "rdpecam",
                channel_id,
                (uint64_t)offset,
                hexbuf,
                ascii);
    }
}

static void guac_rdp_rdpecam_log_message(guac_client* client, const char* prefix,
        const char* channel_name, UINT32 channel_id, UINT8 cam_msg, size_t payload_len,
        const BYTE* payload) {

    if (!client)
        return;

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM %s %s[id=%" PRIu32 "] msg=0x%02X payload_len=%zu",
            prefix ? prefix : "", channel_name ? channel_name : "rdpecam",
            channel_id, cam_msg, payload_len);

    if (guac_rdp_rdpecam_should_hexdump())
        guac_rdp_rdpecam_log_hexdump(client, prefix, channel_name, channel_id, payload, payload_len);
}

static void guac_rdp_rdpecam_log_stream(guac_client* client, const char* prefix,
        const char* channel_name, UINT32 channel_id, wStream* stream) {

    if (!stream)
        return;

    const size_t length = Stream_Length(stream);
    const BYTE* buffer = Stream_Buffer(stream);
    UINT8 cam_msg = (length >= 2) ? buffer[1] : 0;
    const BYTE* payload = (length >= 2) ? buffer + 2 : NULL;
    const size_t payload_len = (length >= 2) ? (length - 2) : 0;

    guac_rdp_rdpecam_log_message(client, prefix, channel_name, channel_id,
            cam_msg, payload_len, payload);
}

/* Forward declarations for device helper functions (definitions at end of file) */
static guac_rdpecam_device* guac_rdpecam_device_create(
        guac_rdp_rdpecam_plugin* plugin, const char* device_name);
static void guac_rdpecam_device_destroy(guac_rdpecam_device* device);
static guac_rdpecam_device* guac_rdpecam_device_lookup(
        guac_rdp_rdpecam_plugin* plugin, const char* device_name);
static guac_rdp_rdpecam_device_caps* guac_rdp_rdpecam_get_device_caps(
        guac_rdp_client* rdp_client, const char* channel_name);
static UINT guac_rdp_rdpecam_new_connection(
        IWTSListenerCallback* listener_callback, IWTSVirtualChannel* channel,
        BYTE* data, int* accept,
        IWTSVirtualChannelCallback** channel_callback);

/**
 * Invoked when RDPECAM capabilities have been updated on the core side.
 * If the plugin is ready (version negotiated) and the enumerator channel is
 * known, immediately sends DeviceAddedNotification for all devices.
 * For capability updates, also removes devices that are no longer in the list.
 */
void guac_rdp_rdpecam_caps_notify(guac_client* client) {
    if (!client)
        return;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    if (!rdp_client)
        return;
    guac_rdp_rdpecam_plugin* plugin = rdp_client->rdpecam_plugin;
    if (!plugin || !plugin->version_negotiated || !plugin->enumerator_channel)
        return;

    guac_rwlock_acquire_write_lock(&(rdp_client->lock));

    if (!rdp_client->rdpecam_caps_updated) {
        guac_rwlock_release_lock(&(rdp_client->lock));
        return;
    }

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM caps_notify: processing capability update");

    /* Build set of new device IDs from capabilities */
    char* new_device_ids[GUAC_RDP_RDPECAM_MAX_DEVICES] = {NULL};
    unsigned int new_device_count = rdp_client->rdpecam_device_caps_count;

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM caps_notify: new capability count = %u", new_device_count);

    for (unsigned int i = 0; i < new_device_count; i++) {
        guac_rdp_rdpecam_device_caps* caps = &rdp_client->rdpecam_device_caps[i];
        if (caps->device_id) {
            new_device_ids[i] = caps->device_id;
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM caps_notify: new device[%u] = '%s'", i, caps->device_id);
        }
    }

    /* Since HashTable_GetKeys is broken in WinPR, we can't reliably iterate device_id_map
     * to find what needs to be removed. Instead, we'll:
     * 1. Scan all channel slots to find channels that should be removed (not in new capabilities)
     * 2. Send removal notifications for those channels (whether Windows opened them or not)
     * 3. Clear device_id_map completely
     * 4. Rebuild it from new capabilities
     * This avoids the HashTable_GetKeys API compatibility issue entirely. */

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM caps_notify: removing all previously advertised channels before rebuild");

    /* Step 1: Since we're about to clear and rebuild device_id_map, send DeviceRemovedNotification
     * for ALL channel slots (0-10) to ensure Windows cleans up any previously advertised devices.
     * Windows will ignore removals for channels that were never advertised. */

    char channels_to_remove[11][64];  /* Slots 0-10 should cover most reasonable scenarios */
    unsigned int remove_count = 0;

    for (unsigned int slot = 0; slot <= 10 && slot < GUAC_RDP_RDPECAM_MAX_DEVICES; slot++) {
        snprintf(channels_to_remove[remove_count], sizeof(channels_to_remove[remove_count]),
                "RDCamera_Device_%u", slot);
        remove_count++;
    }

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM caps_notify: will send removal for slots 0-%u to clean up old advertisements",
            remove_count - 1);

    /* Step 2: Send removal notifications for all slots */
    for (unsigned int i = 0; i < remove_count; i++) {
        char* channel_name = channels_to_remove[i];

        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM sending removal for channel '%s'", channel_name);

        /* Send DeviceRemovedNotification to Windows */
        wStream* rs = Stream_New(NULL, 256);
        if (rs && rdpecam_build_device_removed(rs, channel_name)) {
            Stream_SealLength(rs);
            const size_t out_len = Stream_Length(rs);

            UINT32 enum_channel_id = 0;
            if (plugin->manager && plugin->manager->GetChannelId)
                enum_channel_id = plugin->manager->GetChannelId(plugin->enumerator_channel);

            pthread_mutex_lock(&(rdp_client->message_lock));
            UINT result = plugin->enumerator_channel->Write(plugin->enumerator_channel,
                    (UINT32) out_len, Stream_Buffer(rs), NULL);
            pthread_mutex_unlock(&(rdp_client->message_lock));

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x06 DeviceRemovedNotification (channel='%s') result=%u",
                    enum_channel_id, channel_name, result);
        }
        if (rs) Stream_Free(rs, TRUE);

        /* Clean up device structure */
        guac_rdpecam_device* device = (guac_rdpecam_device*)
            HashTable_GetItemValue(plugin->devices, channel_name);

        if (device) {
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM cleaning up device structure for channel '%s'", channel_name);

            pthread_mutex_lock(&device->lock);
            device->stopping = true;
            device->streaming = false;
            pthread_cond_broadcast(&device->credits_signal);
            pthread_mutex_unlock(&device->lock);

            /* Remove from devices hash table */
            HashTable_Remove(plugin->devices, channel_name);

            /* Remove associated browser mapping */
            guac_rdp_rdpecam_mapping_remove_by_channel(plugin, channel_name);

            /* Destroy device resources (threads, sinks, etc.) */
            guac_rdpecam_device_destroy(device);
        }
        else {
            /* Ensure any lingering mapping for this channel is removed */
            guac_rdp_rdpecam_mapping_remove_by_channel(plugin, channel_name);
        }

        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM caps_notify: completed removal notification for channel '%s'", channel_name);
    }

    /* Step 3: Clear and rebuild device_id_map to avoid stale entries */
    guac_rdp_rdpecam_mapping_clear(plugin);

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM caps_notify: starting device addition phase");

    /* Now send DeviceAddedNotification ONLY for NEW devices (not already in device_id_map) */
    if (new_device_count > 0) {
        unsigned int added_count = 0;

        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM caps_notify: processing %u potential new devices", new_device_count);

        for (unsigned int i = 0; i < new_device_count; i++) {
            guac_rdp_rdpecam_device_caps* caps = &rdp_client->rdpecam_device_caps[i];

            /* Find next available channel index (device_id_map was just cleared, so all devices need assignment) */
            char channel_name[64];
            unsigned int assigned_channel_idx = 0;
            int found_slot = 0;

            /* Find the next available RDCamera_Device_N slot */
            for (unsigned int check_idx = 0; check_idx < 100 && !found_slot; check_idx++) {
                snprintf(channel_name, sizeof(channel_name), "RDCamera_Device_%u", check_idx);

                /* Check if this channel name is already in use */
                int in_use = 0;

                /* Check plugin->devices (may be empty if Windows hasn't opened channels yet) */
                if (plugin->devices) {
                    guac_rdpecam_device* existing_device = (guac_rdpecam_device*)
                        HashTable_GetItemValue(plugin->devices, channel_name);
                    if (existing_device) {
                        in_use = 1;
                    }
                }

                /* Also check device_id_map to see if any device already maps to this channel */
                if (!in_use && plugin->device_id_map) {
                    /* Check each new device we're processing to see if it maps to this channel */
                    for (unsigned int j = 0; j < new_device_count; j++) {
                        if (!new_device_ids[j])
                            continue;

                        guac_rdp_rdpecam_device_mapping* mapping_entry =
                            (guac_rdp_rdpecam_device_mapping*) HashTable_GetItemValue(
                                plugin->device_id_map, new_device_ids[j]);
                        if (mapping_entry && mapping_entry->channel_name
                                && strcmp(mapping_entry->channel_name, channel_name) == 0) {
                            in_use = 1;
                            break;
                        }
                    }
                }

                if (!in_use) {
                    assigned_channel_idx = check_idx;
                    found_slot = 1;
                }
            }

            if (!found_slot) {
                guac_client_log(client, GUAC_LOG_ERROR,
                        "RDPECAM no available channel slots for new device");
                continue;
            }

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM assigning new device '%s' to channel '%s'",
                    caps->device_id, channel_name);

            const char* device_name = "Redirected-Cam0";
            char fallback_name[64];
            if (caps->device_name && caps->device_name[0] != '\0') {
                device_name = caps->device_name;
            } else {
                snprintf(fallback_name, sizeof(fallback_name), "Redirected-Cam%u", i);
                device_name = fallback_name;
            }

            /* Store device ID to channel name mapping */
            if (caps->device_id && caps->device_id[0] != '\0' && plugin->device_id_map) {
                if (!guac_rdp_rdpecam_mapping_add(plugin, caps->device_id, channel_name)) {
                    guac_client_log(client, GUAC_LOG_ERROR,
                            "RDPECAM failed to record device mapping for '%s'", caps->device_id);
                } else {
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM mapped device ID '%s' to channel '%s'",
                            caps->device_id, channel_name);
                }
            }

            /* Create listener for this device channel if not Device_0 */
            if (assigned_channel_idx > 0 && plugin->manager) {
                guac_rdp_rdpecam_listener_callback* device_listener =
                    guac_mem_zalloc(sizeof(guac_rdp_rdpecam_listener_callback));
                if (device_listener) {
                    char* saved_channel_name = guac_mem_alloc(strlen(channel_name) + 1);
                    if (saved_channel_name) {
                        strcpy(saved_channel_name, channel_name);
                        device_listener->client = client;
                        device_listener->channel_name = saved_channel_name;
                        device_listener->plugin = plugin;
                        device_listener->parent.OnNewChannelConnection = guac_rdp_rdpecam_new_connection;

                        plugin->manager->CreateListener(plugin->manager, channel_name, 0,
                                (IWTSListenerCallback*) device_listener, NULL);

                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM registered listener for device channel: %s", channel_name);
                    } else {
                        guac_mem_free(device_listener);
                    }
                }
            }

            /* Send DeviceAddedNotification */
            wStream* rs = Stream_New(NULL, 256);
            if (rs && rdpecam_build_device_added(rs, device_name, channel_name)) {
                Stream_SealLength(rs);
                const size_t out_len = Stream_Length(rs);

                UINT32 enum_channel_id = 0;
                if (plugin->manager && plugin->manager->GetChannelId)
                    enum_channel_id = plugin->manager->GetChannelId(plugin->enumerator_channel);

                pthread_mutex_lock(&(rdp_client->message_lock));
                UINT result = plugin->enumerator_channel->Write(plugin->enumerator_channel,
                        (UINT32) out_len, Stream_Buffer(rs), NULL);
                pthread_mutex_unlock(&(rdp_client->message_lock));

                guac_client_log(client, GUAC_LOG_DEBUG,
                        "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x05 DeviceAddedNotification (device='%s', channel='%s')",
                        enum_channel_id, device_name, channel_name);

                added_count++;
            }
            if (rs) Stream_Free(rs, TRUE);
        }

        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM capability update: added %u new device(s)", added_count);
    } else {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM all cameras disabled");
    }

    rdp_client->rdpecam_caps_updated = 0;
    guac_rwlock_release_lock(&(rdp_client->lock));

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM caps_notify: completed capability update processing");
}

/**
 * Parameters describing the camera stream announced to the browser owner via
 * argv instructions.
 */
typedef struct guac_rdp_camera_start_params {
    uint32_t width;
    uint32_t height;
    uint32_t fps_numerator;
    uint32_t fps_denominator;
    uint8_t stream_index;
    /** Optional browser device ID to target a specific camera */
    const char* device_id;
} guac_rdp_camera_start_params;

/**
 * Invoked for the owner user when Windows requests streaming. Informs the
 * browser which resolution, frame rate, and stream index to use.
 *
 * @param user
 *     The user receiving the camera start signal.
 *
 * @param data
 *     Pointer to a guac_rdp_camera_start_params structure containing the
 *     stream parameters.
 *
 * @return
 *     Always NULL.
 */
static void* guac_rdp_rdpecam_send_camera_start_signal_callback(guac_user* user, void* data) {

    if (user == NULL)
        return NULL;

    guac_rdp_camera_start_params* params = (guac_rdp_camera_start_params*) data;
    guac_socket* socket = user->socket;

    /* Send concise string form always including deviceId (may be empty):
     * WIDTHxHEIGHT@FPS_NUM/FPS_DEN#STREAM_INDEX#DEVICE_ID
     */
    char concise[512];
    const char* device_id_str = (params->device_id) ? params->device_id : "";
    int concise_written = snprintf(concise, sizeof(concise), "%ux%u@%u/%u#%u#%s",
            params->width,
            params->height,
            params->fps_numerator,
            params->fps_denominator,
            params->stream_index,
            device_id_str);
    if (concise_written > 0 && (size_t) concise_written < sizeof(concise)) {
        guac_user_stream_argv(user, socket, "text/plain", "camera-start", concise);
        guac_socket_flush(socket); /* Reduce latency delivering the start signal. */
    }

    return NULL;
}

/**
 * Invoked for the owner user when Windows stops streaming. Signals the
 * browser to release its capture pipeline.
 *
 * @param user
 *     The user receiving the camera stop signal.
 *
 * @param data
 *     Ignored.
 *
 * @return
 *     Always NULL.
 */
static void* guac_rdp_rdpecam_send_camera_stop_signal_callback(guac_user* user, void* data) {

    if (user == NULL)
        return NULL;

    guac_socket* socket = user->socket;
    guac_user_stream_argv(user, socket, "text/plain", "camera-stop", "");
    guac_socket_flush(socket);

    return NULL;
}

/**
 * Dequeue thread entry point. Continuously pops frames from the rdpecam sink
 * and sends them to the RDP client via the RDPECAM protocol.
 *
 * @param arg
 *     Pointer to the guac_rdpecam_device structure for per-device frame processing.
 *
 * @return
 *     NULL on success.
 */
static void* guac_rdp_rdpecam_dequeue_thread(void* arg) {

    guac_rdpecam_device* device = (guac_rdpecam_device*) arg;
    if (!device || !device->sink) {
        return NULL;
    }
    
    guac_client* client = NULL;
    guac_rdpecam_sink* sink = device->sink;
    
    /* For logging, we need the client - get it from sink if available */
    if (sink && sink->client) {
        client = sink->client;
    }
    
    if (!client) {
        return NULL;
    }

    guac_client_log(client, GUAC_LOG_DEBUG,
        "RDPECAM dequeue thread started for device: %s", device->device_name);
    
    uint32_t frames_processed = 0;
    uint32_t frames_dropped = 0;
    uint32_t last_stats_time = 0;

    while (true) {

        /* Lock mutex to check state and wait on condition variable if needed. */
        pthread_mutex_lock(&device->lock);

        /* Check if we should stop */
        if (device->stopping) {
            pthread_mutex_unlock(&device->lock);
            break;
        }

        /* Wait for channel to be available */
        while (!device->stream_channel && !device->stopping) {
            pthread_cond_wait(&device->credits_signal, &device->lock);
        }
        if (device->stopping) {
            pthread_mutex_unlock(&device->lock);
            break;
        }

        /* Wait for streaming to start */
        while (!device->streaming && !device->stopping) {
            pthread_cond_wait(&device->credits_signal, &device->lock);
        }
        if (device->stopping) {
            pthread_mutex_unlock(&device->lock);
            break;
        }

        /* Wait for active sender role */
        while (!device->is_active_sender && !device->stopping) {
            pthread_cond_wait(&device->credits_signal, &device->lock);
        }
        if (device->stopping) {
            pthread_mutex_unlock(&device->lock);
            break;
        }

        /* Wait for credits to be available */
        while (device->credits == 0 && !device->stopping) {
            pthread_cond_wait(&device->credits_signal, &device->lock);
        }
        if (device->stopping) {
            pthread_mutex_unlock(&device->lock);
            break;
        }

        /* Snapshot state for use after unlocking */
        bool is_streaming = device->streaming;
        bool is_active_sender = device->is_active_sender;
        uint32_t available_credits = device->credits;
        IWTSVirtualChannel* current_channel = device->stream_channel;
        pthread_mutex_unlock(&device->lock);

        /* We have credits; attempt to pull a frame from the shared sink. */
        uint8_t* frame_data = NULL;
        size_t frame_length = 0;
        bool keyframe = false;
        uint32_t pts_ms = 0;
        if (!guac_rdpecam_pop(sink, &frame_data, &frame_length, &keyframe, &pts_ms)) {
            /* No frame available or stopping */
            if (device->stopping) {
                break;
            }
            continue;
        }

        /* Validate frame data */
        if (!frame_data || frame_length == 0) {
            guac_client_log(client, GUAC_LOG_WARNING, "RDPECAM received invalid frame data");
            if (frame_data) {
                guac_mem_free(frame_data);
            }
            continue;
        }

        bool stop_requested;
        bool stream_active;
        bool channel_available;
        bool waiting_for_keyframe;
        pthread_mutex_lock(&device->lock);
        stop_requested = device->stopping;
        stream_active = device->streaming;
        channel_available = (device->stream_channel != NULL);
        waiting_for_keyframe = device->need_keyframe;
        bool allow_send = !stop_requested && stream_active && channel_available &&
                (!waiting_for_keyframe || keyframe);
        uint32_t stream_idx = device->stream_index;
        uint32_t sample_seq = device->sample_sequence;
        IWTSVirtualChannel* active_channel = device->stream_channel;
        if (allow_send) {
            device->sample_sequence++;
        }
        pthread_mutex_unlock(&device->lock);

        if (!allow_send) {
            if (!stream_active) {
                guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM dropping frame - streaming not active");
            }
            else if (!channel_available) {
                guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM dropping frame - channel unavailable");
            }
            else if (waiting_for_keyframe && !keyframe) {
                guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM dropping P-frame - waiting for keyframe to start stream");
            }
            else if (stop_requested) {
                guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM dropping frame - device stopping");
            }
            guac_mem_free(frame_data);
            frames_dropped++;
            continue;
        }

        /* Build RDPECAM sample (placeholder header + payload) */

        wStream* s = Stream_New(NULL, frame_length + 64);
        if (s && rdpecam_write_sample_response_header(s,
                    /*streamId*/ stream_idx,
                    /*sampleSequence*/ sample_seq,
                    (uint32_t) frame_length,
                    /*pts in HNS*/ ((uint64_t) pts_ms) * 10000ULL)) {
            /* Log pts conversion for early frames */
            if (frames_processed < 8) {
                uint64_t pts_hns = ((uint64_t) pts_ms) * 10000ULL;
                guac_client_log(client, GUAC_LOG_DEBUG,
                        "RDPECAM TX frame: pts_ms=%u -> pts_hns=%" PRIu64,
                        pts_ms, pts_hns);
            }
            Stream_Write(s, frame_data, frame_length);
            const size_t sample_len = Stream_GetPosition(s);
            uint32_t log_channel_id = 0;
            pthread_mutex_lock(&device->lock);
            log_channel_id = device->stream_channel_id;
            pthread_mutex_unlock(&device->lock);
            guac_rdp_rdpecam_log_stream(client, "TX",
                    device->device_name, log_channel_id, s);
            
            /* Use message_lock to prevent blocking the RDP event loop */
            guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
            pthread_mutex_lock(&(rdp_client->message_lock));
            UINT result = active_channel->Write(active_channel, (UINT32) sample_len, Stream_Buffer(s), NULL);
            pthread_mutex_unlock(&(rdp_client->message_lock));
            
            if (result == CHANNEL_RC_OK) {
                /* Decrement credits atomically and log transition (per-device) */
                pthread_mutex_lock(&device->lock);
                uint32_t before = device->credits;
                if (device->credits > 0) device->credits--;
                uint32_t remaining = device->credits;
                if (keyframe && device->need_keyframe)
                    device->need_keyframe = false;
                pthread_mutex_unlock(&device->lock);

                frames_processed++;
                
                guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM frame sent: %zu bytes, keyframe=%s, pts=%u ms, credits %u->%u",
                               frame_length, keyframe ? "yes" : "no", pts_ms, before, remaining);
                
            } else {
                /* DVC Write failed - log detailed information */
                guac_client_log(client, GUAC_LOG_WARNING, 
                    "RDPECAM DVC Write FAILED: size=%zu, result=0x%08X, frame=%zu, keyframe=%s", 
                    sample_len, result, frames_processed + 1, 
                    keyframe ? "yes" : "no");
                
                frames_dropped++;
                
                /* If channel write fails, we might need to stop streaming */
                if (result != CHANNEL_RC_OK) {
                    guac_client_log(client, GUAC_LOG_ERROR, 
                        "RDPECAM channel write failed (code=0x%08X), stopping streaming", result);
                    pthread_mutex_lock(&device->lock);
                    device->streaming = false;
                    device->is_active_sender = false;
                    pthread_cond_broadcast(&device->credits_signal);
                    pthread_mutex_unlock(&device->lock);
                    pthread_mutex_lock(&sink->lock);
                    sink->streaming = false;
                    sink->has_active_sender = false;
                    sink->active_sender_channel = NULL;
                    pthread_mutex_unlock(&sink->lock);
                }
            }
            Stream_Free(s, TRUE);
        }
        else {
            if (s) Stream_Free(s, TRUE);
            guac_client_log(client, GUAC_LOG_ERROR, "RDPECAM failed to build sample header");
        }

        /* Free frame data */
        guac_mem_free(frame_data);
        
        /* Log performance statistics every 100 frames or 30 seconds */
        uint32_t current_time = (uint32_t) time(NULL);
        if (frames_processed % 100 == 0 || (current_time - last_stats_time) >= 30) {
            uint32_t total_frames = frames_processed + frames_dropped;
            float drop_rate = total_frames > 0 ? (float)frames_dropped * 100.0f / total_frames : 0.0f;
            
            /* Log device credits (per-device) */
            pthread_mutex_lock(&device->lock);
            uint32_t device_credits_log = device->credits;
            pthread_mutex_unlock(&device->lock);
            guac_client_log(client, GUAC_LOG_INFO, "RDPECAM performance stats: device=%s, processed=%u, dropped=%u, drop_rate=%.1f%%, credits=%u, queue=%d/%d", 
                           device->device_name, frames_processed, frames_dropped, drop_rate, device_credits_log, 
                           guac_rdpecam_get_queue_size(sink), GUAC_RDPECAM_MAX_FRAMES);
            
            last_stats_time = current_time;
        }
    }

    /* Log final statistics */
    uint32_t total_frames = frames_processed + frames_dropped;
    float drop_rate = total_frames > 0 ? (float)frames_dropped * 100.0f / total_frames : 0.0f;
    guac_client_log(client, GUAC_LOG_INFO, "RDPECAM final stats for device=%s: processed=%u, dropped=%u, drop_rate=%.1f%%", 
                   device->device_name, frames_processed, frames_dropped, drop_rate);

    guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM dequeue thread stopped for device: %s", device->device_name);
    
    /* Device cleanup is handled by device_destroy via hash table destructor.
     * Thread is detached and will exit naturally when device->stopping is set. */
    
    return NULL;

}

/**
 * Processes a single RDPECAM protocol message delivered by FreeRDP. The
 * provided stream is positioned at the start of the message payload (after
 * the message header) and must be fully consumed by the handler.
 *
 * @param client
 *     The guac_client associated with the RDP session.
 *
 * @param channel
 *     The FreeRDP dynamic virtual channel on which the message was received.
 *
 * @param stream
 *     The WinPR stream containing the message payload.
 *
 * @param rdpecam_channel_callback
 *     Callback context describing the channel and associated device state.
 *
 * @return
 *     CHANNEL_RC_OK on success, or a FreeRDP CHANNEL_RC_* error code.
 */
static UINT guac_rdp_rdpecam_handle_data(guac_client* client, IWTSVirtualChannel* channel,
        wStream* stream, guac_rdp_rdpecam_channel_callback* rdpecam_channel_callback) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    
    /* Resolve per-channel context supplied by FreeRDP. */
    guac_rdpecam_device* device = rdpecam_channel_callback ? rdpecam_channel_callback->device : NULL;
    guac_rdp_rdpecam_plugin* plugin = rdpecam_channel_callback ? rdpecam_channel_callback->plugin : NULL;
    const char* ch_name = rdpecam_channel_callback ? rdpecam_channel_callback->channel_name : "unknown";
    UINT32 channel_id = rdpecam_channel_callback ? rdpecam_channel_callback->channel_id : 0;

    /* Get remaining data from current stream position (FreeRDP has already consumed any framing) */
    size_t data_length = Stream_GetRemainingLength(stream);
    
    if (data_length < 2) {
        guac_client_log(client, GUAC_LOG_WARNING, "RDPECAM message too short: %zu bytes (expected at least 2 for header)", data_length);
        return CHANNEL_RC_OK;
    }

    /* Log full payload prior to parsing */
    const BYTE* raw = Stream_Pointer(stream);

    /* Read MS-RDPECAM protocol header: [Version:1][MessageId:1] */
    BYTE version, cam_msg;
    Stream_Read_UINT8(stream, version);
    Stream_Read_UINT8(stream, cam_msg);

    size_t payload_len = (data_length >= 2) ? (data_length - 2) : 0;
    const BYTE* payload_ptr = Stream_Pointer(stream);
    guac_rdp_rdpecam_log_message(client, "RX", ch_name, channel_id, cam_msg,
            payload_len, payload_ptr);

    guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM RX message on %s[id=%" PRIu32 "]: version=%d, cam_msg=0x%02x, payload_len=%zu", 
                    ch_name, channel_id, version, cam_msg, data_length - 2);

    /* Verify protocol version */
    if (version != RDPECAM_PROTO_VERSION) {
        guac_client_log(client, GUAC_LOG_WARNING, 
            "RDPECAM received message with unexpected version: expected 0x%02x, got 0x%02x",
            RDPECAM_PROTO_VERSION, version);
        return CHANNEL_RC_OK;
    }
    
    /* Process message based on message ID */
    {
        /* Stream is now positioned at the start of the payload (after version and messageId) */
        UINT result = CHANNEL_RC_OK;
        wStream* rs = NULL;

        switch (cam_msg) {
            case RDPECAM_MSG_SELECT_VERSION_RESPONSE: {
                /* Server accepted our version request */
                guac_client_log(client, GUAC_LOG_DEBUG,
                        "RDPECAM RX ChannelId=%" PRIu32 " MessageId=0x04 SelectVersionResponse (version=%d)",
                        channel_id, RDPECAM_PROTO_VERSION);
                
                /* Mark version negotiation as complete */
                if (plugin) {
                    plugin->version_negotiated = true;
                }
                
                /* Store enumerator channel reference for later use */
                if (plugin && !strcasecmp(ch_name, GUAC_RDPECAM_CHANNEL_NAME)) {
                    plugin->enumerator_channel = channel;
                }
                
                /* If devices are already available, send notifications now */
                guac_rwlock_acquire_read_lock(&(rdp_client->lock));
                unsigned int device_count = rdp_client->rdpecam_device_caps_count;
                guac_rwlock_release_lock(&(rdp_client->lock));
                
                if (device_count > 0 && plugin && plugin->enumerator_channel) {
                    guac_rwlock_acquire_write_lock(&(rdp_client->lock));
                    guac_rdp_rdpecam_send_device_notifications(plugin, client, rdp_client, plugin->enumerator_channel);
                    rdp_client->rdpecam_caps_updated = 0; /* Clear the flag */
                    guac_rwlock_release_lock(&(rdp_client->lock));
                } else {
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM version negotiated, waiting for device capabilities");
                }
                break;
            }

            case RDPECAM_MSG_ACTIVATE_DEVICE_REQUEST: {
                guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM received ActivateDeviceRequest on %s[id=%" PRIu32 "]", ch_name, channel_id);
                
                if (!strcasecmp(ch_name, GUAC_RDPECAM_CHANNEL_NAME)) {
                    rs = Stream_New(NULL, 8);
                    if (rs && rdpecam_build_success_response(rs)) {
                        Stream_SealLength(rs);
                        const size_t out_len = Stream_Length(rs);
                        guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                        pthread_mutex_lock(&(rdp_client->message_lock));
                        result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                        pthread_mutex_unlock(&(rdp_client->message_lock));
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x01 SuccessResponse (ActivateDevice control)",
                                channel_id);
                    }
                } else {
                    if (!device) {
                        guac_client_log(client, GUAC_LOG_WARNING,
                                "RDPECAM ActivateDevice on device channel but no device available (ChannelId=%" PRIu32 ")",
                                channel_id);
                        return CHANNEL_RC_OK;
                    }
                    
                    rs = Stream_New(NULL, 8);
                    if (rs && rdpecam_build_success_response(rs)) {
                        Stream_SealLength(rs);
                        const size_t out_len = Stream_Length(rs);
                        guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                        pthread_mutex_lock(&(rdp_client->message_lock));
                        result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                        pthread_mutex_unlock(&(rdp_client->message_lock));
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x01 SuccessResponse (ActivateDevice device=%s)",
                                channel_id, device->device_name);
                    }
                }
                break;
            }

            case RDPECAM_MSG_DEACTIVATE_DEVICE_REQUEST: {
                guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM received DeactivateDeviceRequest on %s[id=%" PRIu32 "]", ch_name, channel_id);
                
                if (!strcasecmp(ch_name, GUAC_RDPECAM_CHANNEL_NAME)) {
                    rs = Stream_New(NULL, 8);
                    if (rs && rdpecam_build_success_response(rs)) {
                        Stream_SealLength(rs);
                        const size_t out_len = Stream_Length(rs);
                        guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                        pthread_mutex_lock(&(rdp_client->message_lock));
                        result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                        pthread_mutex_unlock(&(rdp_client->message_lock));
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x01 SuccessResponse (DeactivateDevice control)",
                                channel_id);
                    }
                } else {
                    if (!device || !device->sink) {
                        guac_client_log(client, GUAC_LOG_WARNING,
                                "RDPECAM DeactivateDevice on device channel but no device/sink available (ChannelId=%" PRIu32 ")",
                                channel_id);
                        return CHANNEL_RC_OK;
                    }

                    bool same_stream_channel = (device->stream_channel == channel) || rdpecam_channel_callback->is_stream_channel;
                    if (!same_stream_channel) {
                        rs = Stream_New(NULL, 8);
                        if (rs && rdpecam_build_success_response(rs)) {
                            Stream_SealLength(rs);
                            const size_t out_len = Stream_Length(rs);
                            guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                            pthread_mutex_lock(&(rdp_client->message_lock));
                            result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                            pthread_mutex_unlock(&(rdp_client->message_lock));
                            guac_client_log(client, GUAC_LOG_DEBUG,
                                    "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x01 SuccessResponse (DeactivateDevice property device=%s)",
                                    channel_id, device->device_name);
                        }
                        break;
                    }

                    rdpecam_channel_callback->is_stream_channel = true;

                    /* Stop streaming if active (per-device) */
                    uint32_t outstanding = 0;
                    uint32_t stream_index = 0;
                    bool was_active_sender = false;
                    pthread_mutex_lock(&device->lock);
                    outstanding = device->credits;
                    stream_index = device->stream_index;
                    was_active_sender = device->is_active_sender;
                    device->credits = 0;
                    device->streaming = false;
                    device->is_active_sender = false;
                    device->need_keyframe = true;
                    pthread_cond_broadcast(&device->credits_signal);
                    pthread_mutex_unlock(&device->lock);

                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM DeactivateDevice device=%s was_active_sender=%s",
                            device->device_name, was_active_sender ? "true" : "false");

                    guac_rdpecam_sink* sink = device->sink;

                    /* Only clear this device's sink state if it was the active sender.
                     * If this device was never streaming, its sink state is already clean.
                     * Only the active sender has sink state that needs to be cleared. */
                    if (was_active_sender) {
                        pthread_mutex_lock(&sink->lock);
                        sink->streaming = false;
                        sink->credits = 0;
                        sink->has_active_sender = false;
                        sink->active_sender_channel = NULL;
                        pthread_mutex_unlock(&sink->lock);

                        /* Clear the browser's frame push target to prevent race condition where
                         * in-flight frames arrive after deactivation but before channel close.
                         * The blob handler will safely drop frames when rdpecam_sink is NULL. */
                        if (rdp_client->rdpecam_sink == sink)
                            rdp_client->rdpecam_sink = NULL;

                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM cleared shared sink state for device=%s",
                                device->device_name);
                    } else {
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM NOT clearing shared sink state (device=%s was not active sender)",
                                device->device_name);
                    }

                    pthread_mutex_lock(&device->lock);
                    device->stream_channel = NULL;
                    pthread_mutex_unlock(&device->lock);

                    /* Send error responses for outstanding credits */
                    for (uint32_t i = 0; i < outstanding; i++) {
                        wStream* es = Stream_New(NULL, 8);
                        if (es && rdpecam_build_sample_error_response(es, stream_index)) {
                            Stream_SealLength(es);
                            const size_t err_len = Stream_Length(es);
                            guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, es);
                            pthread_mutex_lock(&(rdp_client->message_lock));
                            channel->Write(channel, (UINT32) err_len, Stream_Buffer(es), NULL);
                            pthread_mutex_unlock(&(rdp_client->message_lock));
                            guac_client_log(client, GUAC_LOG_DEBUG,
                                    "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x13 SampleErrorResponse (stream=%u)",
                                    channel_id, stream_index);
                        }
                        if (es) Stream_Free(es, TRUE);
                    }
                    
                    rs = Stream_New(NULL, 8);
                    if (rs && rdpecam_build_success_response(rs)) {
                        Stream_SealLength(rs);
                        const size_t out_len = Stream_Length(rs);
                        guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                        pthread_mutex_lock(&(rdp_client->message_lock));
                        result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                        pthread_mutex_unlock(&(rdp_client->message_lock));
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x01 SuccessResponse (DeactivateDevice device=%s)",
                                channel_id, device->device_name);

                        /* Only inform browser to stop camera if this device was the active sender.
                         * This prevents stopping the wrong camera when multiple devices exist but
                         * only one is actively streaming. */
                        if (was_active_sender) {
                            guac_client_log(client, GUAC_LOG_DEBUG,
                                    "RDPECAM sending camera-stop to browser (device %s was active sender)",
                                    device->device_name);
                            guac_client_for_owner(client,
                                guac_rdp_rdpecam_send_camera_stop_signal_callback,
                                NULL);
                        } else {
                            guac_client_log(client, GUAC_LOG_DEBUG,
                                    "RDPECAM NOT sending camera-stop to browser (device %s was not active sender)",
                                    device->device_name);
                        }
                    }
                }
                break;
            }

            case RDPECAM_MSG_STREAM_LIST_REQUEST: {
                if (!device) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM StreamListRequest received but no device available on ChannelId=%" PRIu32,
                            channel_id);
                    return CHANNEL_RC_OK;
                }
                
                /* StreamListRequest has no payload - just respond with our stream list */
                guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM received StreamListRequest");
                
                rdpecam_stream_desc stream = {
                    .FrameSourceType = CAM_STREAM_FRAME_SOURCE_TYPE_Color,
                    .Category = CAM_STREAM_CATEGORY_Capture,
                    .Selected = 1,
                    .CanBeShared = 0
                };
                rs = Stream_New(NULL, 16);
                if (rs && rdpecam_build_stream_list(rs, &stream, 1)) {
                    Stream_SealLength(rs);
                    const size_t out_len = Stream_Length(rs);
                    guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                    pthread_mutex_lock(&(rdp_client->message_lock));
                    result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                    pthread_mutex_unlock(&(rdp_client->message_lock));
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x0A StreamListResponse (streams=%u)",
                            channel_id, 1U);
                }
                break;
            }

            case RDPECAM_MSG_MEDIA_TYPE_LIST_REQUEST: {
                if (!device) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM MediaTypeListRequest received but no device available on ChannelId=%" PRIu32,
                            channel_id);
                    return CHANNEL_RC_OK;
                }
                
                /* Read stream index */
                if (payload_len < 1) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM MediaTypeListRequest missing stream index (payload_len=%zu) ChannelId=%" PRIu32,
                            payload_len, channel_id);
                    return CHANNEL_RC_OK;
                }
                uint8_t stream_idx;
                Stream_Read_UINT8(stream, stream_idx);
                
                guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM received MediaTypeListRequest for stream %d", stream_idx);
                
                rdpecam_media_type_desc media_types[GUAC_RDP_RDPECAM_MAX_FORMATS];
                size_t media_type_count = 0;

                /* Get formats for this specific device */
                guac_rwlock_acquire_read_lock(&(rdp_client->lock));
                guac_rdp_rdpecam_device_caps* caps = NULL;
                if (ch_name) {
                    caps = guac_rdp_rdpecam_get_device_caps(rdp_client, ch_name);
                }

                if (caps && caps->format_count > 0) {
                    /* Use formats from this device's capabilities */
                    for (unsigned int i = 0; i < caps->format_count
                            && media_type_count < GUAC_RDP_RDPECAM_MAX_FORMATS; i++) {
                        guac_rdp_rdpecam_format* fmt = &caps->formats[i];
                        if (!fmt->width || !fmt->height || !fmt->fps_num)
                            continue;
                        media_types[media_type_count++] = (rdpecam_media_type_desc) {
                            .Format = CAM_MEDIA_FORMAT_H264,
                            .Width = fmt->width,
                            .Height = fmt->height,
                            .FrameRateNumerator = fmt->fps_num,
                            .FrameRateDenominator = fmt->fps_den ? fmt->fps_den : 1,
                            .PixelAspectRatioNumerator = 1,
                            .PixelAspectRatioDenominator = 1,
                            .Flags = CAM_MEDIA_TYPE_DESCRIPTION_FLAG_DecodingRequired
                        };
                    }
                }
                guac_rwlock_release_lock(&(rdp_client->lock));

                if (media_type_count == 0) {
                    media_types[media_type_count++] = (rdpecam_media_type_desc) {
                        .Format = CAM_MEDIA_FORMAT_H264,
                        .Width = GUAC_RDPECAM_DEFAULT_WIDTH,
                        .Height = GUAC_RDPECAM_DEFAULT_HEIGHT,
                        .FrameRateNumerator = GUAC_RDPECAM_DEFAULT_FPS_NUM,
                        .FrameRateDenominator = GUAC_RDPECAM_DEFAULT_FPS_DEN,
                        .PixelAspectRatioNumerator = 1,
                        .PixelAspectRatioDenominator = 1,
                        .Flags = CAM_MEDIA_TYPE_DESCRIPTION_FLAG_DecodingRequired
                    };
                    media_types[media_type_count++] = (rdpecam_media_type_desc) {
                        .Format = CAM_MEDIA_FORMAT_H264,
                        .Width = 320,
                        .Height = 240,
                        .FrameRateNumerator = GUAC_RDPECAM_DEFAULT_FPS_NUM,
                        .FrameRateDenominator = GUAC_RDPECAM_DEFAULT_FPS_DEN,
                        .PixelAspectRatioNumerator = 1,
                        .PixelAspectRatioDenominator = 1,
                        .Flags = CAM_MEDIA_TYPE_DESCRIPTION_FLAG_DecodingRequired
                    };
                }

                rs = Stream_New(NULL, 128);
                if (rs && rdpecam_build_media_type_list(rs, media_types, media_type_count)) {
                    Stream_SealLength(rs);
                    const size_t out_len = Stream_Length(rs);
                    guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                    pthread_mutex_lock(&(rdp_client->message_lock));
                    result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                    pthread_mutex_unlock(&(rdp_client->message_lock));
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x0C MediaTypeListResponse (count=%zu)",
                            channel_id, media_type_count);
                }
                break;
            }

            case RDPECAM_MSG_CURRENT_MEDIA_TYPE_REQUEST: {
                if (!device) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM CurrentMediaTypeRequest received but no device available on ChannelId=%" PRIu32,
                            channel_id);
                    return CHANNEL_RC_OK;
                }
                
                /* Read stream index */
                if (payload_len < 1) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM CurrentMediaTypeRequest missing stream index (ChannelId=%" PRIu32 ")",
                            channel_id);
                    return CHANNEL_RC_OK;
                }
                uint8_t stream_idx;
                Stream_Read_UINT8(stream, stream_idx);
                
                if (stream_idx == 0) {
                    /* If no media type set yet, use the default (first advertised type) */
                    rdpecam_media_type_desc media_type = device->media_type;
                    if (media_type.Format == 0) {
                        /* Get formats for this specific device */
                        guac_rwlock_acquire_read_lock(&(rdp_client->lock));
                        guac_rdp_rdpecam_device_caps* caps = NULL;
                        if (ch_name) {
                            caps = guac_rdp_rdpecam_get_device_caps(rdp_client, ch_name);
                        }

                        if (caps && caps->format_count > 0) {
                            /* Use first format from this device's capabilities */
                            guac_rdp_rdpecam_format* preferred = &caps->formats[0];
                            media_type = (rdpecam_media_type_desc){
                                .Format = CAM_MEDIA_FORMAT_H264,
                                .Width = preferred->width,
                                .Height = preferred->height,
                                .FrameRateNumerator = preferred->fps_num,
                                .FrameRateDenominator = preferred->fps_den ? preferred->fps_den : 1,
                                .PixelAspectRatioNumerator = 1,
                                .PixelAspectRatioDenominator = 1,
                                .Flags = CAM_MEDIA_TYPE_DESCRIPTION_FLAG_DecodingRequired
                            };
                        } else {
                            media_type = (rdpecam_media_type_desc){
                                .Format = CAM_MEDIA_FORMAT_H264,
                                .Width = GUAC_RDPECAM_DEFAULT_WIDTH,
                                .Height = GUAC_RDPECAM_DEFAULT_HEIGHT,
                                .FrameRateNumerator = GUAC_RDPECAM_DEFAULT_FPS_NUM,
                                .FrameRateDenominator = GUAC_RDPECAM_DEFAULT_FPS_DEN,
                                .PixelAspectRatioNumerator = 1,
                                .PixelAspectRatioDenominator = 1,
                                .Flags = CAM_MEDIA_TYPE_DESCRIPTION_FLAG_DecodingRequired
                            };
                        }
                        guac_rwlock_release_lock(&(rdp_client->lock));
                    }

                    rs = Stream_New(NULL, 64);
                    if (rs && rdpecam_build_current_media_type(rs, &media_type)) {
                        Stream_SealLength(rs);
                        const size_t out_len = Stream_Length(rs);
                        guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                        pthread_mutex_lock(&(rdp_client->message_lock));
                        result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                        pthread_mutex_unlock(&(rdp_client->message_lock));
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x0E CurrentMediaTypeResponse (format=%u, %ux%u@%u/%u)",
                                channel_id, media_type.Format, media_type.Width, media_type.Height,
                                media_type.FrameRateNumerator, media_type.FrameRateDenominator);
                    }
                }
                break;
            }

            case RDPECAM_MSG_START_STREAMS_REQUEST: {
                if (!device || !device->sink) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM StartStreamsRequest received but no device/sink available (ChannelId=%" PRIu32 ")",
                            channel_id);
                    return CHANNEL_RC_OK;
                }

                /* Parse StartStreamsRequest from stream */
                if (payload_len < 1 + 26) {  /* stream_idx + media_type_desc */
                    guac_client_log(client, GUAC_LOG_WARNING, "RDPECAM StartStreamsRequest too short");
                    return CHANNEL_RC_OK;
                }

                uint8_t stream_idx;
                Stream_Read_UINT8(stream, stream_idx);

                /* Read media type description */
                rdpecam_media_type_desc media_type;
                Stream_Read_UINT8(stream, media_type.Format);
                Stream_Read_UINT32(stream, media_type.Width);
                Stream_Read_UINT32(stream, media_type.Height);
                Stream_Read_UINT32(stream, media_type.FrameRateNumerator);
                Stream_Read_UINT32(stream, media_type.FrameRateDenominator);
                Stream_Read_UINT32(stream, media_type.PixelAspectRatioNumerator);
                Stream_Read_UINT32(stream, media_type.PixelAspectRatioDenominator);
                Stream_Read_UINT8(stream, media_type.Flags);

                /* Handle camera switching: if another device is currently streaming,
                 * stop it before starting this device (single-camera model).
                 * Windows doesn't explicitly stop the old camera before starting the new one,
                 * so we must handle the switch automatically. */
                if (rdp_client->rdpecam_sink != NULL && rdp_client->rdpecam_sink != device->sink) {
                    guac_rdpecam_sink* old_sink = rdp_client->rdpecam_sink;

                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM switching cameras: stopping previous device to start %s",
                            device->device_name);

                    /* Find and stop the old device that owns old_sink */
                    if (plugin && plugin->devices) {
                        /* Iterate through channel slots instead of using broken HashTable_GetKeys */
                        for (unsigned int check_idx = 0; check_idx < 100; check_idx++) {
                            char check_channel[64];
                            snprintf(check_channel, sizeof(check_channel), "RDCamera_Device_%u", check_idx);

                            guac_rdpecam_device* old_device =
                                (guac_rdpecam_device*) HashTable_GetItemValue(plugin->devices, check_channel);
                            if (old_device && old_device->sink == old_sink) {
                                /* Stop the old device */
                                pthread_mutex_lock(&old_device->lock);
                                old_device->streaming = false;
                                old_device->is_active_sender = false;
                                old_device->credits = 0;
                                old_device->stream_channel = NULL;
                                old_device->stream_channel_id = 0;
                                pthread_cond_broadcast(&old_device->credits_signal);
                                pthread_mutex_unlock(&old_device->lock);
                                guac_client_log(client, GUAC_LOG_DEBUG,
                                        "RDPECAM stopped streaming on device %s for camera switch",
                                        old_device->device_name);
                                break;
                            }
                        }
                    }

                    /* Clear the old sink's streaming state */
                    pthread_mutex_lock(&old_sink->lock);
                    old_sink->streaming = false;
                    old_sink->credits = 0;
                    old_sink->has_active_sender = false;
                    old_sink->active_sender_channel = NULL;
                    pthread_mutex_unlock(&old_sink->lock);

                    /* Clear browser's frame push target temporarily.
                     * The new camera-start signal (sent below) will inform the browser
                     * to switch cameras. We don't send camera-stop because:
                     * 1. It has no device ID, so browser can't distinguish which camera to stop
                     * 2. The browser's camera-start handler should gracefully handle switching
                     * 3. Sending stop+start creates a temporary "no camera" state that can confuse browser */
                    rdp_client->rdpecam_sink = NULL;

                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM stopped streaming on old device, proceeding with %s (browser will be notified via camera-start)",
                            device->device_name);
                }

                if (stream_idx == 0) {
                    /* Persist media type for later requests */
                    pthread_mutex_lock(&device->lock);
                    device->media_type = media_type;
                    device->stream_index = stream_idx;
                    device->sample_sequence = 0;
                    device->credits = 0;
                    device->streaming = true;
                    device->need_keyframe = true;
                    device->is_active_sender = true;
                    device->stopping = false;
                    device->stream_channel = channel;
                    device->stream_channel_id = channel_id;
                    pthread_cond_broadcast(&device->credits_signal);
                    pthread_mutex_unlock(&device->lock);

                    rdpecam_channel_callback->is_stream_channel = true;

                    guac_rdpecam_sink* sink = device->sink;
                    pthread_mutex_lock(&sink->lock);

                    /* Flush any stale frames queued before Start Streams */
                    int flushed = 0;
                    while (sink->queue_head) {
                        guac_rdpecam_frame* stale = sink->queue_head;
                        sink->queue_head = stale->next;
                        guac_mem_free(stale->data);
                        guac_mem_free(stale);
                        flushed++;
                    }
                    sink->queue_tail = NULL;
                    sink->queue_size = 0;

                    if (flushed > 0) {
                        guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM flushed %d stale frames before streaming", flushed);
                    }

                    sink->stopping = false;
                    sink->streaming = true;
                    sink->credits = 0;
                    sink->stream_index = stream_idx;
                    if (!sink->has_active_sender) {
                        sink->has_active_sender = true;
                        sink->active_sender_channel = (void*) channel;
                        guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM active sender claimed by device channel");
                    }
                    pthread_mutex_unlock(&sink->lock);

                    guac_client_log(client, GUAC_LOG_DEBUG,
                        "RDPECAM streaming started ChannelId=%" PRIu32 " format=%u %ux%u@%u/%u",
                        channel_id, media_type.Format, media_type.Width, media_type.Height,
                        media_type.FrameRateNumerator, media_type.FrameRateDenominator);

                    /* Browser pushes frames into rdp_client->rdpecam_sink. Point it at this device's sink. */
                    rdp_client->rdpecam_sink = sink;

                    rs = Stream_New(NULL, 8);
                    if (rs && rdpecam_build_start_streams_response(rs, 0)) {
                        Stream_SealLength(rs);
                        const size_t out_len = Stream_Length(rs);
                        guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                        pthread_mutex_lock(&(rdp_client->message_lock));
                        result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                        pthread_mutex_unlock(&(rdp_client->message_lock));
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x01 SuccessResponse (StartStreams)",
                                channel_id);

                        if (result == CHANNEL_RC_OK) {
                            guac_rdp_camera_start_params camera_params = {
                                .width = media_type.Width,
                                .height = media_type.Height,
                                .fps_numerator = media_type.FrameRateNumerator,
                                .fps_denominator = media_type.FrameRateDenominator,
                                .stream_index = stream_idx,
                                .device_id = device ? device->browser_device_id : NULL
                            };

                            guac_client_for_owner(client,
                                guac_rdp_rdpecam_send_camera_start_signal_callback,
                                &camera_params);

                            guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM sent camera-start signal to JavaScript: "
                                "width=%u, height=%u, fps=%u/%u, stream_index=%u",
                                media_type.Width, media_type.Height,
                                media_type.FrameRateNumerator, media_type.FrameRateDenominator,
                                stream_idx);
                        }
                    }
                }
                break;
            }

            case RDPECAM_MSG_STOP_STREAMS_REQUEST: {
                if (!device || !device->sink) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM StopStreamsRequest received but no device/sink available (ChannelId=%" PRIu32 ")",
                            channel_id);
                    return CHANNEL_RC_OK;
                }

                uint32_t outstanding = 0;
                uint32_t stream_index = 0;

                pthread_mutex_lock(&device->lock);
                if (!device->stream_channel) {
                    device->stream_channel = channel;
                    pthread_cond_broadcast(&device->credits_signal);
                }
                rdpecam_channel_callback->is_stream_channel = true;
                outstanding = device->credits;
                stream_index = device->stream_index;
                device->credits = 0;
                device->streaming = false;
                device->is_active_sender = false;
                device->need_keyframe = true;
                pthread_cond_broadcast(&device->credits_signal);
                pthread_mutex_unlock(&device->lock);

                guac_rdpecam_sink* sink = device->sink;
                pthread_mutex_lock(&sink->lock);
                sink->streaming = false;
                sink->credits = 0;
                sink->has_active_sender = false;
                sink->active_sender_channel = NULL;
                pthread_mutex_unlock(&sink->lock);

                if (rdp_client->rdpecam_sink == sink)
                    rdp_client->rdpecam_sink = NULL;

                for (uint32_t i = 0; i < outstanding; i++) {
                    wStream* es = Stream_New(NULL, 8);
                    if (es && rdpecam_build_sample_error_response(es, stream_index)) {
                        Stream_SealLength(es);
                        const size_t err_len = Stream_Length(es);
                        guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, es);
                        pthread_mutex_lock(&(rdp_client->message_lock));
                        channel->Write(channel, (UINT32) err_len, Stream_Buffer(es), NULL);
                        pthread_mutex_unlock(&(rdp_client->message_lock));
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x13 SampleErrorResponse (stream=%u)",
                                channel_id, stream_index);
                    }
                    if (es) Stream_Free(es, TRUE);
                }
                rs = Stream_New(NULL, 8);
                if (rs && rdpecam_build_stop_streams_response(rs, /*status*/ 0)) {
                    Stream_SealLength(rs);
                    const size_t out_len = Stream_Length(rs);
                    guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                    pthread_mutex_lock(&(rdp_client->message_lock));
                    result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                    pthread_mutex_unlock(&(rdp_client->message_lock));
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x01 SuccessResponse (StopStreams)",
                            channel_id);
                    
                    /* PROTOCOL-DRIVEN CAMERA STOP: Signal JavaScript client to stop
                     * camera capture NOW that Windows has requested stream stop.
                     * This coordinates with SERVER_PROTO_001 fix and ensures browser
                     * stops capturing at correct protocol time.
                     * 
                     * Timing: After Stop Streams Response sent to Windows.
                     * Effect: Browser receives argv camera-stop instruction and stops
                     *         getUserMedia() and encoder, cleaning up resources. */
                    if (result == CHANNEL_RC_OK) {
                        /* Send camera-stop signal to owner user via argv instruction */
                        guac_client_for_owner(client, 
                            guac_rdp_rdpecam_send_camera_stop_signal_callback, 
                            NULL);
                        
                        guac_client_log(client, GUAC_LOG_DEBUG, 
                            "RDPECAM sent camera-stop signal to JavaScript");
                    }
                }
                break;
            }

            case RDPECAM_MSG_PROPERTY_LIST_REQUEST: {
                if (!device) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "RDPECAM PropertyListRequest received but no device available (ChannelId=%" PRIu32 ")",
                            channel_id);
                    return CHANNEL_RC_OK;
                }
                
                /* PropertyListRequest has no payload - respond with empty property list */
                guac_client_log(client, GUAC_LOG_DEBUG,
                        "RDPECAM received PropertyListRequest ChannelId=%" PRIu32, channel_id);
                
                rs = Stream_New(NULL, 8);
                if (rs) {
                    Stream_Write_UINT8(rs, RDPECAM_PROTO_VERSION);
                    Stream_Write_UINT8(rs, RDPECAM_MSG_PROPERTY_LIST_RESPONSE);
                    Stream_SealLength(rs);
                    const size_t out_len = Stream_Length(rs);
                    guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, rs);
                    pthread_mutex_lock(&(rdp_client->message_lock));
                    result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
                    pthread_mutex_unlock(&(rdp_client->message_lock));
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x15 PropertyListResponse (empty)",
                            channel_id);
                }
                break;
            }

            case RDPECAM_MSG_SAMPLE_REQUEST: {
                if (!device) {
                    guac_client_log(client, GUAC_LOG_WARNING, "RDPECAM SampleRequest received but no device available on %s[id=%" PRIu32 "]", ch_name, channel_id);
                    return CHANNEL_RC_OK;
                }

                /* Read stream index */
                if (payload_len < 1) {
                    guac_client_log(client, GUAC_LOG_WARNING, "RDPECAM SampleRequest missing stream index");
                    return CHANNEL_RC_OK;
                }
                uint8_t stream_idx;
                Stream_Read_UINT8(stream, stream_idx);

                if (stream_idx == 0 || stream_idx == device->stream_index) {
                    pthread_mutex_lock(&device->lock);
                    /* SampleRequests grant credits on the channel they arrive; bind responses there. */
                    if (device->stream_channel != channel) {
                        device->stream_channel = channel;
                        device->stream_channel_id = channel_id;
                        pthread_cond_broadcast(&device->credits_signal);
                    }
                    rdpecam_channel_callback->is_stream_channel = true;
                    uint32_t before = device->credits;
                    device->credits = GUAC_RDPECAM_SAMPLE_CREDITS;
                    uint32_t remaining = device->credits;
                    /* Wake dequeue thread waiting on this device */
                    pthread_cond_broadcast(&device->credits_signal);
                    pthread_mutex_unlock(&device->lock);

                    /* Ensure browser has a sink to push into if streaming is active */
                    if (device->streaming) {
                        guac_rdp_client* rdp_client_ctx = (guac_rdp_client*) client->data;
                        if (rdp_client_ctx && !rdp_client_ctx->rdpecam_sink) {
                            rdp_client_ctx->rdpecam_sink = device->sink;
                            guac_client_log(client, GUAC_LOG_DEBUG,
                                "RDPECAM bound session sink to active device due to SampleRequest (channel=%" PRIu32 ")",
                                channel_id);
                        }
                    }
                    int queue_size = guac_rdpecam_get_queue_size(device->sink);
                    guac_client_log(client, GUAC_LOG_DEBUG,
                                   "RDPECAM SampleRequest ChannelId=%" PRIu32 " device=%s credits %u->%u queue=%d/%d",
                                   channel_id, device->device_name, before, remaining, queue_size,
                                   GUAC_RDPECAM_MAX_FRAMES);
                }
                break;
            }

            default:
                /* Unknown/unsupported CAM msg; ignore so custom shim can handle */
                break;
        }

        if (rs) Stream_Free(rs, TRUE);
    }
    
    /* Check if capabilities were updated while we were processing messages.
     * This handles the case where capabilities arrive after version negotiation. */
    if (plugin && plugin->version_negotiated && plugin->enumerator_channel) {
        guac_rwlock_acquire_write_lock(&(rdp_client->lock));
        if (rdp_client->rdpecam_caps_updated && rdp_client->rdpecam_device_caps_count > 0) {
            guac_rdp_rdpecam_send_device_notifications(plugin, client, rdp_client, plugin->enumerator_channel);
            rdp_client->rdpecam_caps_updated = 0; /* Clear the flag */
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM sent device notifications after late capability update");
        }
        guac_rwlock_release_lock(&(rdp_client->lock));
    }
    
    return CHANNEL_RC_OK;
}

/**
 * Callback which is invoked when data is received along the RDPECAM channel.
 * This callback is API-dependent and delegates to the API-independent
 * guac_rdp_rdpecam_handle_data function.
 *
 * @param channel_callback
 *     The channel callback structure associated with the connection along
 *     which the data was received.
 *
 * @param stream
 *     The data received.
 *
 * @return
 *     Always zero.
 */
static UINT guac_rdp_rdpecam_data(IWTSVirtualChannelCallback* channel_callback,
        wStream* stream) {

    guac_rdp_rdpecam_channel_callback* rdpecam_channel_callback =
        (guac_rdp_rdpecam_channel_callback*) channel_callback;
    IWTSVirtualChannel* channel = rdpecam_channel_callback->channel;

    /* Invoke generalized (API-independent) data handler with full callback context */
    guac_rdp_rdpecam_handle_data(rdpecam_channel_callback->client, channel, stream, rdpecam_channel_callback);

    return CHANNEL_RC_OK;
}

/**
 * Callback which is invoked when the RDPECAM channel is opened.
 * This is where we initiate the protocol by sending SelectVersionRequest.
 *
 * @param channel_callback
 *     The channel callback structure associated with the connection.
 *
 * @return
 *     CHANNEL_RC_OK on success, an error code otherwise.
 */
static UINT guac_rdp_rdpecam_open(IWTSVirtualChannelCallback* channel_callback) {

    guac_rdp_rdpecam_channel_callback* rdpecam_channel_callback =
        (guac_rdp_rdpecam_channel_callback*) channel_callback;
    IWTSVirtualChannel* channel = rdpecam_channel_callback->channel;
    guac_client* client = rdpecam_channel_callback->client;
    const char* ch_name = rdpecam_channel_callback->channel_name;
    const UINT32 channel_id = rdpecam_channel_callback ? rdpecam_channel_callback->channel_id : 0;

    guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM channel opened (%s) [id=%" PRIu32 "]", ch_name,
            channel_id);

    /* On control channel: initiate version negotiation */
    if (strcasecmp(ch_name, GUAC_RDPECAM_CHANNEL_NAME) == 0) {
        wStream* s = Stream_New(NULL, 8);
        if (s && rdpecam_build_version_request(s)) {
            Stream_SealLength(s);
            const size_t out_len = Stream_Length(s);
            guac_rdp_rdpecam_log_stream(client, "TX", ch_name, channel_id, s);
            
            /* Use message_lock to prevent blocking the RDP event loop */
            guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
            pthread_mutex_lock(&(rdp_client->message_lock));
            UINT result = channel->Write(channel, (UINT32) out_len, Stream_Buffer(s), NULL);
            pthread_mutex_unlock(&(rdp_client->message_lock));
            
            Stream_Free(s, TRUE);
            if (result != CHANNEL_RC_OK) {
                guac_client_log(client, GUAC_LOG_ERROR, "Failed to send SelectVersionRequest: %u", result);
                return result;
            }

            /* Remember enumerator channel for DeviceRemovedNotification */
            guac_rdp_rdpecam_channel_callback* cb = (guac_rdp_rdpecam_channel_callback*) channel_callback;
            if (cb && cb->plugin) {
                guac_rdp_rdpecam_plugin* plugin = cb->plugin;
                plugin->enumerator_channel = channel;
            }
        }
        else {
            if (s) Stream_Free(s, TRUE);
            guac_client_log(client, GUAC_LOG_ERROR, "Failed to build SelectVersionRequest");
            return CHANNEL_RC_NO_MEMORY;
        }
    }

    return CHANNEL_RC_OK;
}

/**
 * Callback which is invoked when a connection to the RDPECAM channel is
 * closed.
 *
 * @param channel_callback
 *     The channel callback structure associated with the connection that was
 *     closed.
 *
 * @return
 *     Always zero.
 */
static UINT guac_rdp_rdpecam_close(IWTSVirtualChannelCallback* channel_callback) {

    guac_rdp_rdpecam_channel_callback* rdpecam_channel_callback =
        (guac_rdp_rdpecam_channel_callback*) channel_callback;
    guac_rdpecam_device* device = rdpecam_channel_callback->device;
    guac_rdp_rdpecam_plugin* plugin = rdpecam_channel_callback->plugin;
    const char* ch_name = rdpecam_channel_callback->channel_name;

    /* Log channel close */
    guac_client_log(rdpecam_channel_callback->client, GUAC_LOG_DEBUG,
            "RDPECAM channel connection closed (%s) [id=%" PRIu32 "]",
            rdpecam_channel_callback->channel_name ? rdpecam_channel_callback->channel_name : "unknown",
            rdpecam_channel_callback->channel_id);

    if (device) {
        int remaining_refs = 0;
        bool closing_stream_channel = false;
        bool was_active_sender = false;

        pthread_mutex_lock(&device->lock);

        if (device->ref_count > 0)
            device->ref_count--;
        remaining_refs = device->ref_count;

        if (device->stream_channel == rdpecam_channel_callback->channel)
            closing_stream_channel = true;
        else if (rdpecam_channel_callback->is_stream_channel)
            closing_stream_channel = true;

        /* Capture whether this device was the active sender BEFORE clearing state.
         * Only the active sender should trigger browser camera-stop when its channel closes. */
        if (closing_stream_channel)
            was_active_sender = device->is_active_sender;

        if (closing_stream_channel) {
            device->stream_channel = NULL;
            device->is_active_sender = false;
            device->streaming = false;
            device->need_keyframe = true;
            pthread_cond_broadcast(&device->credits_signal);
        }

        pthread_mutex_unlock(&device->lock);

        if (closing_stream_channel) {
            guac_rdpecam_signal_stop(device->sink);

            /* Only notify browser to stop camera if this device was the active sender.
             * When switching cameras, the old device's stream channel closes but it's
             * no longer the active sender, so we shouldn't send camera-stop. */
            if (was_active_sender) {
                guac_client_log(rdpecam_channel_callback->client, GUAC_LOG_DEBUG,
                        "RDPECAM sending camera-stop to browser (stream channel closed for active sender device %s)",
                        ch_name ? ch_name : "unknown");
                guac_client_for_owner(rdpecam_channel_callback->client,
                    guac_rdp_rdpecam_send_camera_stop_signal_callback,
                    NULL);
            } else {
                guac_client_log(rdpecam_channel_callback->client, GUAC_LOG_DEBUG,
                        "RDPECAM NOT sending camera-stop to browser (stream channel closed for non-active device %s)",
                        ch_name ? ch_name : "unknown");
            }

            guac_rdp_client* close_rdp_client =
                (guac_rdp_client*) rdpecam_channel_callback->client->data;
            if (close_rdp_client && close_rdp_client->rdpecam_sink == device->sink)
                close_rdp_client->rdpecam_sink = NULL;
        }

        if (plugin && plugin->devices && ch_name && remaining_refs == 0) {
            pthread_mutex_lock(&device->lock);
            device->stopping = true;
            pthread_cond_broadcast(&device->credits_signal);
            pthread_mutex_unlock(&device->lock);

            if (!closing_stream_channel)
                guac_rdpecam_signal_stop(device->sink);

            /* Remove from registry; explicitly destroy device afterwards. */
            guac_rdpecam_device* to_destroy = device;
            if (HashTable_Remove(plugin->devices, (void*) ch_name)) {
                guac_client_log(rdpecam_channel_callback->client, GUAC_LOG_DEBUG,
                    "RDPECAM device removed from registry: %s", ch_name);
                guac_rdpecam_device_destroy(to_destroy);
            }
        }
        else if (remaining_refs != 0) {
            guac_client_log(rdpecam_channel_callback->client, GUAC_LOG_DEBUG,
                "RDPECAM device %s still referenced (%d), deferring destruction",
                ch_name ? ch_name : "unknown", remaining_refs);
        }
    }

    /* Free channel callback */
    guac_mem_free(rdpecam_channel_callback);

    return CHANNEL_RC_OK;

}

/**
 * Callback which is invoked when a new connection to the RDPECAM channel is
 * established. This callback allocates and initializes the channel callback
 * structure containing the required callbacks should be assigned.
 *
 * @param listener_callback
 *     The listener callback structure that was registered for the RDPECAM
 *     channel.
 *
 * @param channel
 *     The virtual channel instance along which the new connection was
 *     established.
 *
 * @param data
 *     The data associated with the new connection.
 *
 * @param accept
 *     Whether to accept the connection.
 *
 * @param channel_callback
 *     The channel callback structure containing the required callbacks should
 *     be assigned.
 *
 * @return
 *     Always zero.
 */
static UINT guac_rdp_rdpecam_new_connection(
        IWTSListenerCallback* listener_callback, IWTSVirtualChannel* channel,
        BYTE* data, int* accept,
        IWTSVirtualChannelCallback** channel_callback) {

    guac_rdp_rdpecam_listener_callback* rdpecam_listener_callback =
        (guac_rdp_rdpecam_listener_callback*) listener_callback;
    guac_client* client = rdpecam_listener_callback->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_rdpecam_plugin* plugin = rdpecam_listener_callback->plugin;

    /* Log new RDPECAM connection */
    const char* ch_name = rdpecam_listener_callback->channel_name ? rdpecam_listener_callback->channel_name : "rdpecam";
    UINT32 channel_id = 0;
    if (plugin && plugin->manager && plugin->manager->GetChannelId)
        channel_id = plugin->manager->GetChannelId(channel);

    guac_client_log(client, GUAC_LOG_DEBUG, "New RDPECAM channel connection (%s) [id=%" PRIu32 "]", ch_name, channel_id);

    /* Ensure there is a device structure for per-channel state. */
    guac_rdpecam_device* device = NULL;

    if (strcasecmp(ch_name, GUAC_RDPECAM_CHANNEL_NAME) != 0) {

        /* Handle device channel connections. */
        device = guac_rdpecam_device_lookup(plugin, ch_name);

        if (!device) {
            device = guac_rdpecam_device_create(plugin, ch_name);
            if (!device) {
                guac_client_log(client, GUAC_LOG_ERROR,
                    "Failed to create RDPECAM device: %s", ch_name);
                *accept = 0;
                return CHANNEL_RC_OK;
            }

            /* Prefer HashTable_Insert for WinPR3/FreeRDP3 forward-compatibility;
             * fall back to HashTable_Add for older WinPR 2.x that may not export it. */
#ifdef HAVE_WINPR_HASHTABLE_INSERT
            if (!HashTable_Insert(plugin->devices, (void*) ch_name, (void*) device)) {
#else
            /* Older WinPR may not export HashTable_Insert; HashTable_Add exists but
             * may be compiled out unless WITH_WINPR_DEPRECATED is enabled. If missing
             * at link time, fall back to a simple leak-safe path by not registering a
             * destructor and relying on explicit removal. */
            if (HashTable_Add(plugin->devices, (void*) ch_name, (void*) device) < 0) {
#endif
                guac_client_log(client, GUAC_LOG_ERROR,
                    "Failed to insert RDPECAM device into registry: %s", ch_name);
                guac_rdpecam_device_destroy(device);
                *accept = 0;
                return CHANNEL_RC_OK;
            }

            guac_client_log(client, GUAC_LOG_DEBUG,
                "Created new RDPECAM device: %s", ch_name);
        }
        else {
            pthread_mutex_lock(&device->lock);
            device->ref_count++;
            device->stopping = false;
            pthread_mutex_unlock(&device->lock);

            guac_client_log(client, GUAC_LOG_DEBUG,
                "Reusing existing RDPECAM device: %s (ref_count=%d)",
                ch_name, device->ref_count);
        }

    }
    /* The control/enumerator channel intentionally proceeds without a device. */

    /* Allocate new channel callback */
    guac_rdp_rdpecam_channel_callback* rdpecam_channel_callback =
        guac_mem_zalloc(sizeof(guac_rdp_rdpecam_channel_callback));

    /* Init channel callback with data from plugin */
    rdpecam_channel_callback->client = client;
    rdpecam_channel_callback->channel = channel;
    rdpecam_channel_callback->device = device;
    rdpecam_channel_callback->channel_name = ch_name;
    rdpecam_channel_callback->plugin = plugin;
    rdpecam_channel_callback->is_stream_channel = false;
    rdpecam_channel_callback->channel_id = channel_id;
    rdpecam_channel_callback->parent.OnDataReceived = guac_rdp_rdpecam_data;
    rdpecam_channel_callback->parent.OnOpen = guac_rdp_rdpecam_open;
    rdpecam_channel_callback->parent.OnClose = guac_rdp_rdpecam_close;

    /* Accept connection and return callback */
    *accept = 1;
    *channel_callback = (IWTSVirtualChannelCallback*) rdpecam_channel_callback;

    guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM channel connection established (%s) [id=%" PRIu32 "]",
            ch_name, channel_id);

    /* Messages will be sent in OnOpen callback, not here */
    return CHANNEL_RC_OK;

}

/**
 * Callback which is invoked when the RDPECAM plugin is being initialized and
 * the listener callback structure containing the required callbacks for new
 * connections must be registered.
 *
 * @param plugin
 *     The RDPECAM plugin being initialized.
 *
 * @param manager
 *     The virtual channel manager through which the listener callback
 *     structure containing the required callbacks should be registered.
 *
 * @return
 *     Always zero.
 */
static UINT guac_rdp_rdpecam_initialize(IWTSPlugin* plugin,
        IWTSVirtualChannelManager* manager) {

    /* Allocate control listener */
    guac_rdp_rdpecam_plugin* rdpecam_plugin = (guac_rdp_rdpecam_plugin*) plugin;
    guac_rdp_rdpecam_listener_callback* control_listener =
        guac_mem_zalloc(sizeof(guac_rdp_rdpecam_listener_callback));
    guac_rdp_rdpecam_listener_callback* device0_listener =
        guac_mem_zalloc(sizeof(guac_rdp_rdpecam_listener_callback));

    if (!control_listener || !device0_listener) {
        guac_client_log(rdpecam_plugin->client, GUAC_LOG_ERROR,
            "Failed to allocate RDPECAM listener callbacks");
        guac_mem_free(control_listener);
        guac_mem_free(device0_listener);
        return CHANNEL_RC_NO_MEMORY;
    }

    control_listener->client = rdpecam_plugin->client;
    control_listener->channel_name = GUAC_RDPECAM_CHANNEL_NAME;
    control_listener->plugin = rdpecam_plugin;
    control_listener->parent.OnNewChannelConnection = guac_rdp_rdpecam_new_connection;
    rdpecam_plugin->control_listener_callback = control_listener;

    device0_listener->client = rdpecam_plugin->client;
    device0_listener->channel_name = GUAC_RDPECAM_DEVICE0_CHANNEL_NAME;
    device0_listener->plugin = rdpecam_plugin;
    device0_listener->parent.OnNewChannelConnection = guac_rdp_rdpecam_new_connection;
    rdpecam_plugin->device0_listener_callback = device0_listener;

    /* Initialize hash table for multi-device support */
    rdpecam_plugin->devices = HashTable_New(FALSE);
    if (!rdpecam_plugin->devices) {
        guac_client_log(rdpecam_plugin->client, GUAC_LOG_ERROR,
            "Failed to create device hash table");
        guac_mem_free(control_listener);
        guac_mem_free(device0_listener);
        return CHANNEL_RC_NO_MEMORY;
    }

    /* Initialize hash table for device ID to channel name mapping */
    rdpecam_plugin->device_id_map = HashTable_New(FALSE);
    rdpecam_plugin->device_id_mappings = NULL;
    if (!rdpecam_plugin->device_id_map) {
        guac_client_log(rdpecam_plugin->client, GUAC_LOG_ERROR,
            "Failed to create device ID map hash table");
        HashTable_Free(rdpecam_plugin->devices);
        guac_mem_free(control_listener);
        guac_mem_free(device0_listener);
        return CHANNEL_RC_NO_MEMORY;
    }

    /* Hash table keys use stable pointers; explicit destruction is handled
     * on removal/termination. */

    /* Keep manager for later (dynamic device channel creation) */
    rdpecam_plugin->manager = manager;
    rdpecam_plugin->enumerator_channel = NULL;

    /* Register control channel listener */
    manager->CreateListener(manager, GUAC_RDPECAM_CHANNEL_NAME, 0,
            (IWTSListenerCallback*) control_listener, NULL);
    manager->CreateListener(manager, GUAC_RDPECAM_DEVICE0_CHANNEL_NAME, 0,
            (IWTSListenerCallback*) device0_listener, NULL);

    guac_client_log(rdpecam_plugin->client, GUAC_LOG_DEBUG,
        "RDPECAM plugin initialized with multi-device support");

    return CHANNEL_RC_OK;

}

/**
 * Callback which is invoked when all connections to the RDPECAM plugin
 * have closed and the plugin is being unloaded.
 *
 * @param plugin
 *     The RDPECAM plugin being unloaded.
 *
 * @return
 *     Always zero.
 */
static UINT guac_rdp_rdpecam_terminated(IWTSPlugin* plugin) {

    guac_rdp_rdpecam_plugin* rdpecam_plugin = (guac_rdp_rdpecam_plugin*) plugin;

    /* Free listener callbacks if allocated */
    if (rdpecam_plugin->control_listener_callback != NULL) {
        guac_mem_free(rdpecam_plugin->control_listener_callback);
        rdpecam_plugin->control_listener_callback = NULL;
    }
    if (rdpecam_plugin->device0_listener_callback != NULL) {
        guac_mem_free(rdpecam_plugin->device0_listener_callback);
        rdpecam_plugin->device0_listener_callback = NULL;
    }

    /* Destroy all devices in hash table */
    if (rdpecam_plugin->devices != NULL) {
        /* Iterate through channel slots instead of using broken HashTable_GetKeys */
        for (unsigned int check_idx = 0; check_idx < 100; check_idx++) {
            char channel_name[64];
            snprintf(channel_name, sizeof(channel_name), "RDCamera_Device_%u", check_idx);

            guac_rdpecam_device* dev = (guac_rdpecam_device*)
                HashTable_GetItemValue(rdpecam_plugin->devices, channel_name);
            if (dev)
                guac_rdpecam_device_destroy(dev);
        }
        HashTable_Free(rdpecam_plugin->devices);
        rdpecam_plugin->devices = NULL;
    }

    /* Clear device ID mappings and associated memory */
    guac_rdp_rdpecam_mapping_clear(rdpecam_plugin);

    /* Free device ID map */
    if (rdpecam_plugin->device_id_map != NULL) {
        HashTable_Free(rdpecam_plugin->device_id_map);
        rdpecam_plugin->device_id_map = NULL;
    }

    guac_client_log(rdpecam_plugin->client, GUAC_LOG_DEBUG, 
        "RDPECAM plugin terminated - all devices destroyed");

    return CHANNEL_RC_OK;

}

/**
 * Creates a new RDPECAM device structure for multi-device support.
 * This allocates and initializes all per-device state including the sink
 * and prepares for thread creation.
 *
 * @param plugin
 *     The RDPECAM plugin instance.
 *
 * @param device_name
 *     Name of the device (e.g., "RDCamera_Device_0").
 *
 * @return
 *     A newly-allocated device structure, or NULL on failure.
 */
static guac_rdpecam_device* guac_rdpecam_device_create(
        guac_rdp_rdpecam_plugin* plugin, const char* device_name) {

    if (!plugin || !device_name) {
        return NULL;
    }

    guac_rdpecam_device* device = guac_mem_zalloc(sizeof(guac_rdpecam_device));
    if (!device) {
        guac_client_log(plugin->client, GUAC_LOG_ERROR,
            "Failed to allocate RDPECAM device structure");
        return NULL;
    }

    /* Allocate and copy device name */
    device->device_name = guac_mem_alloc(strlen(device_name) + 1);
    if (!device->device_name) {
        guac_client_log(plugin->client, GUAC_LOG_ERROR,
            "Failed to allocate device name");
        guac_mem_free(device);
        return NULL;
    }
    strcpy(device->device_name, device_name);

    guac_rdp_client* rdp_client = plugin->client ? (guac_rdp_client*) plugin->client->data : NULL;

    /* Extract device index from channel name (e.g., "RDCamera_Device_0" -> 0) */
    unsigned int device_index = 0;
    if (rdp_client && sscanf(device_name, "RDCamera_Device_%u", &device_index) == 1) {
        /* Look up device capabilities and browser device ID */
        guac_rwlock_acquire_read_lock(&(rdp_client->lock));
        if (device_index < rdp_client->rdpecam_device_caps_count) {
            guac_rdp_rdpecam_device_caps* caps = &rdp_client->rdpecam_device_caps[device_index];
            if (caps->device_id && caps->device_id[0] != '\0') {
                size_t id_len = strlen(caps->device_id);
                device->browser_device_id = guac_mem_alloc(id_len + 1);
                if (device->browser_device_id) {
                    memcpy(device->browser_device_id, caps->device_id, id_len + 1);
                    guac_client_log(plugin->client, GUAC_LOG_DEBUG,
                            "RDPECAM device %s mapped to browser device ID: %s",
                            device_name, device->browser_device_id);
                }
            }
        }
        guac_rwlock_release_lock(&(rdp_client->lock));
    }

    /* Always create a fresh per-device sink for each device.
     * Note: rdp_client->rdpecam_sink is used as a pointer to the active device's sink
     * for the browser to push frames to. It should NOT be reused by new devices,
     * as doing so would steal the sink from an already-active device. */
    device->sink = guac_rdpecam_create(plugin->client);
    if (!device->sink) {
        guac_client_log(plugin->client, GUAC_LOG_ERROR,
            "Failed to create per-device sink for %s", device_name);
        guac_mem_free(device->device_name);
        guac_mem_free(device);
        return NULL;
    }

    guac_client_log(plugin->client, GUAC_LOG_DEBUG,
            "RDPECAM sink created for device: %s",
            device_name);

    /* Initialize thread synchronization primitives */
    if (pthread_mutex_init(&device->lock, NULL) != 0) {
        guac_client_log(plugin->client, GUAC_LOG_ERROR,
            "Failed to initialize device mutex");
        guac_rdpecam_destroy(device->sink);
        guac_mem_free(device->device_name);
        guac_mem_free(device);
        return NULL;
    }

    if (pthread_cond_init(&device->credits_signal, NULL) != 0) {
        guac_client_log(plugin->client, GUAC_LOG_ERROR,
            "Failed to initialize credits condition variable");
        pthread_mutex_destroy(&device->lock);
        guac_rdpecam_destroy(device->sink);
        guac_mem_free(device->device_name);
        guac_mem_free(device);
        return NULL;
    }

    /* Initialize device fields */
    device->stream_channel = NULL;
    device->dequeue_thread_started = false;
    memset(&device->media_type, 0, sizeof(device->media_type));
    device->stream_index = 0;
    device->credits = 0;
    device->sample_sequence = 0;
    device->is_active_sender = false;
    device->streaming = false;
    device->need_keyframe = true;
    device->stopping = false;
    device->ref_count = 1;

    /* Start per-device dequeue thread */
    if (pthread_create(&device->dequeue_thread, NULL, guac_rdp_rdpecam_dequeue_thread, device) != 0) {
        guac_client_log(plugin->client, GUAC_LOG_ERROR,
            "Failed to create dequeue thread for device: %s", device_name);
        pthread_cond_destroy(&device->credits_signal);
        pthread_mutex_destroy(&device->lock);
        guac_rdpecam_destroy(device->sink);
        guac_mem_free(device->device_name);
        guac_mem_free(device);
        return NULL;
    }

    device->dequeue_thread_started = true;

    guac_client_log(plugin->client, GUAC_LOG_DEBUG,
        "RDPECAM device created: %s (dequeue thread started)", device_name);

    return device;
}

/**
 * Removes the mapping entry associated with the given device ID, if present.
 */
static void guac_rdp_rdpecam_mapping_remove_by_device_id(
        guac_rdp_rdpecam_plugin* plugin, const char* device_id) {

    if (!plugin || !device_id)
        return;

    guac_rdp_rdpecam_device_mapping* prev = NULL;
    guac_rdp_rdpecam_device_mapping* current = plugin->device_id_mappings;

    while (current) {
        if (current->device_id_key && strcmp(current->device_id_key, device_id) == 0) {
            if (plugin->device_id_map)
                HashTable_Remove(plugin->device_id_map, (void*) current->device_id_key);

            if (prev)
                prev->next = current->next;
            else
                plugin->device_id_mappings = current->next;

            if (current->channel_name)
                guac_mem_free(current->channel_name);
            guac_mem_free(current);
            return;
        }

        prev = current;
        current = current->next;
    }
}

/**
 * Removes the mapping entry associated with the given channel name, if present.
 */
static void guac_rdp_rdpecam_mapping_remove_by_channel(
        guac_rdp_rdpecam_plugin* plugin, const char* channel_name) {

    if (!plugin || !channel_name)
        return;

    guac_rdp_rdpecam_device_mapping* prev = NULL;
    guac_rdp_rdpecam_device_mapping* current = plugin->device_id_mappings;

    while (current) {
        if (current->channel_name && strcmp(current->channel_name, channel_name) == 0) {
            if (plugin->device_id_map)
                HashTable_Remove(plugin->device_id_map, (void*) current->device_id_key);

            if (prev)
                prev->next = current->next;
            else
                plugin->device_id_mappings = current->next;

            guac_mem_free(current->channel_name);
            guac_mem_free(current);
            return;
        }

        prev = current;
        current = current->next;
    }
}

/**
 * Adds or replaces a device ID mapping to the given channel.
 *
 * @return
 *     TRUE if the mapping was successfully recorded, FALSE otherwise.
 */
static BOOL guac_rdp_rdpecam_mapping_add(
        guac_rdp_rdpecam_plugin* plugin, const char* device_id,
        const char* channel_name) {

    if (!plugin || !device_id || !channel_name)
        return FALSE;

    /* Remove any existing mapping for this device ID */
    guac_rdp_rdpecam_mapping_remove_by_device_id(plugin, device_id);

    char* channel_copy = guac_mem_alloc(strlen(channel_name) + 1);
    if (!channel_copy)
        return FALSE;
    strcpy(channel_copy, channel_name);

    guac_rdp_rdpecam_device_mapping* entry =
        guac_mem_zalloc(sizeof(guac_rdp_rdpecam_device_mapping));
    if (!entry) {
        guac_mem_free(channel_copy);
        return FALSE;
    }

    entry->device_id_key = device_id;
    entry->channel_name = channel_copy;
    entry->next = plugin->device_id_mappings;

#ifdef HAVE_WINPR_HASHTABLE_INSERT
    if (!HashTable_Insert(plugin->device_id_map, (void*) device_id, (void*) entry)) {
#else
    if (HashTable_Add(plugin->device_id_map, (void*) device_id, (void*) entry) < 0) {
#endif
        guac_mem_free(channel_copy);
        guac_mem_free(entry);
        return FALSE;
    }

    plugin->device_id_mappings = entry;
    return TRUE;
}

/**
 * Clears all device ID mappings, releasing any allocated memory.
 */
static void guac_rdp_rdpecam_mapping_clear(
        guac_rdp_rdpecam_plugin* plugin) {

    if (!plugin)
        return;

    guac_rdp_rdpecam_device_mapping* current = plugin->device_id_mappings;
    plugin->device_id_mappings = NULL;

    while (current) {
        guac_rdp_rdpecam_device_mapping* next = current->next;

        if (plugin->device_id_map)
            HashTable_Remove(plugin->device_id_map, (void*) current->device_id_key);

        if (current->channel_name)
            guac_mem_free(current->channel_name);
        guac_mem_free(current);

        current = next;
    }
}

/**
 * Destroys an RDPECAM device structure and frees all associated resources.
 * This is called as a destructor by the hash table when devices are removed
 * or when the plugin terminates.
 *
 * @param device
 *     The device to destroy. May be NULL.
 */
static void guac_rdpecam_device_destroy(guac_rdpecam_device* device) {

    if (!device) {
        return;
    }

    guac_client* client = device->sink ? device->sink->client : NULL;
    guac_rdp_client* rdp_client = client ? (guac_rdp_client*) client->data : NULL;

    /* Signal the dequeue thread to stop */
    pthread_mutex_lock(&device->lock);
    device->stopping = true;
    pthread_cond_broadcast(&device->credits_signal);
    pthread_mutex_unlock(&device->lock);

    if (device->sink)
        guac_rdpecam_signal_stop(device->sink);

    if (device->dequeue_thread_started) {
        int rc = pthread_join(device->dequeue_thread, NULL);
        if (rc != 0 && client) {
            guac_client_log(client, GUAC_LOG_WARNING,
                "RDPECAM dequeue thread join failed for device %s (rc=%d)",
                device->device_name ? device->device_name : "unknown", rc);
        }
    }

    /* Clean up synchronization primitives */
    pthread_cond_destroy(&device->credits_signal);
    pthread_mutex_destroy(&device->lock);

    /* Destroy per-device sink */
    if (device->sink) {
        if (rdp_client && rdp_client->rdpecam_sink == device->sink)
            rdp_client->rdpecam_sink = NULL;
        if (client) {
            guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM destroying sink for device: %s",
                device->device_name ? device->device_name : "unknown");
        }
        guac_rdpecam_destroy(device->sink);
    }

    /* Free browser device ID */
    if (device->browser_device_id) {
        guac_mem_free(device->browser_device_id);
    }

    /* Free device name */
    if (device->device_name) {
        guac_mem_free(device->device_name);
    }

    /* Free device structure */
    guac_mem_free(device);
}

/**
 * Looks up a device in the plugin's device hash table.
 * Returns the device structure if found, NULL otherwise.
 *
 * @param plugin
 *     The RDPECAM plugin instance.
 *
 * @param device_name
 *     Name of the device to look up.
 *
 * @return
 *     Pointer to the device structure, or NULL if not found.
 */
static guac_rdpecam_device* guac_rdpecam_device_lookup(
        guac_rdp_rdpecam_plugin* plugin, const char* device_name) {

    if (!plugin || !device_name || !plugin->devices) {
        return NULL;
    }

    return (guac_rdpecam_device*) HashTable_GetItemValue(
        plugin->devices, (void*) device_name);
}

/**
 * Gets device capabilities for a given channel name by extracting the device
 * index from the channel name pattern and looking up capabilities.
 *
 * WARNING: The caller MUST hold rdp_client->lock (read or write) when calling
 * this function and while using the returned pointer. The returned pointer is
 * only valid while the lock is held.
 *
 * @param rdp_client
 *     The RDP client containing device capabilities. The caller must hold
 *     rdp_client->lock.
 *
 * @param channel_name
 *     The channel name (e.g., "RDCamera_Device_0").
 *
 * @return
 *     Pointer to device capabilities if found, NULL otherwise. Valid only
 *     while rdp_client->lock is held by the caller.
 */
static guac_rdp_rdpecam_device_caps* guac_rdp_rdpecam_get_device_caps(
        guac_rdp_client* rdp_client, const char* channel_name) {

    if (!rdp_client || !channel_name)
        return NULL;

    /* Extract device index from channel name (e.g., "RDCamera_Device_0" -> 0) */
    unsigned int device_index = 0;
    if (sscanf(channel_name, "RDCamera_Device_%u", &device_index) != 1)
        return NULL;

    /* Caller must hold lock - we don't acquire/release it here */
    if (device_index < rdp_client->rdpecam_device_caps_count)
        return &rdp_client->rdpecam_device_caps[device_index];

    return NULL;
}

/**
 * Sends DeviceAddedNotification messages for all devices in capabilities.
 * This function creates device ID mappings, registers listeners for device channels,
 * and sends DeviceAddedNotification messages via the enumerator channel.
 *
 * @param plugin
 *     The RDPECAM plugin instance.
 *
 * @param client
 *     The guac_client instance.
 *
 * @param rdp_client
 *     The RDP client data (must have lock held).
 *
 * @param enumerator_channel
 *     The enumerator channel to send notifications through.
 */
void guac_rdp_rdpecam_send_device_notifications(
        guac_rdp_rdpecam_plugin* plugin, guac_client* client,
        guac_rdp_client* rdp_client, IWTSVirtualChannel* enumerator_channel) {

    if (!plugin || !client || !rdp_client || !enumerator_channel)
        return;

    unsigned int device_count = rdp_client->rdpecam_device_caps_count;

    if (device_count == 0) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "RDPECAM no devices to announce");
        return;
    }

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM sending DeviceAddedNotification for %u device(s)", device_count);

    /* Send DeviceAddedNotification for each device */
    for (unsigned int i = 0; i < device_count; i++) {
        guac_rdp_rdpecam_device_caps* caps = &rdp_client->rdpecam_device_caps[i];
        
        /* Generate channel name: "RDCamera_Device_N" */
        char channel_name[64];
        snprintf(channel_name, sizeof(channel_name), "RDCamera_Device_%u", i);
        
        /* Get device name with fallback */
        const char* device_name = "Redirected-Cam0";
        char fallback_name[64];
        if (caps->device_name && caps->device_name[0] != '\0') {
            device_name = caps->device_name;
        } else {
            snprintf(fallback_name, sizeof(fallback_name), "Redirected-Cam%u", i);
            device_name = fallback_name;
        }
        
        /* Store device ID to channel name mapping */
        if (caps->device_id && caps->device_id[0] != '\0' && plugin->device_id_map) {
            char* channel_name_copy = guac_mem_alloc(strlen(channel_name) + 1);
            if (channel_name_copy) {
                strcpy(channel_name_copy, channel_name);
#ifdef HAVE_WINPR_HASHTABLE_INSERT
                if (!HashTable_Insert(plugin->device_id_map, (void*) caps->device_id, (void*) channel_name_copy)) {
                    guac_client_log(client, GUAC_LOG_ERROR,
                            "RDPECAM failed to insert device ID mapping");
                    guac_mem_free(channel_name_copy);
                } else {
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM mapped device ID '%s' to channel '%s'",
                            caps->device_id, channel_name);
                }
#else
                /* Fallback for older WinPR versions without HashTable_Insert */
                if (HashTable_Add(plugin->device_id_map, (void*) caps->device_id, (void*) channel_name_copy) < 0) {
                    guac_client_log(client, GUAC_LOG_ERROR,
                            "RDPECAM failed to add device ID mapping");
                    guac_mem_free(channel_name_copy);
                } else {
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM mapped device ID '%s' to channel '%s'",
                            caps->device_id, channel_name);
                }
#endif
            }
        }
        
        /* Create listener for this device channel if not Device_0 (Device_0 is pre-created) */
        if (i > 0 && plugin->manager) {
            guac_rdp_rdpecam_listener_callback* device_listener =
                guac_mem_zalloc(sizeof(guac_rdp_rdpecam_listener_callback));
            if (device_listener) {
                /* Allocate and copy channel name for listener */
                char* saved_channel_name = guac_mem_alloc(strlen(channel_name) + 1);
                if (saved_channel_name) {
                    strcpy(saved_channel_name, channel_name);
                    device_listener->client = client;
                    device_listener->channel_name = saved_channel_name;
                    device_listener->plugin = plugin;
                    device_listener->parent.OnNewChannelConnection = guac_rdp_rdpecam_new_connection;
                    
                    /* Register listener for this device channel */
                    plugin->manager->CreateListener(plugin->manager, channel_name, 0,
                            (IWTSListenerCallback*) device_listener, NULL);
                    
                    guac_client_log(client, GUAC_LOG_DEBUG,
                            "RDPECAM registered listener for device channel: %s", channel_name);
                } else {
                    guac_mem_free(device_listener);
                }
            }
        }
        
        /* Send DeviceAddedNotification */
        wStream* rs = Stream_New(NULL, 256);
        if (rs && rdpecam_build_device_added(rs, device_name, channel_name)) {
            Stream_SealLength(rs);
            const size_t out_len = Stream_Length(rs);
            
            UINT32 enum_channel_id = 0;
            if (plugin->manager && plugin->manager->GetChannelId)
                enum_channel_id = plugin->manager->GetChannelId(enumerator_channel);
            
            guac_rdp_rdpecam_log_stream(client, "TX", "RDCamera_Device_Enumerator", enum_channel_id, rs);
            pthread_mutex_lock(&(rdp_client->message_lock));
            UINT result = enumerator_channel->Write(enumerator_channel, (UINT32) out_len, Stream_Buffer(rs), NULL);
            pthread_mutex_unlock(&(rdp_client->message_lock));
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM TX ChannelId=%" PRIu32 " MessageId=0x05 DeviceAddedNotification (device='%s', channel='%s')", 
                    enum_channel_id, device_name, channel_name);
        }
        if (rs) Stream_Free(rs, TRUE);
    }
}

/**
 * Entry point for RDPECAM dynamic virtual channel.
 */
UINT DVCPluginEntry(IDRDYNVC_ENTRY_POINTS* pEntryPoints) {

    /* Pull guac_client from arguments */
#ifdef PLUGIN_DATA_CONST
    const ADDIN_ARGV* args = pEntryPoints->GetPluginData(pEntryPoints);
#else
    ADDIN_ARGV* args = pEntryPoints->GetPluginData(pEntryPoints);
#endif

    guac_client* client = (guac_client*) guac_rdp_string_to_ptr(args->argv[1]);

    /* Pull previously-allocated plugin */
    guac_rdp_rdpecam_plugin* rdpecam_plugin = (guac_rdp_rdpecam_plugin*)
        pEntryPoints->GetPlugin(pEntryPoints, GUAC_RDPECAM_PLUGIN_NAME);

    /* If no such plugin allocated, allocate and register it now */
    if (rdpecam_plugin == NULL) {

        /* Init plugin callbacks and data */
        rdpecam_plugin = guac_mem_zalloc(sizeof(guac_rdp_rdpecam_plugin));
        rdpecam_plugin->parent.Initialize = guac_rdp_rdpecam_initialize;
        rdpecam_plugin->parent.Terminated = guac_rdp_rdpecam_terminated;
        rdpecam_plugin->client = client;
        rdpecam_plugin->version_negotiated = false;

        /* Store plugin reference in rdp_client for access from callbacks */
        guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
        if (rdp_client) {
            rdp_client->rdpecam_plugin = rdpecam_plugin;
            /* Register immediate caps notify callback */
            rdp_client->rdpecam_caps_notify = guac_rdp_rdpecam_caps_notify;
        }

        /* Register plugin for later retrieval */
        pEntryPoints->RegisterPlugin(pEntryPoints, GUAC_RDPECAM_PLUGIN_NAME,
                (IWTSPlugin*) rdpecam_plugin);

        guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM plugin loaded.");
    }

    return CHANNEL_RC_OK;

}
