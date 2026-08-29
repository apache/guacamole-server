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
#include "interpret.h"
#include "log.h"
#include "state.h"

#include <CUnit/CUnit.h>

#include <dirent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * A temporary workspace within which a single test recording is written and
 * interpreted. All members are absolute paths within a freshly-created
 * temporary directory.
 */
typedef struct workspace {

    /** The root temporary directory created by ws_init(). */
    char root[512];

    /** The path of the recording file to be interpreted. */
    char rec[768];

    /** The output directory into which guacclip should extract items. */
    char out[768];

} workspace;

/**
 * Encodes the given bytes as a null-terminated standard base64 string. The
 * returned buffer is heap-allocated and must be freed by the caller.
 */
static char* b64_encode(const void* data, size_t len) {

    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const uint8_t* in = (const uint8_t*) data;
    size_t out_len = 4 * ((len + 2) / 3);
    char* out = malloc(out_len + 1);

    size_t i, o = 0;
    for (i = 0; i < len; i += 3) {
        uint32_t n = (uint32_t) in[i] << 16;
        if (i + 1 < len) n |= (uint32_t) in[i + 1] << 8;
        if (i + 2 < len) n |= (uint32_t) in[i + 2];
        out[o++] = tbl[(n >> 18) & 63];
        out[o++] = tbl[(n >> 12) & 63];
        out[o++] = (i + 1 < len) ? tbl[(n >> 6) & 63] : '=';
        out[o++] = (i + 2 < len) ? tbl[n & 63] : '=';
    }
    out[o] = '\0';

    return out;

}

/**
 * Writes a single Guacamole instruction to the given file from the given array
 * of string elements (the first of which is the opcode).
 */
static void emit_argv(FILE* f, int argc, const char* const* argv) {

    int i;
    for (i = 0; i < argc; i++) {
        if (i != 0)
            fputc(',', f);
        fprintf(f, "%zu.%s", strlen(argv[i]), argv[i]);
    }
    fputc(';', f);

}

/**
 * Writes a single Guacamole instruction to the given file. The variadic
 * arguments are the instruction's elements (opcode first) as const char*, and
 * MUST be terminated by a NULL pointer.
 */
static void emit(FILE* f, ...) {

    const char* argv[32];
    int argc = 0;

    va_list ap;
    va_start(ap, f);

    const char* element;
    while ((element = va_arg(ap, const char*)) != NULL && argc < 32)
        argv[argc++] = element;

    va_end(ap);

    emit_argv(f, argc, argv);

}

/**
 * Writes a "blob" instruction to the given file for the given stream index,
 * base64-encoding the given raw bytes as the blob payload.
 */
static void emit_blob(FILE* f, const char* index, const void* data,
        size_t len) {

    char* b64 = b64_encode(data, len);
    emit(f, "blob", index, b64, NULL);
    free(b64);

}

/**
 * Initializes the given workspace, creating a fresh temporary directory and
 * deriving the recording and output paths within it.
 */
static void ws_init(workspace* ws) {

    char template[] = "/tmp/guacclip_test_XXXXXX";
    char* root = mkdtemp(template);
    CU_ASSERT_PTR_NOT_NULL_FATAL(root);

    snprintf(ws->root, sizeof(ws->root), "%s", root);
    snprintf(ws->rec, sizeof(ws->rec), "%s/recording.guac", root);
    snprintf(ws->out, sizeof(ws->out), "%s/out", root);

}

/**
 * Recursively removes the given workspace's temporary directory.
 */
static void ws_cleanup(workspace* ws) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", ws->root);
    int result = system(cmd);
    (void) result;
}

/**
 * Returns a guacclip_options structure populated with the same defaults used
 * by the command-line tool.
 */
static guacclip_options default_opts(void) {
    guacclip_options options = {
        .outdir           = NULL,
        .direction_filter = NULL,
        .include          = GUACCLIP_INCLUDE_ALL,
        .max_item_bytes   = 64 * 1024 * 1024,
        .dedup            = GUACCLIP_DEDUP_NONE
    };
    return options;
}

/**
 * Interprets the recording in the given workspace using the given options
 * (whose outdir is overridden to the workspace output directory). Logging is
 * suppressed to keep test output clean. Returns the result of
 * guacclip_interpret().
 */
static int run_interpret(workspace* ws, guacclip_options options) {
    guacclip_log_level = GUAC_LOG_ERROR;
    options.outdir = ws->out;
    return guacclip_interpret(ws->rec, &options, true);
}

/**
 * Reads the entire contents of the given file into a newly-allocated,
 * null-terminated buffer. The number of bytes read (excluding the null
 * terminator) is stored in *len_out if len_out is non-NULL. Returns NULL on
 * failure.
 */
