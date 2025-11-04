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

#include "channels/rdpecam/rdpecam_caps.h"
#include "rdp.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/rwlock.h>
#include <guacamole/user.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

size_t guac_rdp_rdpecam_sanitize_device_name(const char* name, char* sanitized, size_t len) {

    if (!name || !sanitized || len == 0)
        return 0;

    size_t pos = 0;
    const char* src = name;

    /* Windows invalid characters: / \ : * ? " < > | */
    while (*src && pos < len - 1) {
        char c = *src++;
        
        /* Replace invalid characters with underscore */
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|') {
            if (pos < len - 1)
                sanitized[pos++] = '_';
        }
        /* Skip control characters */
        else if ((unsigned char)c < 32) {
            continue;
        }
        else {
            sanitized[pos++] = c;
        }
    }

    sanitized[pos] = '\0';

    /* Trim to 255 characters (Windows device name limit) */
    if (pos > 255) {
        sanitized[255] = '\0';
        pos = 255;
    }

    return pos;
}

int guac_rdp_rdpecam_capabilities_callback(guac_user* user,
        const char* mimetype, const char* name, const char* value, void* data) {

    guac_client* client = user ? user->client : NULL;
    guac_rdp_client* rdp_client = client ? (guac_rdp_client*) client->data : NULL;

    if (!client || !rdp_client || !value)
        return 0;

    size_t len = strlen(value);
    char* copy = guac_mem_alloc(len + 1);
    if (!copy)
        return 0;
    memcpy(copy, value, len + 1);

    guac_rwlock_acquire_write_lock(&(rdp_client->lock));
    
    /* Free old device capabilities */
    for (unsigned int i = 0; i < rdp_client->rdpecam_device_caps_count; i++) {
        guac_rdp_rdpecam_device_caps* caps = &rdp_client->rdpecam_device_caps[i];
        if (caps->device_id)
            guac_mem_free(caps->device_id);
        if (caps->device_name)
            guac_mem_free(caps->device_name);
        caps->device_id = NULL;
        caps->device_name = NULL;
        caps->format_count = 0;
    }
    rdp_client->rdpecam_device_caps_count = 0;

    /* Parse multi-device capabilities format (required):
     * "DEVICE_ID:DEVICE_NAME|640x480@30/1,...;DEVICE_ID:DEVICE_NAME|320x240@30/1,..."
     * Format requires semicolon-separated device list, each entry must include device ID.
     */

    unsigned int device_count = 0;
    char* device_saveptr = NULL;
    char* device_entry = strtok_r(copy, ";", &device_saveptr);

    /* Require semicolon-separated format (multi-device) */
    if (!device_entry) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "RDPECAM received capabilities in invalid format (expected semicolon-separated device list)");
        guac_mem_free(copy);
        return 0;
    }

    while (device_entry && device_count < GUAC_RDP_RDPECAM_MAX_DEVICES) {
        guac_rdp_rdpecam_device_caps* caps = &rdp_client->rdpecam_device_caps[device_count];
        
        /* Find pipe separator (between device info and formats) */
        char* formats_str = device_entry;
        char* pipe_pos = strchr(device_entry, '|');
        char* device_info = NULL;
        
        if (!pipe_pos) {
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM skipping device entry without pipe separator: '%s'", device_entry);
            device_entry = strtok_r(NULL, ";", &device_saveptr);
            continue;
        }

        *pipe_pos = '\0';
        device_info = device_entry;
        formats_str = pipe_pos + 1;

        /* Require device ID in format "DEVICE_ID:DEVICE_NAME" */
        char* device_id_parsed = NULL;
        char* device_name_parsed = NULL;
        
        if (!device_info || !*device_info) {
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM skipping device entry without device info");
            device_entry = strtok_r(NULL, ";", &device_saveptr);
            continue;
        }

        char* colon_pos = strchr(device_info, ':');
        if (!colon_pos) {
            guac_client_log(client, GUAC_LOG_WARNING,
                    "RDPECAM skipping device entry without device ID (format: DEVICE_ID:DEVICE_NAME required): '%s'",
                    device_info);
            device_entry = strtok_r(NULL, ";", &device_saveptr);
            continue;
        }

        /* Parse device ID and name (format: "DEVICE_ID:DEVICE_NAME") */
        *colon_pos = '\0';
        device_id_parsed = device_info;
        device_name_parsed = colon_pos + 1;

        /* Require non-empty device ID */
        if (!device_id_parsed || !*device_id_parsed) {
            guac_client_log(client, GUAC_LOG_WARNING,
                    "RDPECAM skipping device entry with empty device ID");
            device_entry = strtok_r(NULL, ";", &device_saveptr);
            continue;
        }

        /* Store device ID (required) */
        size_t id_len = strlen(device_id_parsed);
        caps->device_id = guac_mem_alloc(id_len + 1);
        if (!caps->device_id) {
            guac_client_log(client, GUAC_LOG_ERROR,
                    "RDPECAM failed to allocate device ID string");
            device_entry = strtok_r(NULL, ";", &device_saveptr);
            continue;
        }
        memcpy(caps->device_id, device_id_parsed, id_len + 1);

        /* Sanitize and store device name */
        if (device_name_parsed && *device_name_parsed) {
            char sanitized[256];
            size_t sanitized_len = guac_rdp_rdpecam_sanitize_device_name(
                    device_name_parsed, sanitized, sizeof(sanitized));
            
            if (sanitized_len > 0) {
                caps->device_name = guac_mem_alloc(sanitized_len + 1);
                if (caps->device_name) {
                    memcpy(caps->device_name, sanitized, sanitized_len + 1);
                }
            }
        }

        /* Parse formats for this device */
        unsigned int format_count = 0;
        char* format_saveptr = NULL;
        char* format_token = strtok_r(formats_str, ",", &format_saveptr);

        while (format_token && format_count < GUAC_RDP_RDPECAM_MAX_FORMATS) {
            /* Trim whitespace */
            while (isspace((unsigned char) *format_token))
                format_token++;

            char* end = format_token + strlen(format_token);
            while (end > format_token && isspace((unsigned char) *(end - 1)))
                *(--end) = '\0';

            unsigned int width = 0;
            unsigned int height = 0;
            unsigned int fps_num = 0;
            unsigned int fps_den = 1;

            int parsed = sscanf(format_token, "%ux%u@%u/%u", &width, &height, &fps_num, &fps_den);
            if (parsed < 4) {
                fps_den = 1;
                parsed = sscanf(format_token, "%ux%u@%u", &width, &height, &fps_num);
            }

            if (parsed >= 3 && width && height && fps_num) {
                guac_rdp_rdpecam_format* fmt = &caps->formats[format_count++];
                fmt->width = width;
                fmt->height = height;
                fmt->fps_num = fps_num;
                fmt->fps_den = fps_den ? fps_den : 1;
            }
            else {
                guac_client_log(client, GUAC_LOG_DEBUG,
                        "RDPECAM ignored unparseable format entry: '%s'", format_token);
            }

            format_token = strtok_r(NULL, ",", &format_saveptr);
        }

        caps->format_count = format_count;

        /* Only add device if it has valid formats (device ID is already stored above) */
        if (format_count > 0) {
            device_count++;
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "RDPECAM device %u: id='%s', name='%s', formats=%u",
                    device_count - 1,
                    caps->device_id,
                    caps->device_name ? caps->device_name : "(none)",
                    format_count);
        } else {
            /* No formats for this device - free device ID and skip */
            guac_client_log(client, GUAC_LOG_WARNING,
                    "RDPECAM skipping device '%s' (id='%s') with no valid formats",
                    caps->device_name ? caps->device_name : "(unnamed)",
                    caps->device_id);
            if (caps->device_id) {
                guac_mem_free(caps->device_id);
                caps->device_id = NULL;
            }
            if (caps->device_name) {
                guac_mem_free(caps->device_name);
                caps->device_name = NULL;
            }
        }

        /* Get next device entry */
        device_entry = strtok_r(NULL, ";", &device_saveptr);
    }

    rdp_client->rdpecam_device_caps_count = device_count;
    
    /* Set flag to notify plugin that capabilities have been updated. */
    rdp_client->rdpecam_caps_updated = 1;

    /* If plugin registered a notification callback, invoke it now to allow
     * immediate processing (e.g., sending DeviceAddedNotification). */
    if (rdp_client->rdpecam_caps_notify)
        rdp_client->rdpecam_caps_notify(client);

    guac_rwlock_release_lock(&(rdp_client->lock));

    guac_client_log(client, GUAC_LOG_DEBUG,
            "RDPECAM capabilities updated (%u devices), notifying plugin", device_count);

    guac_mem_free(copy);
    return 0;
}

