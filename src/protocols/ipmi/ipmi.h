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

#ifndef GUAC_IPMI_H
#define GUAC_IPMI_H

#include "settings.h"
#include "terminal/terminal.h"

#include <guacamole/recording.h>
#include <ipmiconsole.h>

#include <pthread.h>
#include <stdbool.h>

/**
 * An out-of-band chassis operation that may be performed asynchronously by the
 * control menu's background worker thread.
 */
typedef enum guac_ipmi_chassis_op {

    /**
     * A chassis power action (using the accompanying power action value).
     */
    GUAC_IPMI_CHASSIS_OP_POWER,

    /**
     * A query of the current chassis power status.
     */
    GUAC_IPMI_CHASSIS_OP_STATUS,

    /**
     * Activation of the chassis identify LED.
     */
    GUAC_IPMI_CHASSIS_OP_IDENTIFY,

    /**
     * Retrieval of the most recent System Event Log entries.
     */
    GUAC_IPMI_CHASSIS_OP_SEL

} guac_ipmi_chassis_op;

/**
 * IPMI-specific client data.
 */
typedef struct guac_ipmi_client {

    /**
     * IPMI connection settings.
     */
    guac_ipmi_settings* settings;

    /**
     * The IPMI client thread.
     */
    pthread_t client_thread;

    /**
     * The libipmiconsole context managing the Serial-over-LAN session, or NULL
     * if no session has been established.
     */
    ipmiconsole_ctx_t console_ctx;

    /**
     * The file descriptor of the established SOL session, as returned by
     * ipmiconsole_ctx_fd(), or -1 if no session has been established. All
     * serial console input/output is read from and written to this descriptor.
     */
    int console_fd;

    /**
     * The terminal which will render all output from the SOL session.
     */
    guac_terminal* term;

    /**
     * Whether the in-terminal control (power management) menu is currently
     * open. While the menu is open, keystrokes are interpreted as menu
     * commands rather than forwarded to the serial console.
     */
    bool menu_open;

    /**
     * When the control menu is awaiting confirmation of a destructive power
     * action, the action pending confirmation. GUAC_IPMI_POWER_NONE when no
     * confirmation is in progress.
     */
    guac_ipmi_power_action menu_pending_action;

    /**
     * Whether the control menu is displaying the result of a completed action
     * and is waiting for any keypress to dismiss it and return to the serial
     * console.
     */
    bool menu_awaiting_dismiss;

    /**
     * Mutex guarding the control menu's asynchronous chassis state
     * (menu_awaiting_dismiss, chassis_busy). Held only briefly around state
     * transitions shared between the user input thread and the chassis worker.
     */
    pthread_mutex_t menu_lock;

    /**
     * Whether an asynchronous chassis operation is currently being performed by
     * the worker thread. While true, control menu keystrokes are ignored.
     */
    bool chassis_busy;

    /**
     * The background thread performing the current (or most recent) chassis
     * operation. Valid for joining only when chassis_thread_valid is true.
     */
    pthread_t chassis_thread;

    /**
     * Whether chassis_thread holds a started thread that has not yet been
     * joined.
     */
    bool chassis_thread_valid;

    /**
     * The chassis operation the worker thread should perform.
     */
    guac_ipmi_chassis_op chassis_op;

    /**
     * The power action to perform when chassis_op is GUAC_IPMI_CHASSIS_OP_POWER.
     */
    guac_ipmi_power_action chassis_action;

    /**
     * The in-progress session recording, or NULL if no recording is in
     * progress.
     */
    guac_recording* recording;

} guac_ipmi_client;

/**
 * Main IPMI client thread, establishing the Serial-over-LAN session and
 * transferring SOL output to the terminal.
 *
 * @param data
 *     The guac_client associated with the IPMI connection.
 *
 * @return
 *     Always NULL.
 */
void* guac_ipmi_client_thread(void* data);

/**
 * Acquires a reference to the process-global libipmiconsole engine,
 * initializing it if this is the first reference. Each successful call must be
 * balanced by a later call to guac_ipmi_engine_unref(). This is thread-safe.
 *
 * @return
 *     Zero on success, or negative if the engine could not be initialized (in
 *     which case no reference is held).
 */
int guac_ipmi_engine_ref();

/**
 * Releases a reference to the process-global libipmiconsole engine, tearing it
 * down once the final reference is released. Must only be called to balance a
 * prior successful guac_ipmi_engine_ref(). This is thread-safe.
 */
void guac_ipmi_engine_unref();

#endif
