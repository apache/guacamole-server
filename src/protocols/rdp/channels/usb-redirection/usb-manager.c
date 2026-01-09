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
#include "channels/usb-redirection/usb-manager.h"

#include <guacamole/mem.h>
#include <guacamole/client.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

guac_rdp_usb_manager* guac_rdp_usb_manager_alloc(guac_client* client) {
    
    if (!client)
        return NULL;
    
    guac_rdp_usb_manager* manager = guac_mem_zalloc(
            sizeof(guac_rdp_usb_manager));
    if (!manager) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Failed to allocate USB device manager");
        return NULL;
    }
    
    manager->client = client;
    manager->next_device_id = 1;
    
    /* Initialize mutex */
    if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Failed to initialize USB manager mutex");
        guac_mem_free(manager);
        return NULL;
    }
    
    guac_client_log(client, GUAC_LOG_DEBUG,
            "USB device manager allocated with %d device slots",
            GUAC_USB_MAX_DEVICES);
    
    return manager;
}

void guac_rdp_usb_manager_free(guac_rdp_usb_manager* manager) {
    
    if (!manager)
        return;
    
    pthread_mutex_lock(&manager->mutex);
    
    /* Free any allocated interface data */
    int freed_count = 0;
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        if (manager->devices[i].interface_data) {
            guac_mem_free(manager->devices[i].interface_data);
            manager->devices[i].interface_data = NULL;
            freed_count++;
        }
    }
    
    if (freed_count > 0)
        guac_client_log(manager->client, GUAC_LOG_DEBUG,
                "Freed interface data for %d USB device(s)", freed_count);
    
    pthread_mutex_unlock(&manager->mutex);
    pthread_mutex_destroy(&manager->mutex);
    
    guac_client_log(manager->client, GUAC_LOG_DEBUG,
            "USB device manager freed");
    
    guac_mem_free(manager);
}

int guac_rdp_usb_manager_add_device(guac_rdp_usb_manager* manager,
        const char* web_usb_id, int vendor_id, int product_id,
        const char* device_name, const char* serial_number,
        int device_class, int device_subclass, int device_protocol,
        const char* interface_data) {
    
    if (!manager || !web_usb_id)
        return 1;
    
    /* Validate web_usb_id length */
    if (strlen(web_usb_id) >= sizeof(manager->devices[0].web_usb_id)) {
        guac_client_log(manager->client, GUAC_LOG_ERROR,
                "USB device ID too long: %s", web_usb_id);
        return 1;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    /* Check if device already exists */
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        if (manager->devices[i].in_use &&
            strcmp(manager->devices[i].web_usb_id, web_usb_id) == 0) {
            
            guac_client_log(manager->client, GUAC_LOG_WARNING,
                    "USB device %s already exists (ID: %d)",
                    web_usb_id, manager->devices[i].device_id);
            
            pthread_mutex_unlock(&manager->mutex);
            return 1;
        }
    }
    
    /* Find free slot */
    guac_rdp_usb_device* device = NULL;
    int slot_index = -1;
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        if (!manager->devices[i].in_use) {
            device = &manager->devices[i];
            slot_index = i;
            break;
        }
    }
    
    if (!device) {
        guac_client_log(manager->client, GUAC_LOG_ERROR,
                "No free USB device slots available (max: %d)",
                GUAC_USB_MAX_DEVICES);
        pthread_mutex_unlock(&manager->mutex);
        return 1;
    }
    
    /* Clear device structure */
    memset(device, 0, sizeof(guac_rdp_usb_device));
    
    /* Assign device ID and basic info */
    device->device_id = manager->next_device_id++;
    strncpy(device->web_usb_id, web_usb_id, sizeof(device->web_usb_id) - 1);
    device->vendor_id = vendor_id & 0xFFFF;
    device->product_id = product_id & 0xFFFF;
    device->device_class = device_class & 0xFF;
    device->device_subclass = device_subclass & 0xFF;
    device->device_protocol = device_protocol & 0xFF;
    
    /* Copy optional strings */
    if (device_name) {
        strncpy(device->device_name, device_name, 
                sizeof(device->device_name) - 1);
        device->device_name[sizeof(device->device_name) - 1] = '\0';
    }
    
    if (serial_number) {
        strncpy(device->serial_number, serial_number, 
                sizeof(device->serial_number) - 1);
        device->serial_number[sizeof(device->serial_number) - 1] = '\0';
    }
    
    /* Copy interface data if provided */
    if (interface_data && *interface_data) {
        size_t data_len = strlen(interface_data) + 1;
        device->interface_data = guac_mem_alloc(data_len);
        if (device->interface_data) {
            memcpy(device->interface_data, interface_data, data_len);
            device->interface_data_size = data_len;
        }
        
        else
            guac_client_log(manager->client, GUAC_LOG_WARNING,
                    "Failed to allocate interface data for device %s",
                    web_usb_id);
    }
    
    /* Mark as active */
    device->in_use = TRUE;
    device->already_sent = FALSE;
    
    guac_client_log(manager->client, GUAC_LOG_INFO,
            "Added USB device %s to slot %d (ID: %d, VID: 0x%04X, PID: 0x%04X, Class: 0x%02X)",
            web_usb_id, slot_index, device->device_id,
            device->vendor_id, device->product_id, device->device_class);
    
    if (device->device_name[0])
        guac_client_log(manager->client, GUAC_LOG_DEBUG,
                "  Device name: %s", device->device_name);
    
    if (device->serial_number[0])
        guac_client_log(manager->client, GUAC_LOG_DEBUG,
                "  Serial number: %s", device->serial_number);
    
    pthread_mutex_unlock(&manager->mutex);
    return 0;
}

