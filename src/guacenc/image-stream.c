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

#include "config.h"
#include "image-stream.h"
#include "jpeg.h"
#include "log.h"
#include "png.h"

#ifdef ENABLE_WEBP
#include "webp.h"
#endif

#include <stdlib.h>
#include <string.h>

guacenc_decoder_mapping guacenc_decoder_map[] = {
    {"image/png",  &guacenc_png_decoder},
    {"image/jpeg", &guacenc_jpeg_decoder},
#ifdef ENABLE_WEBP
    {"image/webp", &guacenc_webp_decoder},
#endif
    {NULL,         NULL}
};

guacenc_decoder* guacenc_get_decoder(const char* mimetype) {

    /* Search through mapping for the decoder having given mimetype */
    guacenc_decoder_mapping* current = guacenc_decoder_map;
    while (current->mimetype != NULL) {

        /* Return decoder if mimetype matches */
        if (strcmp(current->mimetype, mimetype) == 0)
            return current->decoder;

        /* Next candidate decoder */
        current++;

    }

    /* No such decoder */
    guacenc_log(GUAC_LOG_WARNING, "Support for \"%s\" not present", mimetype);
    return NULL;

}

guacenc_image_stream* guacenc_image_stream_alloc(int mask, int index,
        const char* mimetype, int x, int y) {

    /* Allocate stream */
    guacenc_image_stream* stream = malloc(sizeof(guacenc_image_stream));
    if (stream == NULL)
        return NULL;

    /* Init properties */
    stream->index = index;
    stream->mask = mask;
    stream->x = x;
    stream->y = y;

    /* Associate with corresponding decoder */
    guacenc_decoder* decoder = stream->decoder = guacenc_get_decoder(mimetype);
    if (decoder != NULL)
        decoder->init_handler(stream);

    return stream;

}

int guacenc_image_stream_free(guacenc_image_stream* stream) {

    /* Ignore NULL streams */
    if (stream == NULL)
        return 0;

    /* Invoke free handler for decoder (if associated) */
    guacenc_decoder* decoder = stream->decoder;
    if (decoder != NULL)
        decoder->free_handler(stream);

    /* Free actual stream */
    free(stream);
    return 0;

}

