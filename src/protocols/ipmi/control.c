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

#include "chassis.h"
#include "control.h"
#include "ipmi.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <ipmiconsole.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * The interval, in seconds, the chassis identify LED is activated for.
 */
#define GUAC_IPMI_CONTROL_IDENTIFY_INTERVAL 15

/**
 * The maximum size, in bytes, of a single inbound control message (one line of
 * newline-delimited JSON).
 */
#define GUAC_IPMI_CONTROL_BUFFER_LENGTH 4096

/**
 * The maximum size, in bytes, of the buffer used to render the System Event
 * Log for a control-channel response.
 */
#define GUAC_IPMI_CONTROL_SEL_LENGTH 8192

/**
 * Per-inbound-stream reassembly state for the control pipe.
 */
typedef struct guac_ipmi_control_stream {

    /**
     * Buffer accumulating the current (possibly partial) inbound JSON line.
     */
    char buffer[GUAC_IPMI_CONTROL_BUFFER_LENGTH];

    /**
     * The number of bytes currently held in the buffer.
     */
    int length;

} guac_ipmi_control_stream;

/**
 * Escapes the given string into the body of a JSON string (without the
 * surrounding quotes), writing at most dstlen-1 bytes plus a NUL terminator.
 */
void guac_ipmi_control_json_escape(char* dst, int dstlen,
        const char* src) {

    int j = 0;
    for (int i = 0; src[i] != '\0' && j < dstlen - 2; i++) {
        char c = src[i];
        switch (c) {
            case '"':  case '\\': dst[j++] = '\\'; dst[j++] = c;   break;
            case '\n': dst[j++] = '\\'; dst[j++] = 'n';            break;
            case '\r': dst[j++] = '\\'; dst[j++] = 'r';            break;
            case '\t': dst[j++] = '\\'; dst[j++] = 't';            break;
            default:
                /* Drop other control characters; pass everything else */
                if ((unsigned char) c >= 0x20)
                    dst[j++] = c;
        }
    }
    dst[j] = '\0';

}

/**
 * Extracts the string value of the given key from a flat JSON object into out.
 * Returns non-zero if the key was found.
 */
int guac_ipmi_control_json_get(const char* json, const char* key,
        char* out, int outlen) {

    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    int needle_len = strlen(needle);

    /* Search each occurrence of the quoted token, accepting it only when it is
     * used as an object key (immediately followed, ignoring whitespace, by a
     * colon). This avoids matching an identical string that appears as a value
     * (e.g. the "command" in "type":"command"). */
    const char* p = json;
    while ((p = strstr(p, needle)) != NULL) {

        const char* q = p + needle_len;
        while (*q == ' ' || *q == '\t')
            q++;

        if (*q == ':') {
            q++;
            while (*q == ' ' || *q == '\t')
                q++;
            if (*q != '"')
                return 0;
            q++;

            int j = 0;
            while (*q != '\0' && *q != '"' && j < outlen - 1) {
                if (*q == '\\' && q[1] != '\0')
                    q++;
                out[j++] = *q++;
            }
            out[j] = '\0';
            return 1;
        }

        /* This occurrence was a value, not a key; keep searching */
        p += needle_len;
    }

    return 0;

}

/**
 * Sends one newline-delimited JSON message to the given user on a fresh
 * outbound "ipmi-control" pipe stream.
 */
static void guac_ipmi_control_send(guac_user* user, const char* json) {

    guac_stream* stream = guac_user_alloc_stream(user);
    guac_protocol_send_pipe(user->socket, stream, "application/json",
            GUAC_IPMI_CONTROL_PIPE_NAME);
    guac_protocol_send_blob(user->socket, stream, json, strlen(json));
    guac_protocol_send_end(user->socket, stream);
    guac_socket_flush(user->socket);
    guac_user_free_stream(user, stream);

}

/**
 * Pushes a state message to the given user. If query is true, the current
 * power state is actively read from the BMC (which may take several seconds)
 * and included in the message; otherwise only the SOL health is sent, leaving
 * the client's last-known power value untouched. This lets health-only updates
 * (initial state, and the broadcast when SOL connects/disconnects) refresh the
 * connection badge without regressing the displayed power state to "unknown".
 */
