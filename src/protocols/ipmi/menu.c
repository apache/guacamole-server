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
#include "ipmi.h"
#include "menu.h"
#include "settings.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>

#include <ipmiconsole.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * The number of seconds the chassis identify LED is activated for when
 * selected from the control menu.
 */
#define GUAC_IPMI_MENU_IDENTIFY_INTERVAL 15

/**
 * The maximum size, in bytes, of the buffer used to hold multi-line command
 * output (such as the System Event Log) rendered by the menu.
 */
#define GUAC_IPMI_MENU_OUTPUT_LENGTH 8192

/**
 * Terminal escape sequence switching to the alternate screen buffer (and
 * clearing it), used to display the control menu without disturbing — or being
 * recorded into — the serial console's primary screen and scrollback.
 */
#define GUAC_IPMI_MENU_ALT_ENTER "\x1B[?1049h\x1B[H\x1B[2J"

/**
 * Terminal escape sequence restoring the primary screen buffer, returning the
 * display to exactly the serial console output present before the menu opened.
 */
#define GUAC_IPMI_MENU_ALT_EXIT "\x1B[?1049l"

/**
 * Displays a "press any key to return" prompt and marks the current control
 * menu operation complete. Forward-declared for use by the chassis worker.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 */
static void guac_ipmi_menu_await_dismiss(guac_client* client);

/**
 * Writes the given null-terminated string to the client's terminal as console
 * output.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 *
 * @param str
 *     The null-terminated string to write.
 */
static void guac_ipmi_menu_print(guac_client* client, const char* str) {
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_terminal_write(ipmi_client->term, str, strlen(str));
}

/**
 * Returns a human-readable name for the given power action.
 *
 * @param action
 *     The power action to describe.
 *
 * @return
 *     A static, human-readable description of the action.
 */
static const char* guac_ipmi_menu_action_name(guac_ipmi_power_action action) {
    switch (action) {
        case GUAC_IPMI_POWER_ON:                   return "Power On";
        case GUAC_IPMI_POWER_OFF:                  return "Power Off";
        case GUAC_IPMI_POWER_CYCLE:                return "Power Cycle";
        case GUAC_IPMI_POWER_RESET:                return "Hard Reset";
        case GUAC_IPMI_POWER_SOFT_SHUTDOWN:        return "Soft Shutdown";
        case GUAC_IPMI_POWER_DIAGNOSTIC_INTERRUPT: return "Diagnostic Interrupt";
        default:                                   return "None";
    }
}

/**
 * Renders the control menu into the terminal.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 */
static void guac_ipmi_menu_render(guac_client* client) {
    guac_ipmi_menu_print(client,
        "\x1B[1m=== IPMI Control Menu ===\x1B[0m\r\n"
        " [1] Power On          [2] Power Off\r\n"
        " [3] Power Cycle       [4] Hard Reset\r\n"
        " [5] Soft Shutdown     [6] Diagnostic Interrupt (NMI)\r\n"
        " [s] Power Status      [i] Identify (15s)\r\n"
        " [e] View Event Log    [b] Send Break\r\n"
        " [q] Close menu\r\n"
        "Select: ");
}

/**
 * Background worker thread which performs the pending chassis operation (as
 * described by the client's chassis_op / chassis_action fields) without
 * blocking the user input thread, reports the result to the terminal, and then
 * marks the menu ready for dismissal.
 *
 * @param data
 *     The guac_client associated with the IPMI connection.
 *
 * @return
 *     Always NULL.
 */
static void* guac_ipmi_menu_chassis_worker(void* data) {

    guac_client* client = (guac_client*) data;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    switch (ipmi_client->chassis_op) {

        case GUAC_IPMI_CHASSIS_OP_POWER:
            if (guac_ipmi_chassis_power(client, ipmi_client->chassis_action) == 0)
                guac_ipmi_menu_print(client, "Command sent successfully.\r\n");
            else
                guac_ipmi_menu_print(client,
                        "Command FAILED (see server log).\r\n");
            break;

        case GUAC_IPMI_CHASSIS_OP_STATUS: {
            char status[128];
            if (guac_ipmi_chassis_status(client, status, sizeof(status)) == 0) {
                guac_ipmi_menu_print(client, status);
                guac_ipmi_menu_print(client, "\r\n");
            }
            else
                guac_ipmi_menu_print(client,
                        "Unable to query status (see server log).\r\n");
            break;
        }

        case GUAC_IPMI_CHASSIS_OP_IDENTIFY:
            if (guac_ipmi_chassis_identify(client,
                        GUAC_IPMI_MENU_IDENTIFY_INTERVAL, false) == 0)
                guac_ipmi_menu_print(client, "Identify LED activated.\r\n");
            else
                guac_ipmi_menu_print(client,
                        "Unable to activate identify LED (see server log).\r\n");
            break;

        case GUAC_IPMI_CHASSIS_OP_SEL: {
            char output[GUAC_IPMI_MENU_OUTPUT_LENGTH];
            if (guac_ipmi_chassis_sel(client, output, sizeof(output)) == 0)
                guac_ipmi_menu_print(client, output);
            else
                guac_ipmi_menu_print(client,
                        "Unable to read event log (see server log).\r\n");
            break;
        }

    }

    guac_ipmi_menu_await_dismiss(client);
    return NULL;

}

