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

#ifndef GUAC_RDP_CHANNELS_USB_MANAGER_H
#define GUAC_RDP_CHANNELS_USB_MANAGER_H

#include <guacamole/client.h>
#include <winpr/wtypes.h>
#include <pthread.h>

/**
 * Maximum number of USB devices that can be tracked simultaneously.
 */
#define GUAC_USB_MAX_DEVICES 16

/**
 * USB device information structure containing all metadata about a
 * connected USB device.
 */
typedef struct guac_rdp_usb_device {
    
    /**
     * Internal device ID assigned by the manager.
     */
    UINT32 device_id;
    
    /**
     * WebUSB device ID from the client.
     */
    char web_usb_id[128];
    
    /**
     * USB vendor ID.
     */
    UINT16 vendor_id;
    
    /**
     * USB product ID.
     */
    UINT16 product_id;
    
    /**
     * Human-readable device name.
     */
    char device_name[256];
    
    /**
     * Device serial number.
     */
    char serial_number[128];
    
    /**
     * USB device class code.
     */
    UINT8 device_class;
    
    /**
     * USB device subclass code.
     */
    UINT8 device_subclass;
    
    /**
     * USB device protocol code.
     */
    UINT8 device_protocol;
    
    /**
     * Interface descriptor data (base64 encoded).
     */
    char* interface_data;
    
    /**
     * Size of interface data.
     */
    size_t interface_data_size;
    
    /**
     * Whether this device slot is in use.
     */
    BOOL in_use;
    
    /**
     * Whether this device has been sent to the RDP server.
     */
    BOOL already_sent;
    
} guac_rdp_usb_device;

/**
 * USB device manager structure that tracks all connected USB devices
 * and their state.
 */
typedef struct guac_rdp_usb_manager {
    
    /**
     * Array of USB devices being tracked.
     */
    guac_rdp_usb_device devices[GUAC_USB_MAX_DEVICES];
    
    /**
     * Next device ID to assign to a new device.
     */
    UINT32 next_device_id;
    
    /**
     * Mutex for thread-safe access to device list.
     */
    pthread_mutex_t mutex;
    
    /**
     * Reference to the Guacamole client for logging.
     */
    guac_client* client;
    
} guac_rdp_usb_manager;

/**
 * Callback function type for iterating over devices.
 *
 * @param device
 *     The current device being processed.
 *
 * @param data
 *     User-provided data passed to the iteration function.
 */
typedef void (*guac_rdp_usb_device_callback)(
        guac_rdp_usb_device* device, void* data);

/**
 * Allocate and initialize a new USB device manager.
 *
 * @param client
 *     The Guacamole client for logging.
 *
 * @return
 *     A newly allocated USB device manager, or NULL on failure.
 */
guac_rdp_usb_manager* guac_rdp_usb_manager_alloc(guac_client* client);

/**
 * Free a USB device manager and all associated resources.
 *
 * @param manager
 *     The USB device manager to free.
 */
void guac_rdp_usb_manager_free(guac_rdp_usb_manager* manager);

/**
 * Add a USB device to the manager.
 *
 * @param manager
 *     The USB device manager.
 *
 * @param web_usb_id
 *     The WebUSB device identifier from the client.
 *
 * @param vendor_id
 *     The USB vendor ID.
 *
 * @param product_id
 *     The USB product ID.
 *
 * @param device_name
 *     Human-readable device name, or NULL if unknown.
 *
 * @param serial_number
 *     Device serial number, or NULL if unknown.
 *
 * @param device_class
 *     USB device class code.
 *
 * @param device_subclass
 *     USB device subclass code.
 *
 * @param device_protocol
 *     USB device protocol code.
 *
 * @param interface_data
 *     Interface descriptor data (base64), or NULL if not available.
 *
 * @return
 *     0 on success, non-zero on failure.
 */
int guac_rdp_usb_manager_add_device(guac_rdp_usb_manager* manager,
        const char* web_usb_id, int vendor_id, int product_id,
        const char* device_name, const char* serial_number,
        int device_class, int device_subclass, int device_protocol,
        const char* interface_data);

/**
 * Remove a USB device from the manager.
 *
 * @param manager
 *     The USB device manager.
 *
 * @param web_usb_id
 *     The WebUSB device identifier to remove.
 *
 * @return
 *     0 on success, non-zero if device not found.
 */
int guac_rdp_usb_manager_remove_device(guac_rdp_usb_manager* manager,
        const char* web_usb_id);

/**
 * Get a USB device by WebUSB ID.
 *
 * @param manager
 *     The USB device manager.
 *
 * @param web_usb_id
 *     The WebUSB device identifier to find.
 *
 * @return
 *     Pointer to the device structure, or NULL if not found.
 *     The returned pointer is only valid while the manager mutex is held.
 */
guac_rdp_usb_device* guac_rdp_usb_manager_get_device(
        guac_rdp_usb_manager* manager, const char* web_usb_id);

/**
 * Get a USB device by internal device ID.
 *
 * @param manager
 *     The USB device manager.
 *
 * @param device_id
 *     The internal device ID to find.
 *
 * @return
 *     Pointer to the device structure, or NULL if not found.
 *     The returned pointer is only valid while the manager mutex is held.
 */
guac_rdp_usb_device* guac_rdp_usb_manager_get_device_by_id(
        guac_rdp_usb_manager* manager, UINT32 device_id);

/**
 * Iterate over all active devices in the manager.
 *
 * @param manager
 *     The USB device manager.
 *
 * @param callback
 *     Function to call for each active device.
 *
 * @param data
 *     User data to pass to the callback function.
 */
void guac_rdp_usb_manager_foreach_device(
        guac_rdp_usb_manager* manager,
        guac_rdp_usb_device_callback callback, void* data);

/**
 * Mark a device as sent to the RDP server.
 *
 * @param manager
 *     The USB device manager.
 *
 * @param device_id
 *     The internal device ID to mark as sent.
 */
void guac_rdp_usb_manager_mark_device_sent(
        guac_rdp_usb_manager* manager,
        UINT32 device_id);

/**
 * Get the number of active devices.
 *
 * @param manager
 *     The USB device manager.
 *
 * @return
 *     The number of devices currently being tracked.
 */
int guac_rdp_usb_manager_get_device_count(
        guac_rdp_usb_manager* manager);

#endif