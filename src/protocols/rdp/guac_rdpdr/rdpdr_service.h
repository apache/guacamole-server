/*
 * Copyright (C) 2013 Glyptodon LLC
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
     * An arbitrary device name, used for logging purposes only.
     */
    const char* device_name;

    /**
     * Handler which will be called when the RDPDR plugin is forming the client
     * device announce list.
     */
    guac_rdpdr_device_announce_handler* announce_handler;

    /**
     * Handler which should be called for every I/O request received.
     */
    guac_rdpdr_device_iorequest_handler* iorequest_handler;

    /**
     * Handlel which should be called when the device is being free'd.
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
 * Creates a new stream which contains the ommon DR_DEVICE_IOCOMPLETION header
 * used for virtually all responses.
 */
wStream* guac_rdpdr_new_io_completion(guac_rdpdr_device* device,
        int completion_id, int status, int size);

/**
 * Begins streaming the given file to the user via a Guacamole file stream.
 */
void guac_rdpdr_start_download(guac_rdpdr_device* device, const char* path);

#endif

