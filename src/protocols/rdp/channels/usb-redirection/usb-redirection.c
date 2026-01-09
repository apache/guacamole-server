/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.
 */

#include "config.h"
#include "channels/usb-redirection/usb-redirection.h"
#include "channels/usb-redirection/usb-manager.h"
#include "plugins/guacusb/guacusb.h"
#include "plugins/channels.h"
#include "plugins/ptr-string.h"
#include "rdp.h"

#include <guacamole/client.h>
#include <guacamole/user.h>
#include <guacamole/mem.h>
#include <freerdp/freerdp.h>
#include <stdlib.h>
#include <string.h>

int guac_rdp_user_usbconnect_handler(guac_user* user, 
        const char* device_id, int vendor_id, int product_id,
        const char* device_name, const char* serial_number,
        int device_class, int device_subclass, int device_protocol,
        const char* interface_data) {
    
    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    
    // ... existing validation code ...
    
    /* Add device to manager */
    int result = guac_rdp_usb_manager_add_device(rdp_client->usb_manager,
            device_id, vendor_id, product_id, device_name, serial_number,
            device_class, device_subclass, device_protocol, interface_data);
    
    if (result != 0) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Failed to add USB device %s to manager", device_id);
        return 1;
    }
    
    guac_client_log(client, GUAC_LOG_INFO,
            "USB device %s added to manager successfully", device_id);
    
    /* Check if we can send to RDP server */
    if (rdp_client->usb_plugin) {
        if (rdp_client->usb_plugin->channel) {
            guac_client_log(client, GUAC_LOG_INFO,
                    "USB: Channel available, device will be sent to RDP server");
            // TODO: Send device when channel protocol is implemented
        }
        else {
            guac_client_log(client, GUAC_LOG_INFO,
                    "USB: Waiting for URBDRC channel to connect. Device queued.");
        }
    }
    else {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "USB: Plugin not yet loaded. Device queued.");
    }
    
    return 0;
}

int guac_rdp_user_usbdisconnect_handler(guac_user* user, 
        const char* device_id) {
    
    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    
    /* Verify USB support is enabled */
    if (!rdp_client->settings->usb_enabled) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "USB redirection is not enabled for this connection");
        return 0;
    }
    
    /* Check if manager exists */
    if (!rdp_client->usb_manager) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "USB manager not initialized");
        return 0;
    }
    
    guac_client_log(client, GUAC_LOG_INFO,
            "USB device disconnect: %s", device_id);
    
    /* Remove device from manager */
    int result = guac_rdp_usb_manager_remove_device(
            rdp_client->usb_manager, device_id);
    
    if (result != 0) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "USB device %s not found", device_id);
    }
    else {
        guac_client_log(client, GUAC_LOG_INFO,
                "USB device %s removed from manager", device_id);
    }
    
    return 0;
}

int guac_rdp_user_usbdata_handler(guac_user* user, const char* device_id,
        int endpoint_number, const char* data, const char* transfer_type) {
    
    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    
    /* Verify USB support is enabled */
    if (!rdp_client->settings->usb_enabled) {
        return 0;
    }
    
    /* Get plugin reference */
    guac_rdp_usb_plugin* plugin = rdp_client->usb_plugin;
    if (!plugin) {
        return 0;
    }
    
    /* Calculate data length from base64 encoded string */
    size_t data_len = data ? strlen(data) : 0;
    size_t decoded_len = (data_len * 3) / 4;  /* Approximate decoded size */
    
    guac_client_log(client, GUAC_LOG_TRACE,
            "USB data for device %s: endpoint %d, type %s, ~%zu bytes",
            device_id, endpoint_number, transfer_type, decoded_len);
    
    /* TODO: Forward data to device through plugin */
    /* This will be implemented when channel data forwarding is ready */
    
    return 0;
}

void guac_rdp_usb_load_plugin(rdpContext* context) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    
    /* Only load if USB is enabled and manager exists */
    if (!rdp_client->settings->usb_enabled || !rdp_client->usb_manager) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "USB redirection disabled or manager not initialized");
        return;
    }
    
    /* Add "guacusb" channel with client reference */
    char client_ref[GUAC_RDP_PTR_STRING_LENGTH];
    guac_rdp_ptr_to_string(client, client_ref);
    guac_freerdp_dynamic_channel_collection_add(context->settings,
            "guacusb", client_ref, NULL);
    
    guac_client_log(client, GUAC_LOG_DEBUG,
            "USB redirection plugin scheduled for loading");
}