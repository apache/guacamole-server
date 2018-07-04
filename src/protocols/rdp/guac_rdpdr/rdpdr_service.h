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


#ifndef __GUAC_RDPDR_SERVICE_H
#define __GUAC_RDPDR_SERVICE_H

#include "config.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * The maximum number of bytes to allow for a device read.
 */
#define GUAC_RDP_MAX_READ_BUFFER 4194304

typedef struct guac_rdpdrPlugin guac_rdpdrPlugin;
typedef struct guac_rdpdr_device guac_rdpdr_device;

/**
 * Handler for client device list announce. Each implementing device must write
 * its announcement header and data to the given output stream.
 */
typedef void guac_rdpdr_device_announce_handler(guac_rdpdr_device* device, wStream* output_stream,
        int device_id);

/**
 * Handler for device I/O requests.
 */
typedef void guac_rdpdr_device_iorequest_handler(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int major_func, int minor_func);

/**
 * Handler for cleaning up the dynamically-allocated portions of a device.
 */
typedef void guac_rdpdr_device_free_handler(guac_rdpdr_device* device);

/**
 * Arbitrary device forwarded over the RDPDR channel.
 */
struct guac_rdpdr_device {

    /**
     * The RDPDR plugin owning this device.
     */
    guac_rdpdrPlugin* rdpdr;

    /**
     * The ID assigned to this device by the RDPDR plugin.
     */
    int device_id;

    /**
     * Device name, used for logging and for passthrough to the
     * server.
     */
    const char* device_name;

    /**
     * The type of RDPDR device that this represents.
     */
    uint32_t device_type;

    /**
     * The DOS name of the device.  Max 8 bytes, including terminator.
     */
    const char *dos_name;
    
    /**
     * The stream that stores the RDPDR device announcement for this device.
     */
    wStream* device_announce;
    
    /**
     * The length of the device_announce wStream.
     */
    int device_announce_len;

    /**
     * Handler which should be called for every I/O request received.
     */
    guac_rdpdr_device_iorequest_handler* iorequest_handler;

    /**
     * Handler which should be called when the device is being freed.
     */
    guac_rdpdr_device_free_handler* free_handler;

    /**
     * Arbitrary data, used internally by the handlers for this device.
     */
    void* data;

};

/**
 * Structure representing the current state of the Guacamole RDPDR plugin for
 * FreeRDP.
 */
struct guac_rdpdrPlugin {

    /**
     * The FreeRDP parts of this plugin. This absolutely MUST be first.
     * FreeRDP depends on accessing this structure as if it were an instance
     * of rdpSvcPlugin.
     */
    rdpSvcPlugin plugin;

    /**
     * Reference to the client owning this instance of the RDPDR plugin.
     */
    guac_client* client;

    /**
     * The number of devices registered within the devices array.
     */
    int devices_registered;

    /**
     * Array of registered devices.
     */
    guac_rdpdr_device devices[8];

};

/**
 * Handler called when this plugin is loaded by FreeRDP.
 */
void guac_rdpdr_process_connect(rdpSvcPlugin* plugin);

/**
 * Handler called when this plugin receives data along its designated channel.
 */
void guac_rdpdr_process_receive(rdpSvcPlugin* plugin,
        wStream* input_stream);

/**
 * Handler called when this plugin is being unloaded.
 */
void guac_rdpdr_process_terminate(rdpSvcPlugin* plugin);

/**
 * Handler called when this plugin receives an event. For the sake of RDPDR,
 * all events will be ignored and simply free'd.
 */
void guac_rdpdr_process_event(rdpSvcPlugin* plugin, wMessage* event);

/**
 * Creates a new stream which contains the common DR_DEVICE_IOCOMPLETION header
 * used for virtually all responses.
 */
wStream* guac_rdpdr_new_io_completion(guac_rdpdr_device* device,
        int completion_id, int status, int size);

/**
 * Begins streaming the given file to the user via a Guacamole file stream.
 */
void guac_rdpdr_start_download(guac_rdpdr_device* device, char* path);

#endif