int guac_rdp_usb_manager_remove_device(
        guac_rdp_usb_manager* manager, const char* web_usb_id) {
    
    if (!manager || !web_usb_id)
        return 1;
    
    pthread_mutex_lock(&manager->mutex);
    
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        guac_rdp_usb_device* device = &manager->devices[i];
        
        if (device->in_use && 
            strcmp(device->web_usb_id, web_usb_id) == 0) {
            
            UINT32 device_id = device->device_id;
            char device_name[256];
            strncpy(device_name, device->device_name, sizeof(device_name) - 1);
            device_name[sizeof(device_name) - 1] = '\0';
            
            /* Free interface data if allocated */
            if (device->interface_data) {
                guac_mem_free(device->interface_data);
                device->interface_data = NULL;
            }
            
            /* Clear device structure */
            memset(device, 0, sizeof(guac_rdp_usb_device));
            
            guac_client_log(manager->client, GUAC_LOG_INFO,
                    "Removed USB device %s (ID: %d%s%s)",
                    web_usb_id, device_id,
                    device_name[0] ? ", " : "",
                    device_name[0] ? device_name : "");
            
            pthread_mutex_unlock(&manager->mutex);
            return 0;
        }
    }
    
    guac_client_log(manager->client, GUAC_LOG_WARNING,
            "USB device %s not found for removal", web_usb_id);
    
    pthread_mutex_unlock(&manager->mutex);
    return 1;
}

guac_rdp_usb_device* guac_rdp_usb_manager_get_device(
        guac_rdp_usb_manager* manager, const char* web_usb_id) {
    
    if (!manager || !web_usb_id)
        return NULL;
    
    /* Note: Caller should hold mutex if needed */
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        guac_rdp_usb_device* device = &manager->devices[i];
        if (device->in_use &&
            strcmp(device->web_usb_id, web_usb_id) == 0) {
            return device;
        }
    }
    
    return NULL;
}

guac_rdp_usb_device* guac_rdp_usb_manager_get_device_by_id(
        guac_rdp_usb_manager* manager, UINT32 device_id) {
    
    if (!manager)
        return NULL;
    
    /* Note: Caller should hold mutex if needed */
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        guac_rdp_usb_device* device = &manager->devices[i];
        if (device->in_use && device->device_id == device_id)
            return device;
    }
    
    return NULL;
}

void guac_rdp_usb_manager_foreach_device(
        guac_rdp_usb_manager* manager, 
        guac_rdp_usb_device_callback callback, void* data) {
    
    if (!manager || !callback)
        return;
    
    pthread_mutex_lock(&manager->mutex);
    
    int processed = 0;
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        if (manager->devices[i].in_use) {
            callback(&manager->devices[i], data);
            processed++;
        }
    }
    
    guac_client_log(manager->client, GUAC_LOG_TRACE,
            "Processed %d USB device(s) with callback", processed);
    
    pthread_mutex_unlock(&manager->mutex);
}

void guac_rdp_usb_manager_mark_device_sent(
        guac_rdp_usb_manager* manager, UINT32 device_id) {
    
    if (!manager)
        return;
    
    pthread_mutex_lock(&manager->mutex);
    
    guac_rdp_usb_device* device = guac_rdp_usb_manager_get_device_by_id(
            manager, device_id);
    
    if (device) {
        device->already_sent = TRUE;
        guac_client_log(manager->client, GUAC_LOG_DEBUG,
                "Marked USB device %s (ID: %d) as sent to server",
                device->web_usb_id, device_id);
    } 
    
    else
        guac_client_log(manager->client, GUAC_LOG_WARNING,
                "Cannot mark device ID %d as sent - device not found",
                device_id);
    
    pthread_mutex_unlock(&manager->mutex);
}

int guac_rdp_usb_manager_get_device_count(
        guac_rdp_usb_manager* manager) {
    
    if (!manager)
        return 0;

    pthread_mutex_lock(&manager->mutex);
    
    int count = 0;
    for (int i = 0; i < GUAC_USB_MAX_DEVICES; i++) {
        if (manager->devices[i].in_use)
            count++;
    }
    
    pthread_mutex_unlock(&manager->mutex);
    
    return count;
}