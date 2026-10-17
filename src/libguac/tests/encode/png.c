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

#include "../../encode-png.h"

#include <CUnit/CUnit.h>
#include <guacamole/parser.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>

#include <cairo/cairo.h>
#include <png.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** In-memory storage for synthetic PNG/protocol data only. */
typedef struct test_png_buffer {
    unsigned char* data;
    size_t length;
    size_t offset;
} test_png_buffer;

static int append_bytes(test_png_buffer* buffer, const void* data, size_t length) {
    if (!length)
        return 0;
    unsigned char* resized = realloc(buffer->data, buffer->length + length);
    if (!resized)
        return -1;
    buffer->data = resized;
    memcpy(buffer->data + buffer->length, data, length);
    buffer->length += length;
    return 0;
}

static ssize_t capture_bytes(guac_socket* socket, const void* data, size_t length) {
    return append_bytes(socket->data, data, length) ? -1 : (ssize_t) length;
}

static cairo_status_t read_png_bytes(void* closure, unsigned char* data,
        unsigned int length) {
    test_png_buffer* buffer = closure;
    if (length > buffer->length - buffer->offset)
        return CAIRO_STATUS_READ_ERROR;
    memcpy(data, buffer->data + buffer->offset, length);
    buffer->offset += length;
    return CAIRO_STATUS_SUCCESS;
}

/** Encode through the production writer, socket, blob protocol and parser. */
static void encode_surface(cairo_surface_t* surface, test_png_buffer* png) {
    test_png_buffer protocol = {0};
    guac_socket* socket = guac_socket_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(socket);
    socket->data = &protocol;
    socket->write_handler = capture_bytes;
    guac_stream stream = { .index = 7 };
    CU_ASSERT_EQUAL(guac_png_write(socket, &stream, surface), 0);
    CU_ASSERT_EQUAL(guac_socket_flush(socket), 0);
    guac_socket_free(socket);

    /* Reassemble the real Guacamole blob instructions into one PNG. */
    size_t offset = 0;
    while (offset < protocol.length) {
        guac_parser* parser = guac_parser_alloc();
        CU_ASSERT_PTR_NOT_NULL_FATAL(parser);
        while (offset < protocol.length && parser->state != GUAC_PARSE_COMPLETE
                && parser->state != GUAC_PARSE_ERROR) {
            int consumed = guac_parser_append(parser, protocol.data + offset,
                    protocol.length - offset);
            CU_ASSERT(consumed > 0);
            if (consumed <= 0)
                break;
            offset += consumed;
        }
        CU_ASSERT_EQUAL(parser->state, GUAC_PARSE_COMPLETE);
        CU_ASSERT_EQUAL(parser->argc, 2);
        if (parser->state != GUAC_PARSE_COMPLETE
                || parser->argc != 2) {
            guac_parser_free(parser);
            break;
        }
        CU_ASSERT_STRING_EQUAL(parser->opcode, "blob");
        CU_ASSERT_STRING_EQUAL(parser->argv[0], "7");
        int length = guac_protocol_decode_base64(parser->argv[1]);
        CU_ASSERT(length >= 0);
        if (length >= 0)
            CU_ASSERT_EQUAL(append_bytes(png, parser->argv[1], length), 0);
        guac_parser_free(parser);
    }
    CU_ASSERT_EQUAL(offset, protocol.length);
    free(protocol.data);
}

/**
 * Generate initialized Cairo pixels, encode all option values, decode the PNG
 * and compare every pixel. Extra row padding is initialized and checked too.
 */
