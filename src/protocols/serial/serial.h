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

#ifndef GUAC_SERIAL_H
#define GUAC_SERIAL_H

#include "settings.h"
#include "stream.h"
#include "terminal/terminal.h"

#include <guacamole/recording.h>

#include <pthread.h>

/**
 * Serial-specific client data.
 */
typedef struct guac_serial_client {

    /**
     * Serial connection settings.
     */
    guac_serial_settings* settings;

    /**
     * The serial client thread.
     */
    pthread_t client_thread;

    /**
     * Whether client_thread has been successfully created and must therefore
     * be joined during cleanup.
     */
    int client_thread_valid;

    /**
     * The serial byte stream connected to the serial line, or NULL if no
     * connection has been established.
     */
    guac_serial_stream* stream;

    /**
     * The terminal which will render all output from the serial line.
     */
    guac_terminal* term;

    /**
     * The in-progress session recording, or NULL if no recording is in
     * progress.
     */
    guac_recording* recording;

} guac_serial_client;

/**
 * Main serial client thread, handling transfer of serial output to the
 * terminal and user input to the serial line.
 *
 * @param data
 *     The guac_client instance associated with the serial connection.
 *
 * @return
 *     Always NULL.
 */
void* guac_serial_client_thread(void* data);

#endif