static char* read_file(const char* path, size_t* len_out) {

    if (len_out != NULL)
        *len_out = 0;

    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return NULL;
    }

    char* buffer = malloc((size_t) size + 1);
    size_t read = fread(buffer, 1, (size_t) size, f);
    fclose(f);

    buffer[read] = '\0';
    if (len_out != NULL)
        *len_out = read;

    return buffer;

}

/**
 * Reads the manifest.json of the given output directory into a newly-allocated,
 * null-terminated string. Returns NULL on failure.
 */
static char* read_manifest(const char* outdir) {
    char path[900];
    snprintf(path, sizeof(path), "%s/manifest.json", outdir);
    return read_file(path, NULL);
}

/**
 * Counts the number of regular entries within the "items" subdirectory of the
 * given output directory. Returns -1 if the directory cannot be opened.
 */
static int count_items(const char* outdir) {

    char items[900];
    snprintf(items, sizeof(items), "%s/items", outdir);

    DIR* dir = opendir(items);
    if (dir == NULL)
        return -1;

    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0
                && strcmp(entry->d_name, "..") != 0)
            count++;
    }

    closedir(dir);
    return count;

}

/**
 * Reads the single item file found within the given output directory's "items"
 * subdirectory into a newly-allocated buffer, storing its length in *len_out.
 * Intended for tests which expect exactly one item. Returns NULL if no item is
 * found.
 */
static char* read_only_item(const char* outdir, size_t* len_out) {

    char items[900];
    snprintf(items, sizeof(items), "%s/items", outdir);

    DIR* dir = opendir(items);
    if (dir == NULL)
        return NULL;

    char* result = NULL;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0
                || strcmp(entry->d_name, "..") == 0)
            continue;
        char path[1900];
        snprintf(path, sizeof(path), "%s/%s", items, entry->d_name);
        result = read_file(path, len_out);
        break;
    }

    closedir(dir);
    return result;

}

/**
 * Returns whether some item file within the given output directory's "items"
 * subdirectory has content exactly matching the given bytes.
 */
static bool item_content_exists(const char* outdir, const void* data,
        size_t len) {

    char items[900];
    snprintf(items, sizeof(items), "%s/items", outdir);

    DIR* dir = opendir(items);
    if (dir == NULL)
        return false;

    bool found = false;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0
                || strcmp(entry->d_name, "..") == 0)
            continue;
        char path[1900];
        snprintf(path, sizeof(path), "%s/%s", items, entry->d_name);
        size_t item_len;
        char* content = read_file(path, &item_len);
        if (content != NULL && item_len == len
                && memcmp(content, data, len) == 0)
            found = true;
        free(content);
        if (found)
            break;
    }

    closedir(dir);
    return found;

}

/**
 * Test which verifies that a clipboard stream split across several blobs is
 * reassembled into the exact original byte sequence.
 */
void test_interpret__multiblob(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    emit_blob(f, "1", "Hello", 5);
    emit_blob(f, "1", ", ", 2);
    emit_blob(f, "1", "World!", 6);
    emit(f, "end", "1", NULL);
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    CU_ASSERT_EQUAL(count_items(ws.out), 1);

    size_t len;
    char* content = read_only_item(ws.out, &len);
    CU_ASSERT_PTR_NOT_NULL_FATAL(content);
    CU_ASSERT_EQUAL(len, 13);
    CU_ASSERT_NSTRING_EQUAL(content, "Hello, World!", 13);
    free(content);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that two interleaved clipboard streams are reassembled
 * independently and correctly.
 */
void test_interpret__interleaved(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    emit(f, "clipboard", "2", "text/plain", NULL);
    emit_blob(f, "1", "AA", 2);
    emit_blob(f, "2", "BBB", 3);
    emit_blob(f, "1", "AA", 2);
    emit_blob(f, "2", "BBB", 3);
    emit(f, "end", "1", NULL);
    emit(f, "end", "2", NULL);
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    CU_ASSERT_EQUAL(count_items(ws.out), 2);
    CU_ASSERT_TRUE(item_content_exists(ws.out, "AAAA", 4));
    CU_ASSERT_TRUE(item_content_exists(ws.out, "BBBBBB", 6));

    ws_cleanup(&ws);

}

/**
 * Test which verifies that a preceding "log" direction annotation is correlated
 * with the clipboard stream it describes.
 */
void test_interpret__direction(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "log",
            "clipboard stream=5 direction=guest-to-client mimetype=text/plain",
            NULL);
    emit(f, "clipboard", "5", "text/plain", NULL);
    emit_blob(f, "5", "secret", 6);
    emit(f, "end", "5", NULL);
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(
            strstr(manifest, "\"direction\": \"guest-to-client\""));
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that a clipboard stream lacking any direction annotation
 * is reported with direction "unknown".
 */
void test_interpret__unknown_direction(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    emit_blob(f, "1", "data", 4);
    emit(f, "end", "1", NULL);
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"direction\": \"unknown\""));
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies the core guard: blob/end instructions on a stream index
 * that was never opened by a "clipboard" instruction (e.g. a graphical image
 * stream) produce no clipboard item.
 */
