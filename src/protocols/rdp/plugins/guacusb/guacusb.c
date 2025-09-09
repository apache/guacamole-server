/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.
 */

#include "plugins/guacusb/guacusb.h"
#include "plugins/guacusb/guacusb-messages.h"
#include "channels/usb-redirection/usb-manager.h"
#include "plugins/ptr-string.h"
#include "rdp.h"

#include <freerdp/dvc.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <winpr/stream.h>
#include <stdint.h>

/**
 * Process incoming data from the URBDRC channel
 */
static UINT guac_rdp_usb_on_data_received(
        IWTSVirtualChannelCallback* channel_callback, 
        wStream* data) {
    
    guac_rdp_usb_channel_callback* callback = 
            (guac_rdp_usb_channel_callback*) channel_callback;
    guac_rdp_usb_plugin* plugin = callback->plugin;
    guac_client* client = plugin->client;
    
    uint32_t InterfaceId, MessageId, FunctionId;
    
    if (!Stream_CheckAndLogRequiredLength("guacusb", data, 12))
        return ERROR_INVALID_DATA;
    
    Stream_Read_UINT32(data, InterfaceId);
    Stream_Read_UINT32(data, MessageId);
    Stream_Read_UINT32(data, FunctionId);
    
    guac_client_log(client, GUAC_LOG_DEBUG,
            "USB: Received message - Interface=0x%08X, MsgId=%u, Func=0x%03X",
            InterfaceId, MessageId, FunctionId);
    
    if ((InterfaceId & 0x3FFFFFFF) == CAPABILITIES_NEGOTIATOR) {
        // This is the initial handshake from server
        if (FunctionId == RIM_EXCHANGE_CAPABILITY_REQUEST) {
            // Server is starting the handshake
            return guac_rdp_usb_send_capability_response(plugin, 
                    MessageId, RIM_CAPABILITY_VERSION_01);
        }
    }

    /* Mark channel as ready and send test message */
    if (plugin->channel_status == INIT_CHANNEL_IN) {
        plugin->channel_status = INIT_CHANNEL_OUT;
        guac_client_log(client, GUAC_LOG_INFO,
                "USB: Channel ready, sending test message");
        
        /* Test that we can send messages */
        guac_rdp_usb_send_test_message(plugin);
    }
    
    return CHANNEL_RC_OK;
}

/**
 * Channel close handler
 */
static UINT guac_rdp_usb_on_close(IWTSVirtualChannelCallback* channel_callback) {
    
    guac_rdp_usb_channel_callback* callback = 
            (guac_rdp_usb_channel_callback*) channel_callback;
    guac_rdp_usb_plugin* plugin = callback->plugin;
    
    guac_client_log(plugin->client, GUAC_LOG_INFO, "USB: Channel closed");
    
    plugin->channel = NULL;
    plugin->channel_status = INIT_CHANNEL_IN;
    guac_mem_free(callback);
    
    return CHANNEL_RC_OK;
}

/**
 * New channel connection handler
 */
static UINT guac_rdp_usb_on_new_channel_connection(
        IWTSListenerCallback* listener_callback,
        IWTSVirtualChannel* channel,
        BYTE* data,
        BOOL* accept,
        IWTSVirtualChannelCallback** channel_callback) {

    guac_rdp_usb_listener_callback* usb_listener =
            (guac_rdp_usb_listener_callback*) listener_callback;
    guac_rdp_usb_plugin* plugin = usb_listener->plugin;

    /* Accept the connection */
    *accept = TRUE;

    /* Allocate channel callback */
    guac_rdp_usb_channel_callback* usb_callback =
            guac_mem_zalloc(sizeof(guac_rdp_usb_channel_callback));

    usb_callback->client = usb_listener->client;
    usb_callback->channel = channel;
    usb_callback->plugin = plugin;
    usb_callback->parent.OnDataReceived = guac_rdp_usb_on_data_received;
    usb_callback->parent.OnClose = guac_rdp_usb_on_close;

    /* Store channel reference */
    plugin->channel = channel;

    *channel_callback = (IWTSVirtualChannelCallback*) usb_callback;

    guac_client_log(usb_listener->client, GUAC_LOG_INFO,
            "USB: Channel connection established");

    return CHANNEL_RC_OK;
}

/**
 * Plugin initialization
 */
/**
 * Plugin initialization
 */
