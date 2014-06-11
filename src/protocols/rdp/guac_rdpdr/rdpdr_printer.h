/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