static void roundtrip(cairo_format_t format, int width, int height, int padding) {
    const char* old = getenv("GUAC_PNG_FAST_ENCODING");
    char* saved = old ? strdup(old) : NULL;
    int stride = cairo_format_stride_for_width(format, width) + padding;
    size_t size = (size_t) stride * height;
    unsigned char* pixels = malloc(size);
    unsigned char* original = malloc(size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(pixels);
    CU_ASSERT_PTR_NOT_NULL_FATAL(original);
    memset(pixels, 0xCD, size);
    for (int y = 0; y < height; y++) {
        uint32_t* row = (uint32_t*) (pixels + (size_t) y * stride);
        for (int x = 0; x < width; x++) {
            unsigned int r = (x * 3 + y * 7) & 255;
            unsigned int g = (x * 13 + y * 11) & 255;
            unsigned int b = (x + y * 5) & 255;
            unsigned int a = (x * 5 + y * 3) & 255;
            if (format == CAIRO_FORMAT_ARGB32) {
                r = r * a / 255;
                g = g * a / 255;
                b = b * a / 255;
            }
            /* The RGB24 high byte is unused and deliberately varies. */
            row[x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    memcpy(original, pixels, size);
    cairo_surface_t* surface = cairo_image_surface_create_for_data(pixels,
            format, width, height, stride);
    CU_ASSERT_EQUAL_FATAL(cairo_surface_status(surface), CAIRO_STATUS_SUCCESS);
    cairo_surface_mark_dirty(surface);

    /* Only exactly "1" may activate the experimental writer. */
    const char* modes[] = { NULL, "0", "true", "01", " 1", "1" };
    test_png_buffer baseline = {0};
    for (unsigned int mode = 0; mode < sizeof(modes) / sizeof(modes[0]); mode++) {
        if (modes[mode]) {
            CU_ASSERT_EQUAL(setenv("GUAC_PNG_FAST_ENCODING", modes[mode], 1), 0);
        }
        else {
            CU_ASSERT_EQUAL(unsetenv("GUAC_PNG_FAST_ENCODING"), 0);
        }
        test_png_buffer png = {0};
        encode_surface(surface, &png);
        CU_ASSERT(png.length >= 33);
        if (png.length < 33) {
            free(png.data);
            continue;
        }
        CU_ASSERT_EQUAL(memcmp(png.data, "\x89PNG\r\n\x1a\n", 8), 0);
        CU_ASSERT_EQUAL(memcmp(png.data + 12, "IHDR", 4), 0);
        if (format == CAIRO_FORMAT_RGB24 && mode == 5) {
            CU_ASSERT_EQUAL(png.data[24], 8);
            CU_ASSERT_EQUAL(png.data[25], PNG_COLOR_TYPE_RGB);
        }
        else if (format == CAIRO_FORMAT_RGB24 && width * height <= 256)
            CU_ASSERT_EQUAL(png.data[25], PNG_COLOR_TYPE_PALETTE);

        /* An independent decoder checks stream validity and every pixel. */
        cairo_surface_t* decoded = cairo_image_surface_create_from_png_stream(
                read_png_bytes, &png);
        CU_ASSERT_EQUAL(cairo_surface_status(decoded), CAIRO_STATUS_SUCCESS);
        if (cairo_surface_status(decoded) == CAIRO_STATUS_SUCCESS) {
            CU_ASSERT_EQUAL(cairo_image_surface_get_width(decoded), width);
            CU_ASSERT_EQUAL(cairo_image_surface_get_height(decoded), height);
            cairo_surface_flush(decoded);
            const unsigned char* decoded_data = cairo_image_surface_get_data(decoded);
            int decoded_stride = cairo_image_surface_get_stride(decoded);
            for (int y = 0; y < height; y++) {
                const uint32_t* expected = (const uint32_t*)
                    (original + (size_t) y * stride);
                const uint32_t* actual = (const uint32_t*)
                    (decoded_data + (size_t) y * decoded_stride);
                for (int x = 0; x < width; x++) {
                    if (format == CAIRO_FORMAT_RGB24) {
                        CU_ASSERT_EQUAL(actual[x] & 0xFFFFFF, expected[x] & 0xFFFFFF);
                    }
                    else {
                        CU_ASSERT_EQUAL(actual[x], expected[x]);
                    }
                }
            }
        }
        cairo_surface_destroy(decoded);
        CU_ASSERT_EQUAL(memcmp(pixels, original, size), 0);

        if (mode == 0)
            baseline = png;
        else {
            /* Default flags and all ARGB32 writes must preserve exact output. */
            if (mode != 5 || format == CAIRO_FORMAT_ARGB32) {
                CU_ASSERT_EQUAL(png.length, baseline.length);
                if (png.length == baseline.length)
                    CU_ASSERT_EQUAL(memcmp(png.data, baseline.data, png.length), 0);
            }
            free(png.data);
        }
    }
    free(baseline.data);
    cairo_surface_destroy(surface);
    free(pixels);
    free(original);
    if (saved) {
        CU_ASSERT_EQUAL(setenv("GUAC_PNG_FAST_ENCODING", saved, 1), 0);
        free(saved);
    }
    else
        CU_ASSERT_EQUAL(unsetenv("GUAC_PNG_FAST_ENCODING"), 0);
}

void test_png__rgb_single_pixel(void) {
    roundtrip(CAIRO_FORMAT_RGB24, 1, 1, 0);
}

void test_png__rgb_odd_dimensions(void) {
    roundtrip(CAIRO_FORMAT_RGB24, 17, 13, 0);
}

void test_png__rgb_padded_stride(void) {
    roundtrip(CAIRO_FORMAT_RGB24, 512, 257, 12);
}

void test_png__rgb_large_odd_stride(void) {
    roundtrip(CAIRO_FORMAT_RGB24, 1366, 768, 20);
}

void test_png__argb_odd_fallback(void) {
    roundtrip(CAIRO_FORMAT_ARGB32, 17, 13, 8);
}

void test_png__argb_padded_fallback(void) {
    roundtrip(CAIRO_FORMAT_ARGB32, 512, 257, 16);
}
