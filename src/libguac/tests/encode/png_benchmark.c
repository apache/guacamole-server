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

#define _POSIX_C_SOURCE 200809L

#include "../../encode-png.h"

#include <guacamole/socket.h>
#include <guacamole/stream.h>

#include <cairo/cairo.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PNG_BENCHMARK_WIDTH 1280
#define PNG_BENCHMARK_HEIGHT 720
#define PNG_BENCHMARK_WARMUPS 2
#define PNG_BENCHMARK_SAMPLES 7

/**
 * A data structure to store the count of encoded protocol bytes.
 */
typedef struct png_benchmark_sink {
    size_t bytes;
} png_benchmark_sink;

/**
 * Counts bytes written to a benchmark socket and returns the number accepted.
 *
 * @param socket
 *     The socket containing a png_benchmark_sink in its data pointer.
 *
 * @param data
 *     Bytes being written.
 *
 * @param length
 *     Number of bytes being written.
 *
 * @return
 *     Number of bytes accepted.
 */
static ssize_t count_bytes(guac_socket* socket, const void* data,
        size_t length) {

    (void) data;
    ((png_benchmark_sink*) socket->data)->bytes += length;
    return (ssize_t) length;

}

/**
 * Returns the current monotonic clock value in nanoseconds.
 *
 * @return
 *     The current monotonic clock value in nanoseconds.
 */
static uint64_t get_time_ns(void) {

    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (uint64_t) time.tv_sec * 1000000000ULL + time.tv_nsec;

}

/**
 * Populates a deterministic RGB24 surface for the specified synthetic image.
 *
 * @param surface
 *     Surface to populate.
 *
 * @param pattern
 *     Zero for four-color tiles, one for a gradient/text pattern, or two for
 *     fixed-seed RGB noise.
 */
