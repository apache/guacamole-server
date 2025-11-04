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

#ifndef GUAC_RDP_PLUGINS_GUACRDPECAM_H
#define GUAC_RDP_PLUGINS_GUACRDPECAM_H

#include <freerdp/constants.h>
#include <freerdp/dvc.h>
#include <freerdp/freerdp.h>
#include <guacamole/argv-fntypes.h>
#include <guacamole/client.h>
#include <guacamole/user.h>
#include <pthread.h>

#include "channels/rdpecam/rdpecam_sink.h"
#include "rdpecam_proto.h"

/* Forward declaration */
typedef struct guac_rdp_client guac_rdp_client;

/**
 * The name of the RDPECAM control/enumeration dynamic virtual channel.
 * This MUST match the MS-RDPECAM specification.
 */
#define GUAC_RDPECAM_CHANNEL_NAME "RDCamera_Device_Enumerator"

/**
 * Temporary device channel name for the first virtual camera device.
 * This will be created as a separate listener. In a full implementation,
 * device channels are named dynamically by deviceId.
 * Windows expects the format "RDCamera_Device_N" where N is the device index.
 */
#define GUAC_RDPECAM_DEVICE0_CHANNEL_NAME "RDCamera_Device_0"

/**
 * The name of the guacrdpecam plugin.
 */
#define GUAC_RDPECAM_PLUGIN_NAME "guacrdpecam"

/**
 * Extended version of the IWTSListenerCallback structure, providing additional
 * access to Guacamole-specific data. The IWTSListenerCallback provides access
 * to callbacks related to the receipt of new connections to the RDPECAM
 * channel.
 */
typedef struct guac_rdp_rdpecam_listener_callback {

    /**
     * The parent IWTSListenerCallback structure that this structure extends.
     * THIS MEMBER MUST BE FIRST!
     */
    IWTSListenerCallback parent;

    /**
     * The guac_client instance associated with the RDP connection using the
     * RDPECAM plugin.
     */
    guac_client* client;

    /**
     * The channel name this listener is registered for.
     */
    const char* channel_name;

    /** Back-reference to the RDPECAM plugin. */
    struct guac_rdp_rdpecam_plugin* plugin;

} guac_rdp_rdpecam_listener_callback;

/**
 * Device state structure for multi-device support.
 * Each connected camera device has one instance of this structure,
 * managed by the plugin's hash table indexed by device/channel name.
 */
typedef struct guac_rdpecam_device {

    /**
     * Device/channel name (e.g., "RDCamera_Device_0").
     * This is the key for the hash table lookup.
     */
    char* device_name;

    /**
     * Browser device ID from navigator.mediaDevices.
     * Used to map between browser devices and Windows channels.
     */
    char* browser_device_id;

    /**
     * The current active virtual channel for this device's streaming data.
     * Only set while a streaming-capable channel is connected.
     */
    IWTSVirtualChannel* stream_channel;

    /** Cached numeric channel identifier for the current stream channel. */
    uint32_t stream_channel_id;

    /**
     * Per-device frame sink for buffering video frames.
     * Independent queue for each device.
     */
    guac_rdpecam_sink* sink;

    /**
     * Per-device dequeue thread for encoding and transmitting frames.
     * Each device has its own thread reading from its own sink.
     */
    pthread_t dequeue_thread;

    /**
     * Whether the dequeue thread has been successfully started.
     * Used to safely join the thread during shutdown.
     */
    bool dequeue_thread_started;

    /**
     * Per-device media type descriptor for the current stream.
     * Independent for each device.
     */
    rdpecam_media_type_desc media_type;

    /**
     * Stream index from 0x0F (START_STREAMS_REQUEST) message.
     * Independent for each device.
     */
    uint32_t stream_index;

    /**
     * Sample credits for flow control (independent per device).
     * Only this device's threads decrement this counter.
     */
    uint32_t credits;

    /**
     * Monotonic sample sequence value used for outgoing samples.
     */
    uint32_t sample_sequence;

    /**
     * Whether this device is the active sender.
     * Only one device may be actively sending frames per plugin session.
     */
    bool is_active_sender;

    /**
     * Whether streaming is currently active for this device.
     */
    bool streaming;

    /**
     * Whether the next frame must be a keyframe before streaming resumes.
     */
    bool need_keyframe;

    /**
     * Signal to stop the dequeue thread.
     * Set by channel close handler, checked by dequeue thread.
     */
    bool stopping;

    /**
     * Reference count for handling multiple channel opens.
     * Allows device to persist across channel reconnections.
     */
    int ref_count;

    /**
     * Mutex protecting all per-device fields.
     * Ensures thread-safe access to credits, streaming state, etc.
     */
    pthread_mutex_t lock;

    /**
     * Condition variable for signaling credit availability.
     * Woken when new sample credits arrive via SAMPLE_REQUEST.
     */
    pthread_cond_t credits_signal;

} guac_rdpecam_device;