/**
 * Starts the given chassis operation on the background worker thread, printing
 * the given "working" message to the terminal immediately. If an operation is
 * already in progress, the request is silently ignored.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 *
 * @param op
 *     The chassis operation to perform.
 *
 * @param action
 *     The power action to perform, if op is GUAC_IPMI_CHASSIS_OP_POWER;
 *     otherwise ignored.
 *
 * @param working_message
 *     A null-terminated status message printed to the terminal before the
 *     operation begins.
 */
static void guac_ipmi_menu_start_op(guac_client* client,
        guac_ipmi_chassis_op op, guac_ipmi_power_action action,
        const char* working_message) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    /* Refuse to start a second operation while one is in progress */
    pthread_mutex_lock(&ipmi_client->menu_lock);
    if (ipmi_client->chassis_busy) {
        pthread_mutex_unlock(&ipmi_client->menu_lock);
        return;
    }
    ipmi_client->chassis_busy = true;
    pthread_mutex_unlock(&ipmi_client->menu_lock);

    /* Reap the previous worker, if any, before reusing the thread handle */
    if (ipmi_client->chassis_thread_valid) {
        pthread_join(ipmi_client->chassis_thread, NULL);
        ipmi_client->chassis_thread_valid = false;
    }

    ipmi_client->chassis_op = op;
    ipmi_client->chassis_action = action;

    guac_ipmi_menu_print(client, working_message);

    if (pthread_create(&ipmi_client->chassis_thread, NULL,
                guac_ipmi_menu_chassis_worker, client) == 0)
        ipmi_client->chassis_thread_valid = true;
    else {
        /* The worker could not be started; report and allow dismissal */
        guac_ipmi_menu_print(client, "Unable to start operation.\r\n");
        guac_ipmi_menu_await_dismiss(client);
    }

}

/**
 * Closes the control menu, clearing all menu state.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 */
static void guac_ipmi_menu_close(guac_client* client) {
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    ipmi_client->menu_open = false;
    ipmi_client->menu_pending_action = GUAC_IPMI_POWER_NONE;
    ipmi_client->menu_awaiting_dismiss = false;

    /* Restore the primary screen buffer, returning the display to the serial
     * console exactly as it was before the menu opened. */
    guac_ipmi_menu_print(client, GUAC_IPMI_MENU_ALT_EXIT);
}

/**
 * Marks the menu as displaying command output, prompting the user to press any
 * key to return to the serial console. This keeps the output visible on the
 * alternate screen until the user dismisses it.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 */
static void guac_ipmi_menu_await_dismiss(guac_client* client) {
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    guac_ipmi_menu_print(client,
            "\r\n\x1B[2m[ Press any key to return ]\x1B[0m");

    /* Publish the completed state: the operation (if any) is finished and the
     * next keypress should return to the console. */
    pthread_mutex_lock(&ipmi_client->menu_lock);
    ipmi_client->menu_awaiting_dismiss = true;
    ipmi_client->chassis_busy = false;
    pthread_mutex_unlock(&ipmi_client->menu_lock);
}

/**
 * Marks the given destructive power action as awaiting confirmation, prompting
 * the user.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 *
 * @param action
 *     The action awaiting confirmation.
 */
static void guac_ipmi_menu_confirm(guac_client* client,
        guac_ipmi_power_action action) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    ipmi_client->menu_pending_action = action;

    char message[128];
    snprintf(message, sizeof(message), "\r\nConfirm %s? (y/N): ",
            guac_ipmi_menu_action_name(action));
    guac_ipmi_menu_print(client, message);

}

void guac_ipmi_menu_open(guac_client* client) {
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    ipmi_client->menu_open = true;
    ipmi_client->menu_pending_action = GUAC_IPMI_POWER_NONE;
    ipmi_client->menu_awaiting_dismiss = false;

    /* Render the menu on the alternate screen so it neither pollutes the serial
     * console's scrollback and session recording nor corrupts any full-screen
     * application currently drawn on the primary screen. */
    guac_ipmi_menu_print(client, GUAC_IPMI_MENU_ALT_ENTER);
    guac_ipmi_menu_render(client);
}