static void guac_ipmi_control_send_state(guac_user* user, guac_client* client,
        bool query) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    pthread_mutex_lock(&ipmi_client->state_lock);
    bool sol_connected = ipmi_client->sol_connected;
    pthread_mutex_unlock(&ipmi_client->state_lock);

    const char* health = sol_connected ? "sol-connected" : "sol-disconnected";

    char json[256];

    if (query) {

        const char* power = "unknown";
        char status[128];
        if (guac_ipmi_chassis_status(client, status, sizeof(status)) == 0) {
            if (strstr(status, "OFF") != NULL)
                power = "off";
            else if (strstr(status, "ON") != NULL)
                power = "on";
        }

        snprintf(json, sizeof(json),
                "{\"type\":\"state\",\"power\":\"%s\",\"health\":\"%s\"}",
                power, health);

    }
    else
        snprintf(json, sizeof(json),
                "{\"type\":\"state\",\"health\":\"%s\"}", health);

    guac_ipmi_control_send(user, json);

}

static void* guac_ipmi_control_broadcast_state_callback(guac_user* user,
        void* data) {

    guac_ipmi_control_send_state(user, (guac_client*) data, false);
    return NULL;

}

void guac_ipmi_control_broadcast_state(guac_client* client) {

    guac_client_foreach_user(client,
            guac_ipmi_control_broadcast_state_callback, client);

}

/**
 * Pushes a command result message to the given user.
 */
static void guac_ipmi_control_send_result(guac_user* user, const char* id,
        bool ok, const char* message) {

    char id_esc[128], msg_esc[256], json[512];
    guac_ipmi_control_json_escape(id_esc, sizeof(id_esc), id);
    guac_ipmi_control_json_escape(msg_esc, sizeof(msg_esc), message);

    snprintf(json, sizeof(json),
            "{\"id\":\"%s\",\"type\":\"result\",\"ok\":%s,\"message\":\"%s\"}",
            id_esc, ok ? "true" : "false", msg_esc);
    guac_ipmi_control_send(user, json);

}

/**
 * Reads the System Event Log and pushes it to the given user. The entries are
 * currently delivered as a preformatted text field; structured per-entry
 * delivery is a planned refinement.
 */
static void guac_ipmi_control_send_sel(guac_user* user, guac_client* client) {

    char* sel = guac_mem_alloc(GUAC_IPMI_CONTROL_SEL_LENGTH);
    char* json = guac_mem_alloc(GUAC_IPMI_CONTROL_SEL_LENGTH * 2);

    if (guac_ipmi_chassis_sel(client, sel, GUAC_IPMI_CONTROL_SEL_LENGTH) == 0) {
        char* esc = guac_mem_alloc(GUAC_IPMI_CONTROL_SEL_LENGTH * 2);
        guac_ipmi_control_json_escape(esc, GUAC_IPMI_CONTROL_SEL_LENGTH * 2, sel);
        snprintf(json, GUAC_IPMI_CONTROL_SEL_LENGTH * 2,
                "{\"type\":\"sel\",\"text\":\"%s\"}", esc);
        guac_mem_free(esc);
    }
    else
        snprintf(json, GUAC_IPMI_CONTROL_SEL_LENGTH * 2,
                "{\"type\":\"sel\",\"error\":\"Unable to read System Event Log.\"}");

    guac_ipmi_control_send(user, json);
    guac_mem_free(sel);
    guac_mem_free(json);

}

/**
 * Maps a command string to a chassis power action, or GUAC_IPMI_POWER_NONE if
 * the command is not a power action.
 */
static guac_ipmi_power_action guac_ipmi_control_power_action(const char* cmd) {
    if (strcmp(cmd, "power-on") == 0)             return GUAC_IPMI_POWER_ON;
    if (strcmp(cmd, "power-off") == 0)            return GUAC_IPMI_POWER_OFF;
    if (strcmp(cmd, "power-cycle") == 0)          return GUAC_IPMI_POWER_CYCLE;
    if (strcmp(cmd, "hard-reset") == 0)           return GUAC_IPMI_POWER_RESET;
    if (strcmp(cmd, "soft-shutdown") == 0)        return GUAC_IPMI_POWER_SOFT_SHUTDOWN;
    if (strcmp(cmd, "diagnostic-interrupt") == 0) return GUAC_IPMI_POWER_DIAGNOSTIC_INTERRUPT;
    return GUAC_IPMI_POWER_NONE;
}

/**
 * Attempts to acquire the shared chassis operation lock, so control-channel
 * operations and the in-terminal menu never run overlapping BMC sessions.
 * Returns non-zero if the lock was acquired.
 */
static int guac_ipmi_control_try_acquire(guac_ipmi_client* ipmi_client) {
    int acquired = 0;
    pthread_mutex_lock(&ipmi_client->menu_lock);
    if (!ipmi_client->chassis_busy) {
        ipmi_client->chassis_busy = true;
        acquired = 1;
    }
    pthread_mutex_unlock(&ipmi_client->menu_lock);
    return acquired;
}

