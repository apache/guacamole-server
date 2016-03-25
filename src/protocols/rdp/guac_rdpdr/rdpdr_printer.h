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


#ifndef __GUAC_RDPDR_PRINTER_H
#define __GUAC_RDPDR_PRINTER_H

#include "config.h"

#include "rdpdr_service.h"

#include <guacamole/stream.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <pthread.h>

/**
 * Data specific to an instance of the printer device.
 */
typedef struct guac_rdpdr_printer_data {

    /**
     * Stream for receiving printed files.
     */
    guac_stream* stream;

    /**
     * File descriptor that should be written to when sending documents to the
     * printer.
     */
    int printer_input;

    /**
     * File descriptor that should be read from when receiving output from the
     * printer.
     */
    int printer_output;

    /**
     * Thread which transfers data from the printer to the Guacamole client.
     */
    pthread_t printer_output_thread;

    /**
     * The number of bytes received in the current print job.
     */
    int bytes_received;

} guac_rdpdr_printer_data;

/**
 * Registers a new printer device within the RDPDR plugin. This must be done
 * before RDPDR connection finishes.
 */
void guac_rdpdr_register_printer(guac_rdpdrPlugin* rdpdr);

/**
 * The command to run when filtering postscript to produce PDF. This must be
 * a NULL-terminated array of arguments, where the first argument is the name
 * of the file to run.
 */
extern char* const guac_rdpdr_pdf_filter_command[];

#endif