void guac_ipmi_menu_handle_key(guac_client* client, int keysym) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    /* Ignore all keystrokes while an asynchronous chassis operation is running,
     * so it cannot be interrupted or run twice concurrently. */
    pthread_mutex_lock(&ipmi_client->menu_lock);
    int busy = ipmi_client->chassis_busy;
    pthread_mutex_unlock(&ipmi_client->menu_lock);
    if (busy)
        return;

    /* While displaying the output of a completed action, any key dismisses it
     * and returns to the serial console. */
    if (ipmi_client->menu_awaiting_dismiss) {
        guac_ipmi_menu_close(client);
        return;
    }

    /* If a destructive action is awaiting confirmation, interpret this key as
     * the confirmation response. */
    if (ipmi_client->menu_pending_action != GUAC_IPMI_POWER_NONE) {

        guac_ipmi_power_action action = ipmi_client->menu_pending_action;
        ipmi_client->menu_pending_action = GUAC_IPMI_POWER_NONE;

        if (keysym == 'y' || keysym == 'Y') {
            char message[128];
            snprintf(message, sizeof(message), "\r\n%s: working...\r\n",
                    guac_ipmi_menu_action_name(action));
            guac_ipmi_menu_start_op(client, GUAC_IPMI_CHASSIS_OP_POWER, action,
                    message);
        }
        else {
            guac_ipmi_menu_print(client, "\r\nCancelled.\r\n");
            guac_ipmi_menu_await_dismiss(client);
        }

        return;
    }

    switch (keysym) {

        /* Power On is non-destructive and executed directly */
        case '1': {
            char message[128];
            snprintf(message, sizeof(message), "\r\n%s: working...\r\n",
                    guac_ipmi_menu_action_name(GUAC_IPMI_POWER_ON));
            guac_ipmi_menu_start_op(client, GUAC_IPMI_CHASSIS_OP_POWER,
                    GUAC_IPMI_POWER_ON, message);
            break;
        }

        /* Destructive actions require confirmation */
        case '2':
            guac_ipmi_menu_confirm(client, GUAC_IPMI_POWER_OFF);
            break;
        case '3':
            guac_ipmi_menu_confirm(client, GUAC_IPMI_POWER_CYCLE);
            break;
        case '4':
            guac_ipmi_menu_confirm(client, GUAC_IPMI_POWER_RESET);
            break;
        case '5':
            guac_ipmi_menu_confirm(client, GUAC_IPMI_POWER_SOFT_SHUTDOWN);
            break;
        case '6':
            guac_ipmi_menu_confirm(client, GUAC_IPMI_POWER_DIAGNOSTIC_INTERRUPT);
            break;

        /* Power status */
        case 's':
        case 'S':
            guac_ipmi_menu_start_op(client, GUAC_IPMI_CHASSIS_OP_STATUS,
                    GUAC_IPMI_POWER_NONE, "\r\nQuerying power status...\r\n");
            break;

        /* System Event Log viewer */
        case 'e':
        case 'E':
            guac_ipmi_menu_start_op(client, GUAC_IPMI_CHASSIS_OP_SEL,
                    GUAC_IPMI_POWER_NONE, "\r\nReading System Event Log...\r\n");
            break;

        /* Chassis identify LED */
        case 'i':
        case 'I':
            guac_ipmi_menu_start_op(client, GUAC_IPMI_CHASSIS_OP_IDENTIFY,
                    GUAC_IPMI_POWER_NONE, "\r\nActivating identify LED...\r\n");
            break;

        /* Send a serial break over the active SOL session */
        case 'b':
        case 'B': {
            pthread_mutex_lock(&ipmi_client->state_lock);
            int broke = ipmi_client->sol_connected
                    && ipmiconsole_ctx_generate_break(
                        ipmi_client->console_ctx) == 0;
            pthread_mutex_unlock(&ipmi_client->state_lock);
            if (broke)
                guac_ipmi_menu_print(client, "\r\nBreak sent.\r\n");
            else
                guac_ipmi_menu_print(client, "\r\nUnable to send break.\r\n");
            guac_ipmi_menu_await_dismiss(client);
            break;
        }

        /* Close the menu without action */
        case 'q':
        case 'Q':
        case 0xFF1B: /* Escape */
            guac_ipmi_menu_close(client);
            break;

        /* Ignore any other key, leaving the menu open */
        default:
            break;

    }

}
