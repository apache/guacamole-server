/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.
 */

#ifndef GUAC_RDP_PLUGINS_GUACUSB_H
#define GUAC_RDP_PLUGINS_GUACUSB_H

#include <freerdp/constants.h>
#include <freerdp/dvc.h>
#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <stdint.h>

/* Channel states */
#define INIT_CHANNEL_IN     1
#define INIT_CHANNEL_OUT    0

/* Forward declarations */
typedef struct guac_rdp_usb_plugin guac_rdp_usb_plugin;
typedef struct guac_rdp_usb_manager guac_rdp_usb_manager;
typedef struct guac_rdp_usb_device guac_rdp_usb_device;

/**
 * Listener callback structure for URBDRC channel
 */
typedef struct guac_rdp_usb_listener_callback {
    IWTSListenerCallback parent;
    guac_client* client;
    guac_rdp_usb_plugin* plugin;
    IWTSVirtualChannelManager* channel_manager;
} guac_rdp_usb_listener_callback;

/**
 * Channel callback structure for URBDRC channel data
 */
typedef struct guac_rdp_usb_channel_callback {
    IWTSVirtualChannelCallback parent;
    IWTSVirtualChannel* channel;
    guac_client* client;
    guac_rdp_usb_plugin* plugin;
    IWTSVirtualChannelManager* channel_manager;
} guac_rdp_usb_channel_callback;

/**
 * Main USB plugin structure
 */
struct guac_rdp_usb_plugin {
    
    /* FreeRDP plugin base */
    IWTSPlugin parent;
    
    /* Listener callback for new connections */
    guac_rdp_usb_listener_callback* listener_callback;
    
    /* Guacamole client reference */
    guac_client* client;
    
    /* Channel management */
    IWTSVirtualChannel* channel;
    IWTSListener* listener;
    uint32_t channel_status;
    int initialized;
    
    /* Device manager - shared with rdp_client, not owned by plugin */
    guac_rdp_usb_manager* device_manager;
};

#endif /* GUAC_RDP_PLUGINS_GUACUSB_H */