/*
 * Copyright (C) 2016 Glyptodon, Inc.
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

#ifndef GUACENC_DISPLAY_H
#define GUACENC_DISPLAY_H

#include "config.h"
#include "buffer.h"
#include "image-stream.h"
#include "layer.h"

#include <guacamole/timestamp.h>

/**
 * The maximum number of buffers that the Guacamole video encoder will handle
 * within a single Guacamole protocol dump.
 */
#define GUACENC_DISPLAY_MAX_BUFFERS 4096

/**
 * The maximum number of layers that the Guacamole video encoder will handle
 * within a single Guacamole protocol dump.
 */
#define GUACENC_DISPLAY_MAX_LAYERS 64

/**
 * The maximum number of streams that the Guacamole video encoder will handle
 * within a single Guacamole protocol dump.
 */
#define GUACENC_DISPLAY_MAX_STREAMS 64

/**
 * The current state of the Guacamole video encoder's internal display.
 */
typedef struct guacenc_display {

    /**
     * All currently-allocated buffers. The index of the buffer corresponds to
     * its position within this array, where -1 is the 0th entry. If a buffer
     * has not yet been allocated, or a buffer has been freed (due to a
     * "dispose" instruction), its entry here will be NULL.
     */
    guacenc_buffer* buffers[GUACENC_DISPLAY_MAX_BUFFERS];

    /**
     * All currently-allocated layers. The index of the layer corresponds to
     * its position within this array. If a layer has not yet been allocated,
     * or a layer has been freed (due to a "dispose" instruction), its entry
     * here will be NULL.
     */
    guacenc_layer* layers[GUACENC_DISPLAY_MAX_LAYERS];

    /**
     * All currently-allocated image streams. The index of the stream
     * corresponds to its position within this array. If a stream has not yet
     * been allocated, or a stream has been freed (due to an "end"
     * instruction), its entry here will be NULL.
     */
    guacenc_image_stream* image_streams[GUACENC_DISPLAY_MAX_STREAMS];

    /**
     * The timestamp of the last sync instruction handled, or 0 if no sync has
     * yet been read.
     */
    guac_timestamp last_sync;

} guacenc_display;

/**
 * Handles a received "sync" instruction having the given timestamp, flushing
 * the current display to the in-progress video encoding.
 *
 * @param display
 *     The display to flush to the video encoding as a new frame.
 *
 * @param timestamp
 *     The timestamp of the new frame, as dictated by the "sync" instruction
 *     sent at the end of the frame.
 *
 * @return
 *     Zero if the frame was successfully written, non-zero if an error occurs.
 */
int guacenc_display_sync(guacenc_display* display, guac_timestamp timestamp);

#endif