void test_interpret__non_clipboard_stream(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    /* Stream 7 is never opened by a clipboard instruction */
    emit_blob(f, "7", "\x89PNG\r\n", 6);
    emit(f, "end", "7", NULL);
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    CU_ASSERT_EQUAL(count_items(ws.out), 0);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"generated_items\": 0"));
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that a stream still open at end-of-file yields an item
 * marked incomplete.
 */
void test_interpret__truncation(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    emit_blob(f, "1", "partial", 7);
    /* No "end" instruction: the stream is truncated at EOF */
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    /* The partial content is still written */
    CU_ASSERT_EQUAL(count_items(ws.out), 1);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"complete\": false"));
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "stream-open-at-eof"));
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that a clipboard item exceeding the configured
 * --max-item-bytes is reported as oversized and is not written to disk.
 */
void test_interpret__oversized(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    emit_blob(f, "1", "abcdefgh", 8);
    emit(f, "end", "1", NULL);
    fclose(f);

    guacclip_options options = default_opts();
    options.max_item_bytes = 4;
    CU_ASSERT_EQUAL(run_interpret(&ws, options), 0);

    /* Oversized content is never written */
    CU_ASSERT_EQUAL(count_items(ws.out), 0);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "oversized"));
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"filename\": null"));
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that a malformed base64 blob payload is handled without
 * crashing.
 */
void test_interpret__bad_base64(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    /* Emit a raw, deliberately-malformed base64 payload */
    emit(f, "blob", "1", "!!!!not-valid-base64", NULL);
    emit(f, "end", "1", NULL);
    fclose(f);

    /* The recording is interpreted without crashing */
    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that a clipboard instruction bearing a non-numeric stream
 * index is ignored, producing no item.
 */
void test_interpret__non_numeric_stream(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "x", "text/plain", NULL);
    emit(f, "blob", "x", "aGk=", NULL);
    emit(f, "end", "x", NULL);
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    CU_ASSERT_EQUAL(count_items(ws.out), 0);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"generated_items\": 0"));
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that, under --dedup none, two identical clipboard items
 * are both written to disk, with the second annotated with duplicate_of set to
 * the first item's sequence number.
 */
void test_interpret__dedup_none(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    emit_blob(f, "1", "duplicate", 9);
    emit(f, "end", "1", NULL);
    emit(f, "clipboard", "2", "text/plain", NULL);
    emit_blob(f, "2", "duplicate", 9);
    emit(f, "end", "2", NULL);
    fclose(f);

    CU_ASSERT_EQUAL(run_interpret(&ws, default_opts()), 0);

    /* Both copies are written */
    CU_ASSERT_EQUAL(count_items(ws.out), 2);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"duplicate_of\": 0"));
    /* No "duplicate" warning is emitted in none mode */
    CU_ASSERT_PTR_NULL(strstr(manifest, "\"duplicate\""));
    free(manifest);

    ws_cleanup(&ws);

}

/**
 * Test which verifies that, under --dedup skip, the second of two identical
 * clipboard items is not written to disk (filename null), is annotated with a
 * "duplicate" warning, and still carries duplicate_of in the manifest.
 */
void test_interpret__dedup_skip(void) {

    workspace ws;
    ws_init(&ws);

    FILE* f = fopen(ws.rec, "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    emit(f, "sync", "1000", NULL);
    emit(f, "clipboard", "1", "text/plain", NULL);
    emit_blob(f, "1", "duplicate", 9);
    emit(f, "end", "1", NULL);
    emit(f, "clipboard", "2", "text/plain", NULL);
    emit_blob(f, "2", "duplicate", 9);
    emit(f, "end", "2", NULL);
    fclose(f);

    guacclip_options options = default_opts();
    options.dedup = GUACCLIP_DEDUP_SKIP;
    CU_ASSERT_EQUAL(run_interpret(&ws, options), 0);

    /* Only the first copy is written to disk */
    CU_ASSERT_EQUAL(count_items(ws.out), 1);

    char* manifest = read_manifest(ws.out);
    CU_ASSERT_PTR_NOT_NULL_FATAL(manifest);
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"duplicate_of\": 0"));
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"filename\": null"));
    CU_ASSERT_PTR_NOT_NULL(strstr(manifest, "\"duplicate\""));
    free(manifest);

    ws_cleanup(&ws);

}