static UINT guac_rdp_usb_initialize(IWTSPlugin* plugin,
        IWTSVirtualChannelManager* manager) {
    
    guac_rdp_usb_plugin* usb_plugin = (guac_rdp_usb_plugin*) plugin;
    guac_client* client = usb_plugin->client;
    
    if (usb_plugin->initialized) {
        guac_client_log(client, GUAC_LOG_WARNING, 
                "USB plugin already initialized");
        return CHANNEL_RC_OK;
    }
    
    guac_client_log(client, GUAC_LOG_DEBUG, "USB: Initializing plugin");
    
    /* Check if device manager exists and has devices */
    if (usb_plugin->device_manager) {
        int device_count = guac_rdp_usb_manager_get_device_count(
                usb_plugin->device_manager);
        if (device_count > 0) {
            guac_client_log(client, GUAC_LOG_INFO,
                    "USB: %d device(s) already in manager, waiting for channel",
                    device_count);
        }
    }
    
    /* Initialize channel state */
    usb_plugin->channel_status = INIT_CHANNEL_IN;
    
    /* Create listener callback */
    usb_plugin->listener_callback =
            guac_mem_zalloc(sizeof(guac_rdp_usb_listener_callback));
    usb_plugin->listener_callback->client = client;
    usb_plugin->listener_callback->plugin = usb_plugin;
    usb_plugin->listener_callback->parent.OnNewChannelConnection =
            guac_rdp_usb_on_new_channel_connection;
    
    /* Register URBDRC channel listener */
    UINT result = manager->CreateListener(manager, "URBDRC", 0,
            (IWTSListenerCallback*) usb_plugin->listener_callback, 
            &usb_plugin->listener);
    
    if (result != CHANNEL_RC_OK) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "USB: Failed to create channel listener: %u", result);
        guac_mem_free(usb_plugin->listener_callback);
        usb_plugin->listener_callback = NULL;
        return result;
    }
    
    usb_plugin->initialized = 1;
    guac_client_log(client, GUAC_LOG_INFO, 
            "USB: Plugin initialized, waiting for URBDRC channel connection");
    
    return CHANNEL_RC_OK;
}

/**
 * Plugin termination
 */
static UINT guac_rdp_usb_terminated(IWTSPlugin* plugin) {
    
    guac_rdp_usb_plugin* usb_plugin = (guac_rdp_usb_plugin*) plugin;
    guac_client* client = usb_plugin->client;
    
    guac_client_log(client, GUAC_LOG_DEBUG, "USB: Plugin terminating");
    
    /* Free listener callback */
    if (usb_plugin->listener_callback) {
        guac_mem_free(usb_plugin->listener_callback);
        usb_plugin->listener_callback = NULL;
    }
    
    guac_client_log(client, GUAC_LOG_DEBUG, "USB: Plugin terminated");
    return CHANNEL_RC_OK;
}

/**
 * Plugin entry point
 */
UINT DVCPluginEntry(IDRDYNVC_ENTRY_POINTS* pEntryPoints) {
    
#ifdef PLUGIN_DATA_CONST
    const ADDIN_ARGV* args = pEntryPoints->GetPluginData(pEntryPoints);
#else
    ADDIN_ARGV* args = pEntryPoints->GetPluginData(pEntryPoints);
#endif
    
    guac_client* client = (guac_client*) guac_rdp_string_to_ptr(args->argv[1]);
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    
    guac_client_log(client, GUAC_LOG_DEBUG, "USB: Plugin entry point called");
    
    /* Allocate new plugin */
    guac_rdp_usb_plugin* usb_plugin = guac_mem_zalloc(sizeof(guac_rdp_usb_plugin));
    usb_plugin->parent.Initialize = guac_rdp_usb_initialize;
    usb_plugin->parent.Terminated = guac_rdp_usb_terminated;
    usb_plugin->client = client;
    
    /* Reference the device manager from rdp_client */
    usb_plugin->device_manager = rdp_client->usb_manager;
    
    if (usb_plugin->device_manager)
        guac_client_log(client, GUAC_LOG_DEBUG, 
                "USB: Device manager available");

    else
        guac_client_log(client, GUAC_LOG_WARNING,
                "USB: No device manager available");

    rdp_client->usb_plugin = usb_plugin;
    
    /* Register the plugin */
    UINT result = pEntryPoints->RegisterPlugin(pEntryPoints, "guacusb",
            (IWTSPlugin*) usb_plugin);
    
    if (result != CHANNEL_RC_OK) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "USB: Failed to register plugin: %u", result);
        guac_mem_free(usb_plugin);
        rdp_client->usb_plugin = NULL;
        return result;
    }
    
    guac_client_log(client, GUAC_LOG_INFO, "USB: Plugin registered");
    return CHANNEL_RC_OK;
}