/**
 * Extended version of the IWTSVirtualChannelCallback structure, providing
 * additional access to Guacamole-specific data. The IWTSVirtualChannelCallback
 * provides access to callbacks related to an active connection to the
 * RDPECAM channel, including receipt of data. 
 */
typedef struct guac_rdp_rdpecam_channel_callback {

    /**
     * The parent IWTSVirtualChannelCallback structure that this structure
     * extends. THIS MEMBER MUST BE FIRST!
     */
    IWTSVirtualChannelCallback parent;

    /**
     * The actual virtual channel instance along which the RDPECAM plugin
     * should send any responses.
     */
    IWTSVirtualChannel* channel;

    /**
     * The guac_client instance associated with the RDP connection using the
     * RDPECAM plugin.
     */
    guac_client* client;

    /**
     * Pointer to the device state for this channel connection, if any.
     * Obtained from plugin->devices hash table using channel name.
     */
    guac_rdpecam_device* device;

    /**
     * The channel name associated with this callback (control vs device).
     */
    const char* channel_name;

    /** Back-reference to the RDPECAM plugin. */
    struct guac_rdp_rdpecam_plugin* plugin;

    /**
     * Whether this channel is the streaming channel for the device.
     */
    bool is_stream_channel;

    /**
     * The numeric channel identifier reported by FreeRDP, if known.
     */
    uint32_t channel_id;

} guac_rdp_rdpecam_channel_callback;

/**
 * All data associated with Guacamole's RDPECAM plugin for FreeRDP.
 */
typedef struct guac_rdp_rdpecam_plugin {

    /**
     * The parent IWTSPlugin structure that this structure extends. THIS
     * MEMBER MUST BE FIRST!
     */
    IWTSPlugin parent;

    /**
     * The listener callback structure allocated when the RDPECAM plugin
     * was loaded, if any. If the plugin did not fully load, this will be NULL.
     * If non-NULL, this callback structure must be freed when the plugin is
     * terminated.
     */
    guac_rdp_rdpecam_listener_callback* control_listener_callback;
    guac_rdp_rdpecam_listener_callback* device0_listener_callback;

    /**
     * Hash table for managing multiple device channels.
     * Key: device/channel name (e.g., "RDCamera_Device_0")
     * Value: guac_rdpecam_device* (per-device state)
     * 
     * Replaces the old single-device architecture where only one device
     * state could exist at a time.
     */
    wHashTable* devices;

    /**
     * Hash table mapping browser device IDs to Windows channel names.
     * Key: browser device ID (from navigator.mediaDevices)
     * Value: channel name (e.g., "RDCamera_Device_0")
     * 
     * Used to route camera-start signals from Windows channel selection
     * back to the correct browser device.
     */
    wHashTable* device_id_map;

    /**
     * The guac_client instance associated with the RDP connection using the
     * RDPECAM plugin.
     */
    guac_client* client;

    /**
     * Virtual channel manager retained for creating additional listeners
     * (per-device channels) after initialization.
     */
    IWTSVirtualChannelManager* manager;

    /** Enumerator channel (RDCamera_Device_Enumerator) for notifications. */
    IWTSVirtualChannel* enumerator_channel;

    /**
     * Whether version negotiation has completed (SelectVersionResponse received).
     * Used to determine when to send DeviceAddedNotification messages.
     */
    bool version_negotiated;

} guac_rdp_rdpecam_plugin;

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
        guac_rdp_client* rdp_client, IWTSVirtualChannel* enumerator_channel);

#endif