static void fill_surface(cairo_surface_t* surface, int pattern) {

    static const uint32_t colors[] = {
        0xFF203040, 0xFF708090, 0xFFD0B080, 0xFF304060
    };

    cairo_surface_flush(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    uint32_t random = 0x12345678;

    for (int y = 0; y < PNG_BENCHMARK_HEIGHT; y++) {
        uint32_t* row = (uint32_t*) (data + (size_t) y * stride);
        for (int x = 0; x < PNG_BENCHMARK_WIDTH; x++) {

            if (pattern == 0)
                row[x] = colors[((x / 32) + (y / 32)) % 4];

            else if (pattern == 1) {
                uint32_t red = (uint32_t) (x * 255 / PNG_BENCHMARK_WIDTH);
                uint32_t green = (uint32_t) (y * 255 / PNG_BENCHMARK_HEIGHT);
                uint32_t blue = (uint32_t) ((x / 64 + y / 32) * 17) & 255;
                row[x] = 0xFF000000 | (red << 16) | (green << 8) | blue;
                if (x > 100 && x < 1180 && y > 100 && y < 650
                        && ((x / 8 + y / 8) % 5 == 0))
                    row[x] ^= 0x00303030;
            }

            else {
                random ^= random << 13;
                random ^= random >> 17;
                random ^= random << 5;
                row[x] = 0xFF000000 | (random & 0x00FFFFFF);
            }

        }
    }

    cairo_surface_mark_dirty(surface);

}

/**
 * Encodes one sample, stores elapsed time and serialized protocol bytes, and
 * returns zero on success or non-zero if encoding or socket flushing fails.
 *
 * @param surface
 *     Surface to encode.
 *
 * @param fast
 *     Non-zero to select the fast encoder.
 *
 * @param socket
 *     Socket used to collect serialized byte counts.
 *
 * @param sink
 *     Byte counter associated with the socket.
 *
 * @param stream
 *     Stream to associate with emitted blob instructions.
 *
 * @param bytes
 *     Receives the number of bytes written, including Guacamole framing.
 *
 * @param elapsed
 *     Receives elapsed nanoseconds.
 *
 * @return
 *     Zero on success, non-zero if encoding or socket flushing fails.
 */
static int encode_sample(cairo_surface_t* surface, int fast,
        guac_socket* socket, png_benchmark_sink* sink, guac_stream* stream,
        size_t* bytes, uint64_t* elapsed) {

    sink->bytes = 0;
    uint64_t start = get_time_ns();
    int result = fast
        ? guac_png_write_with_options(socket, stream, surface, 1)
        : guac_png_write(socket, stream, surface);
    if (!result)
        result = guac_socket_flush(socket);

    *elapsed = get_time_ns() - start;
    *bytes = sink->bytes;
    return result;

}

/**
 * Compares two elapsed-time samples for qsort() and returns a negative value,
 * zero, or a positive value as the first sample is less than, equal to, or
 * greater than the second.
 *
 * @param a
 *     First elapsed-time sample.
 *
 * @param b
 *     Second elapsed-time sample.
 *
 * @return
 *     A negative value, zero, or a positive value according to the ordering.
 */
static int compare_times(const void* a, const void* b) {

    uint64_t left = *(const uint64_t*) a;
    uint64_t right = *(const uint64_t*) b;
    return (left > right) - (left < right);

}

/**
 * Benchmarks both encoders against one deterministic synthetic image and
 * returns zero on success or non-zero if setup or encoding fails.
 *
 * @param name
 *     Human-readable image pattern name.
 *
 * @param pattern
 *     Pattern identifier accepted by fill_surface().
 *
 * @return
 *     Zero on success, non-zero if setup or encoding fails.
 */
static int run_case(const char* name, int pattern) {

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
            PNG_BENCHMARK_WIDTH, PNG_BENCHMARK_HEIGHT);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return 1;
    }
    fill_surface(surface, pattern);

    png_benchmark_sink sinks[2] = {{0}, {0}};
    guac_socket* sockets[2] = { guac_socket_alloc(), guac_socket_alloc() };
    if (!sockets[0] || !sockets[1]) {
        if (sockets[0])
            guac_socket_free(sockets[0]);
        if (sockets[1])
            guac_socket_free(sockets[1]);
        cairo_surface_destroy(surface);
        return 1;
    }
    for (int mode = 0; mode < 2; mode++) {
        sockets[mode]->data = &sinks[mode];
        sockets[mode]->write_handler = count_bytes;
    }

    guac_stream stream = { .index = 1 };
    uint64_t elapsed[2][PNG_BENCHMARK_SAMPLES];
    size_t bytes[2] = {0, 0};

    /* Warm up both encoders before collecting timed samples. */
    for (int warmup = 0; warmup < PNG_BENCHMARK_WARMUPS; warmup++) {
        for (int mode = 0; mode < 2; mode++) {
            uint64_t ignored_time;
            size_t ignored_bytes;
            if (encode_sample(surface, mode, sockets[mode], &sinks[mode],
                    &stream, &ignored_bytes, &ignored_time))
                goto fail;
        }
    }

    /* Alternate which encoder runs first to reduce ordering bias. */
    for (int sample = 0; sample < PNG_BENCHMARK_SAMPLES; sample++) {
        int first = sample % 2;
        for (int order = 0; order < 2; order++) {
            int mode = first ^ order;
            if (encode_sample(surface, mode, sockets[mode], &sinks[mode],
                    &stream, &bytes[mode], &elapsed[mode][sample]))
                goto fail;
        }
    }

    qsort(elapsed[0], PNG_BENCHMARK_SAMPLES, sizeof(uint64_t), compare_times);
    qsort(elapsed[1], PNG_BENCHMARK_SAMPLES, sizeof(uint64_t), compare_times);

    uint64_t default_time = elapsed[0][PNG_BENCHMARK_SAMPLES / 2];
    uint64_t fast_time = elapsed[1][PNG_BENCHMARK_SAMPLES / 2];
    printf("%s: default %zu bytes/%llu us; fast %zu bytes/%llu us; "
            "speedup %.2fx\n",
            name, bytes[0], (unsigned long long) (default_time / 1000),
            bytes[1], (unsigned long long) (fast_time / 1000),
            (double) default_time / fast_time);

    guac_socket_free(sockets[0]);
    guac_socket_free(sockets[1]);
    cairo_surface_destroy(surface);
    return 0;

fail:
    fprintf(stderr, "PNG encoding failed during benchmark case: %s\n", name);
    guac_socket_free(sockets[0]);
    guac_socket_free(sockets[1]);
    cairo_surface_destroy(surface);
    return 1;

}

/**
 * Runs all PNG benchmark cases and returns zero if they all succeed, or
 * non-zero if any case fails.
 *
 * @return
 *     Zero if all cases succeed, non-zero otherwise.
 */
int main(void) {

    puts("1280x720 RGB24; two warmups and seven timed samples per encoder;");
    puts("reported byte counts include Guacamole blob framing, "
            "not network I/O.");

    if (run_case("four-color tiles", 0)
            || run_case("gradient/text pattern", 1)
            || run_case("fixed-seed RGB noise", 2))
        return 1;

    return 0;

}
