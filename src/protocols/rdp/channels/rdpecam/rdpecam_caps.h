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

#ifndef GUAC_RDP_CHANNELS_RDPECAM_CAPS_H
#define GUAC_RDP_CHANNELS_RDPECAM_CAPS_H

#include <guacamole/user.h>
#include <stddef.h>

/**
 * The name of the guacamole protocol argument for camera capabilities.
 */
#define GUAC_RDPECAM_ARG_CAPABILITIES "rdpecam-capabilities"

/**
 * The name of the guacamole protocol argument for camera capability updates.
 * This is sent when the user enables/disables cameras during an active session.
 */
#define GUAC_RDPECAM_ARG_CAPABILITIES_UPDATE "rdpecam-capabilities-update"

/**
 * The name of the guacamole protocol argument for camera capability updates.
 * This is sent when the user enables/disables cameras during an active session.
 */

/**
 * Maximum number of RDPECAM formats remembered from the browser.
 */
#define GUAC_RDP_RDPECAM_MAX_FORMATS 16

/**
 * Maximum number of camera devices that can be redirected simultaneously.
 */
#define GUAC_RDP_RDPECAM_MAX_DEVICES 8

/**
 * Describes a single camera format (resolution + frame rate) reported by the
 * browser.
 */
typedef struct guac_rdp_rdpecam_format {
    /**
     * Width of the video format in pixels.
     */
    unsigned int width;

    /**
     * Height of the video format in pixels.
     */
    unsigned int height;

    /**
     * Frame rate numerator (frames per second).
     */
    unsigned int fps_num;

    /**
     * Frame rate denominator (for fractional frame rates).
     */
    unsigned int fps_den;
} guac_rdp_rdpecam_format;

/**
 * Per-device camera capabilities reported by the browser.
 */
typedef struct guac_rdp_rdpecam_device_caps {
    /**
     * Browser device ID (unique identifier from navigator.mediaDevices).
     * Used to map between browser devices and Windows channel names.
     */
    char* device_id;

    /**
     * Sanitized device name from track.label, suitable for Windows.
     * If NULL or empty, a default name will be used based on device index.
     */
    char* device_name;

    /**
     * Supported formats for this device.
     */
    guac_rdp_rdpecam_format formats[GUAC_RDP_RDPECAM_MAX_FORMATS];

    /**
     * Number of valid entries within formats array.
     */
    unsigned int format_count;
} guac_rdp_rdpecam_device_caps;

/**
 * Sanitizes a camera device name for Windows compatibility.
 * Removes or replaces characters that are invalid in Windows device names.
 *
 * @param name
 *     The device name to sanitize.
 *
 * @param sanitized
 *     Buffer to store the sanitized name (must be at least 256 bytes).
 *
 * @param len
 *     Size of the sanitized buffer.
 *
 * @return
 *     Number of characters written (excluding null terminator), or 0 on error.
 */
size_t guac_rdp_rdpecam_sanitize_device_name(const char* name, char* sanitized, size_t len);

/**
 * Callback invoked when camera capabilities are received from the browser.
 * This function parses the multi-device capability string and updates the
 * RDP client's device capability storage. An empty string clears all previously
 * advertised devices.
 *
 * @param user
 *     The user who sent the capabilities.
 *
 * @param mimetype
 *     The mimetype of the data (unused).
 *
 * @param name
 *     The name of the argument. Either "rdpecam-capabilities" or
 *     "rdpecam-capabilities-update".
 *
 * @param value
 *     The capability string in format:
 *     "DEVICE_ID:DEVICE_NAME|WIDTHxHEIGHT@FPS_NUM/FPS_DEN,...;..."
 *     or empty if all cameras are disabled.
 *
 * @param data
 *     User-defined data (unused).
 *
 * @return
 *     Always returns 0.
 */
int guac_rdp_rdpecam_capabilities_callback(guac_user* user,
        const char* mimetype, const char* name, const char* value, void* data);

#endif

