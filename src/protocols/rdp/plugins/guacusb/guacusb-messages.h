/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.
 */

#ifndef GUAC_RDP_PLUGINS_GUACUSB_MESSAGES_H
#define GUAC_RDP_PLUGINS_GUACUSB_MESSAGES_H

#include <winpr/stream.h>
#include <stdint.h>
#include "guacusb.h"

/* Protocol constants from MS-RDPEUSB */
#define RIM_CAPABILITY_VERSION_01       0x00000001

#define CAPABILITIES_NEGOTIATOR         0x00000000
#define CLIENT_DEVICE_SINK             0x00000001
#define SERVER_CHANNEL_NOTIFICATION    0x00000002
#define CLIENT_CHANNEL_NOTIFICATION    0x00000003

#define STREAM_ID_NONE   0x0UL
#define STREAM_ID_PROXY  0x1UL
#define STREAM_ID_STUB   0x2UL

#define RIMCALL_RELEASE                0x00000001
#define RIMCALL_QUERYINTERFACE         0x00000002
#define RIM_EXCHANGE_CAPABILITY_REQUEST 0x00000100
#define CHANNEL_CREATED                0x00000100
#define ADD_VIRTUAL_CHANNEL            0x00000100
#define ADD_DEVICE                     0x00000101
#define RETRACT_DEVICE                 0x00000107

#define UsbRetractReason_Removed        0x00000001

/* Message sending functions */
int guac_rdp_usb_send_capability_response(guac_rdp_usb_plugin* plugin, 
        uint32_t MessageId, uint32_t Version);

int guac_rdp_usb_send_channel_created(guac_rdp_usb_plugin* plugin,
        uint32_t MessageId, uint32_t MajorVersion, uint32_t MinorVersion, 
        uint32_t Capabilities);

int guac_rdp_usb_send_virtual_channel_add(guac_rdp_usb_plugin* plugin,
        uint32_t device_id);

int guac_rdp_usb_send_device_add(guac_rdp_usb_plugin* plugin,
        guac_rdp_usb_device* device);

int guac_rdp_usb_send_retract_device(guac_rdp_usb_plugin* plugin,
        uint32_t device_id, uint32_t reason);

int guac_rdp_usb_write_stream(guac_rdp_usb_plugin* plugin, wStream* s);

int guac_rdp_usb_send_test_message(guac_rdp_usb_plugin* plugin);

#endif /* GUAC_RDP_PLUGINS_GUACUSB_MESSAGES_H */