static void guac_ipmi_control_release(guac_ipmi_client* ipmi_client) {
    pthread_mutex_lock(&ipmi_client->menu_lock);
    ipmi_client->chassis_busy = false;
    pthread_mutex_unlock(&ipmi_client->menu_lock);
}

/**
 * Parses and executes a single inbound control command line.
 */
static void guac_ipmi_control_dispatch(guac_user* user, const char* line) {

    guac_client* client = user->client;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    char type[32] = "", command[48] = "", id[128] = "";
    guac_ipmi_control_json_get(line, "type", type, sizeof(type));
    guac_ipmi_control_json_get(line, "command", command, sizeof(command));
    guac_ipmi_control_json_get(line, "id", id, sizeof(id));

    if (strcmp(type, "command") != 0)
        return;

    /* Non-mutating reads need no chassis lock */
    if (strcmp(command, "refresh-status") == 0 || strcmp(command, "status") == 0) {
        guac_ipmi_control_send_state(user, client, true);
        return;
    }

    if (strcmp(command, "read-sel") == 0) {
        guac_ipmi_control_send_sel(user, client);
        return;
    }

    /* Break operates on the existing SOL session, not a chassis session. Hold
     * state_lock and test sol_connected so the context cannot be torn down
     * between the liveness check and its use. */
    if (strcmp(command, "send-break") == 0) {
        pthread_mutex_lock(&ipmi_client->state_lock);
        int ok = ipmi_client->sol_connected
                && ipmiconsole_ctx_generate_break(ipmi_client->console_ctx) == 0;
        pthread_mutex_unlock(&ipmi_client->state_lock);
        guac_ipmi_control_send_result(user, id, ok,
                ok ? "Break sent." : "Unable to send break.");
        return;
    }

    /* Remaining commands open a chassis session; serialize with the menu */
    if (!guac_ipmi_control_try_acquire(ipmi_client)) {
        guac_ipmi_control_send_result(user, id, false,
                "Another operation is already in progress.");
        return;
    }

    if (strcmp(command, "identify") == 0) {
        int ok = guac_ipmi_chassis_identify(client,
                GUAC_IPMI_CONTROL_IDENTIFY_INTERVAL, false) == 0;
        guac_ipmi_control_send_result(user, id, ok,
                ok ? "Identify LED activated." : "Unable to activate identify LED.");
    }
    else {
        guac_ipmi_power_action action = guac_ipmi_control_power_action(command);
        if (action == GUAC_IPMI_POWER_NONE)
            guac_ipmi_control_send_result(user, id, false, "Unknown command.");
        else {
            int ok = guac_ipmi_chassis_power(client, action) == 0;
            guac_ipmi_control_send_result(user, id, ok,
                    ok ? "Command sent successfully." : "Command failed.");
        }
    }

    guac_ipmi_control_release(ipmi_client);

}

/**
 * Blob handler for the inbound control pipe: reassembles newline-delimited JSON
 * and dispatches each complete line.
 */
static int guac_ipmi_control_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_ipmi_control_stream* cs = (guac_ipmi_control_stream*) stream->data;
    const char* in = (const char*) data;

    for (int i = 0; i < length; i++) {
        char c = in[i];
        if (c == '\n') {
            cs->buffer[cs->length] = '\0';
            if (cs->length > 0)
                guac_ipmi_control_dispatch(user, cs->buffer);
            cs->length = 0;
        }
        else if (cs->length < GUAC_IPMI_CONTROL_BUFFER_LENGTH - 1)
            cs->buffer[cs->length++] = c;
        else
            /* Oversized line; drop it to resynchronize on the next newline */
            cs->length = 0;
    }

    guac_protocol_send_ack(user->socket, stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

static int guac_ipmi_control_end_handler(guac_user* user, guac_stream* stream) {
    guac_mem_free(stream->data);
    stream->data = NULL;
    return 0;
}

void guac_ipmi_control_open(guac_user* user, guac_stream* stream) {

    guac_ipmi_control_stream* cs =
        guac_mem_zalloc(sizeof(guac_ipmi_control_stream));

    stream->data = cs;
    stream->blob_handler = guac_ipmi_control_blob_handler;
    stream->end_handler = guac_ipmi_control_end_handler;

    guac_protocol_send_ack(user->socket, stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);

    /* Send an immediate, non-blocking state so the client has something to
     * render; the client may request an authoritative value with
     * "refresh-status". */
    guac_ipmi_control_send_state(user, user->client, false);

}
