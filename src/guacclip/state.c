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
#include "log.h"
#include "sha256.h"
#include "state.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * The initial capacity, in bytes, allocated for a clipboard stream's
 * accumulation buffer.
 */
#define GUACCLIP_INITIAL_BUFFER 4096

/**
 * The initial capacity, in entries, of the manifest items array.
 */
#define GUACCLIP_INITIAL_ITEMS 16

/**
 * Creates the given directory if it does not already exist, behaving like
 * "mkdir -p" for a single path component depth (the parent is assumed to
 * exist). An already-existing directory is not treated as an error.
 *
 * @param path
 *     The directory path to create.
 *
 * @return
 *     Zero on success (including when the directory already exists), non-zero
 *     on failure.
 */
static int guacclip_mkdir(const char* path) {

    if (mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0)
        return 0;

    /* Existing directory is acceptable */
    if (errno == EEXIST)
        return 0;

    guacclip_log(GUAC_LOG_ERROR, "Failed to create directory \"%s\": %s",
            path, strerror(errno));
    return 1;

}

/**
 * Returns the file extension (including leading dot) appropriate for the given
 * clipboard mimetype.
 *
 * @param mimetype
 *     The mimetype to map to an extension.
 *
 * @return
 *     A statically-allocated extension string, including the leading dot.
 */
static const char* guacclip_extension_for(const char* mimetype) {

    if (strcmp(mimetype, "text/plain") == 0)
        return ".txt";
    if (strcmp(mimetype, "image/png") == 0)
        return ".png";
    if (strcmp(mimetype, "image/jpeg") == 0)
        return ".jpg";
    if (strcmp(mimetype, "image/bmp") == 0)
        return ".bmp";
    if (strcmp(mimetype, "image/tiff") == 0)
        return ".tiff";

    return ".bin";

}

/**
 * Writes a filesystem-safe "slug" derived from the given mimetype into the
 * given buffer. All characters other than ASCII alphanumerics are replaced
 * with underscores.
 *
 * @param mimetype
 *     The mimetype to convert into a slug.
 *
 * @param out
 *     The buffer to receive the null-terminated slug.
 *
 * @param out_size
 *     The size of the output buffer, in bytes.
 */
static void guacclip_mime_slug(const char* mimetype, char* out,
        size_t out_size) {

    size_t i;
    for (i = 0; mimetype[i] != '\0' && i < out_size - 1; i++) {
        char c = mimetype[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9'))
            out[i] = c;
        else
            out[i] = '_';
    }
    out[i] = '\0';

}

/**
 * Returns whether clipboard content having the given mimetype should be
 * extracted, according to the configured include mode.
 *
 * @param options
 *     The extraction options.
 *
 * @param mimetype
 *     The mimetype to test.
 *
 * @return
 *     true if the content should be extracted, false otherwise.
 */
static bool guacclip_include_matches(const guacclip_options* options,
        const char* mimetype) {

    switch (options->include) {

        case GUACCLIP_INCLUDE_ALL:
            return true;

        case GUACCLIP_INCLUDE_IMAGE:
            return strncmp(mimetype, "image/", 6) == 0;

        case GUACCLIP_INCLUDE_TEXT:
            return strncmp(mimetype, "text/", 5) == 0;

    }

    return true;

}

/**
 * Locates the active clipboard stream having the given stream index.
 *
 * @param state
 *     The interpreter state to search.
 *
 * @param index
 *     The stream index to locate.
 *
 * @return
 *     A pointer to the matching stream, or NULL if no clipboard stream is open
 *     on the given index.
 */
static guacclip_stream* guacclip_find_stream(guacclip_state* state, int index) {

    int i;
    for (i = 0; i < GUACCLIP_MAX_STREAMS; i++) {
        if (state->streams[i].in_use && state->streams[i].index == index)
            return &state->streams[i];
    }

    return NULL;

}

/**
 * Adds a warning message to the given manifest item, if space remains.
 *
 * @param item
 *     The manifest item to annotate.
 *
 * @param warning
 *     The warning message to add. A copy is stored.
 */
static void guacclip_item_warn(guacclip_item* item, const char* warning) {

    if (item->num_warnings >= GUACCLIP_MAX_WARNINGS)
        return;

    char* copy = strdup(warning);
    if (copy == NULL)
        return;

    item->warnings[item->num_warnings++] = copy;

}

/**
 * Duplicates the given string, falling back to duplicating the given
 * fallback literal if either the input is NULL or the duplication of the
 * input fails due to memory exhaustion. This function never returns NULL: if
 * even the fallback cannot be duplicated, the process is treated as being
 * fatally out of memory.
 *
 * @param value
 *     The string to duplicate, or NULL to duplicate the fallback directly.
 *
 * @param fallback
 *     The non-NULL fallback string to duplicate if value is NULL or if
 *     duplicating value fails.
 *
 * @return
 *     A newly-allocated, heap-owned copy of value or fallback.
 */
static char* guacclip_strdup_or(const char* value, const char* fallback) {

    char* copy = value != NULL ? strdup(value) : NULL;
    if (copy != NULL)
        return copy;

    copy = strdup(fallback);
    if (copy != NULL)
        return copy;

    guacclip_log(GUAC_LOG_ERROR, "Out of memory duplicating string.");
    exit(1);

}

/**
 * Creates and opens the given temporary file path for writing, exclusively.
 * The file is created with O_EXCL and O_NOFOLLOW so that a pre-existing file
 * or symlink already present at that path (as might be planted by another
 * process with write access to the output directory) causes the open to
 * fail rather than being silently followed or overwritten, closing a
 * symlink-race window inherent to opening a deterministic temporary path.
 *
 * @param tmp_path
 *     The temporary file path to create and open.
 *
 * @return
 *     A FILE* opened for writing in binary mode, or NULL on failure (with an
 *     error already logged).
 */
static FILE* guacclip_create_tmp(const char* tmp_path) {

    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW,
            S_IRUSR | S_IWUSR);
    if (fd < 0) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to create \"%s\": %s",
                tmp_path, strerror(errno));
        return NULL;
    }

    FILE* file = fdopen(fd, "wb");
    if (file == NULL) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to allocate stream for \"%s\": "
                "%s", tmp_path, strerror(errno));
        close(fd);
        unlink(tmp_path);
        return NULL;
    }

    return file;

}

/**
 * Writes the given bytes to a new file at the given path using a temporary
 * file and atomic rename, ensuring that no partially-written file is ever
 * visible at the destination path.
 *
 * @param path
 *     The final destination path.
 *
 * @param data
 *     The bytes to write.
 *
 * @param length
 *     The number of bytes to write.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guacclip_write_file(const char* path, const char* data,
        size_t length) {

    /* Build temporary path alongside the destination */
    char tmp_path[4096];
    int len = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    if (len < 0 || (size_t) len >= sizeof(tmp_path)) {
        guacclip_log(GUAC_LOG_ERROR, "Output path too long: \"%s\"", path);
        return 1;
    }

    /* Open temporary file for writing, exclusively (guards against a
     * pre-planted file or symlink at the deterministic temporary path) */
    FILE* file = guacclip_create_tmp(tmp_path);
    if (file == NULL)
        return 1;

    /* Write payload (binary-safe) */
    if (length > 0 && fwrite(data, 1, length, file) != length) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to write \"%s\": %s",
                tmp_path, strerror(errno));
        fclose(file);
        unlink(tmp_path);
        return 1;
    }

    if (fclose(file) != 0) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to flush \"%s\": %s",
                tmp_path, strerror(errno));
        unlink(tmp_path);
        return 1;
    }

    /* Atomically move into place */
    if (rename(tmp_path, path) != 0) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to rename \"%s\" to \"%s\": %s",
                tmp_path, path, strerror(errno));
        unlink(tmp_path);
        return 1;
    }

    return 0;

}

/**
 * Ensures the manifest items array has room for at least one more entry,
 * growing it if necessary.
 *
 * @param state
 *     The interpreter state whose items array should be grown.
 *
 * @return
 *     Zero on success, non-zero if reallocation failed.
 */
static int guacclip_ensure_item_capacity(guacclip_state* state) {

    if (state->num_items < state->items_capacity)
        return 0;

    /* Enforce a hard cap on the total number of manifest items to bound
     * memory growth against a hostile recording containing an extremely
     * large number of clipboard open/end pairs */
    if (state->num_items >= GUACCLIP_MAX_ITEMS) {
        if (!state->item_limit_warned) {
            guacclip_log(GUAC_LOG_WARNING, "Maximum number of manifest items "
                    "(%d) reached; further clipboard items will be dropped "
                    "from the manifest.", GUACCLIP_MAX_ITEMS);
            state->item_limit_warned = true;
        }
        return 1;
    }

    int new_capacity = state->items_capacity == 0
            ? GUACCLIP_INITIAL_ITEMS : state->items_capacity * 2;
    if (new_capacity > GUACCLIP_MAX_ITEMS)
        new_capacity = GUACCLIP_MAX_ITEMS;

    guacclip_item* resized = realloc(state->items,
            sizeof(guacclip_item) * new_capacity);
    if (resized == NULL) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to grow manifest items array.");
        return 1;
    }

    state->items = resized;
    state->items_capacity = new_capacity;
    return 0;

}

/**
 * Releases all memory associated with the given clipboard stream and marks its
 * slot as free.
 *
 * @param stream
 *     The clipboard stream to release.
 */
static void guacclip_stream_release(guacclip_stream* stream) {
    free(stream->mimetype);
    free(stream->direction);
    free(stream->buffer);
    memset(stream, 0, sizeof(*stream));
}

/**
 * Locates the earliest previously-finalized manifest item whose SHA-256 digest
 * matches the given digest, returning its sequence number. Used to populate the
 * "duplicate_of" field and to drive --dedup handling. Only items which had a
 * digest computed (i.e. non-oversized items) are considered.
 *
 * @param state
 *     The interpreter state whose already-finalized items should be searched.
 *
 * @param sha256
 *     The lowercase hexadecimal SHA-256 digest to search for.
 *
 * @return
 *     The sequence number of the first (lowest-sequence) earlier item having
 *     an identical digest, or -1 if no earlier item shares the digest.
 */
static int guacclip_find_duplicate(guacclip_state* state, const char* sha256) {

    int i;
    int first = -1;

    for (i = 0; i < state->num_items; i++) {
        guacclip_item* other = &state->items[i];
        if (other->sha256[0] != '\0' && strcmp(other->sha256, sha256) == 0
                && (first < 0 || other->sequence < first))
            first = other->sequence;
    }

    return first;

}

/**
 * Finalizes the given clipboard stream, computing its digest, writing its file
 * (when appropriate), and appending a manifest entry. The stream is released
 * before this function returns.
 *
 * @param state
 *     The interpreter state owning the stream.
 *
 * @param stream
 *     The clipboard stream to finalize.
 *
 * @param incomplete_reason
 *     NULL if finalization is occurring due to a normal "end" instruction, or
 *     a short warning string (added to the item's warnings and forcing
 *     complete=false) if finalization is occurring for some other reason,
 *     such as end-of-file being reached with the stream still open, or the
 *     stream being reopened before it was closed.
 */
static void guacclip_finalize_stream(guacclip_state* state,
        guacclip_stream* stream, const char* incomplete_reason) {

    /* Streams tracked purely for correct blob/end consumption are dropped */
    if (stream->skipped) {
        guacclip_stream_release(stream);
        return;
    }

    if (guacclip_ensure_item_capacity(state)) {
        guacclip_stream_release(stream);
        return;
    }

    guacclip_item* item = &state->items[state->num_items];
    memset(item, 0, sizeof(*item));

    item->sequence = stream->sequence;
    item->stream = stream->index;
    item->direction = guacclip_strdup_or(stream->direction, "unknown");
    item->mimetype = guacclip_strdup_or(stream->mimetype,
            "application/octet-stream");
    item->offset_ms = stream->offset_ms;
    item->sync_timestamp = stream->sync_timestamp;
    item->bytes = stream->length;
    item->expected_bytes = stream->expected_bytes;
    item->complete = true;
    item->filename = NULL;
    item->sha256[0] = '\0';
    item->duplicate_of = -1;

    /* An oversized item is never written to disk */
    if (stream->oversized) {
        item->complete = false;
        guacclip_item_warn(item, "oversized");
    }

    /* A stream finalized for any reason other than a normal "end"
     * instruction (e.g. still open at EOF, or reopened before being closed)
     * is incomplete */
    if (incomplete_reason != NULL) {
        item->complete = false;
        guacclip_item_warn(item, incomplete_reason);
    }

    /* Detect a mismatch against the annotated byte count */
    if (stream->expected_bytes >= 0
            && (size_t) stream->expected_bytes != stream->length) {
        item->complete = false;
        guacclip_item_warn(item, "byte-count-mismatch");
    }

    /* Compute digest and write file only for non-oversized items */
    if (!stream->oversized) {

        guacclip_sha256_hex(stream->buffer, stream->length, item->sha256);

        /* Correlate against any earlier item having identical content */
        item->duplicate_of = guacclip_find_duplicate(state, item->sha256);
        bool is_duplicate = item->duplicate_of >= 0;

        /* In "skip" dedup mode, a duplicate's content is not written to disk;
         * the item is still recorded in the manifest (with duplicate_of set)
         * and annotated with a "duplicate" warning */
        if (is_duplicate
                && state->options->dedup == GUACCLIP_DEDUP_SKIP) {
            guacclip_item_warn(item, "duplicate");
        }
        else {

        /* Build slug and 8-char digest prefix for the filename */
        char slug[128];
        guacclip_mime_slug(item->mimetype, slug, sizeof(slug));

        char prefix[9];
        memcpy(prefix, item->sha256, 8);
        prefix[8] = '\0';

        const char* ext = guacclip_extension_for(item->mimetype);

        /* Compose the relative item filename */
        char rel_name[1024];
        int rlen = snprintf(rel_name, sizeof(rel_name),
                "%d_%" PRId64 "ms_%s_stream%d_%s_%s%s",
                item->sequence, item->offset_ms, item->direction,
                item->stream, slug, prefix, ext);

        if (rlen < 0 || (size_t) rlen >= sizeof(rel_name)) {
            guacclip_log(GUAC_LOG_WARNING,
                    "Item filename too long for stream %d; not written.",
                    item->stream);
            guacclip_item_warn(item, "filename-too-long");
            item->complete = false;
        }
        else {

            /* Build absolute path within the items subdirectory */
            char full_path[4096];
            int flen = snprintf(full_path, sizeof(full_path), "%s/items/%s",
                    state->outdir, rel_name);

            if (flen < 0 || (size_t) flen >= sizeof(full_path)) {
                guacclip_log(GUAC_LOG_WARNING,
                        "Item path too long for stream %d; not written.",
                        item->stream);
                guacclip_item_warn(item, "path-too-long");
                item->complete = false;
            }
            else if (guacclip_write_file(full_path, stream->buffer,
                        stream->length)) {
                guacclip_item_warn(item, "write-failed");
                item->complete = false;
            }
            else {
                /* Record path relative to the output directory */
                char rel_path[1200];
                snprintf(rel_path, sizeof(rel_path), "items/%s", rel_name);
                item->filename = strdup(rel_path);
            }

        }

        }

    }

    state->num_items++;

    guacclip_log(GUAC_LOG_INFO, "Extracted clipboard item: seq=%d stream=%d "
            "direction=%s mimetype=%s bytes=%zu complete=%s",
            item->sequence, item->stream, item->direction, item->mimetype,
            item->bytes, item->complete ? "true" : "false");

    guacclip_stream_release(stream);

}

guacclip_state* guacclip_state_alloc(const char* recording_path,
        const guacclip_options* options) {

    /* Create output directory and items subdirectory */
    if (guacclip_mkdir(options->outdir))
        return NULL;

    char items_dir[4096];
    int len = snprintf(items_dir, sizeof(items_dir), "%s/items",
            options->outdir);
    if (len < 0 || (size_t) len >= sizeof(items_dir)) {
        guacclip_log(GUAC_LOG_ERROR, "Output directory path too long: \"%s\"",
                options->outdir);
        return NULL;
    }

    if (guacclip_mkdir(items_dir))
        return NULL;

    /* Allocate zero-initialized state */
    guacclip_state* state = calloc(1, sizeof(guacclip_state));
    if (state == NULL) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to allocate interpreter state.");
        return NULL;
    }

    state->options = options;
    state->recording_path = recording_path;
    state->outdir = options->outdir;
    state->next_sequence = 0;

    return state;

}

void guacclip_state_sync(guacclip_state* state, int64_t timestamp) {

    if (!state->has_sync) {
        state->has_sync = true;
        state->first_sync_timestamp = timestamp;
    }

    state->last_sync_timestamp = timestamp;

}

void guacclip_state_buffer_direction(guacclip_state* state, int index,
        const char* direction, int64_t expected_bytes) {

    int i;

    /* Update any existing pending annotation for this index */
    for (i = 0; i < GUACCLIP_MAX_PENDING; i++) {
        if (state->pending[i].in_use && state->pending[i].index == index) {
            free(state->pending[i].direction);
            state->pending[i].direction = strdup(direction);
            state->pending[i].expected_bytes = expected_bytes;
            return;
        }
    }

    /* Otherwise, occupy a free slot */
    for (i = 0; i < GUACCLIP_MAX_PENDING; i++) {
        if (!state->pending[i].in_use) {
            state->pending[i].in_use = true;
            state->pending[i].index = index;
            state->pending[i].direction = strdup(direction);
            state->pending[i].expected_bytes = expected_bytes;
            return;
        }
    }

    guacclip_log(GUAC_LOG_WARNING, "Too many pending clipboard direction "
            "annotations; dropping annotation for stream %d.", index);

}

/**
 * Consumes and removes any buffered direction annotation for the given stream
 * index.
 *
 * @param state
 *     The interpreter state to search.
 *
 * @param index
 *     The stream index whose annotation should be consumed.
 *
 * @param direction
 *     Receives a newly-allocated copy of the direction string, or NULL if no
 *     annotation was buffered. The caller owns the returned string.
 *
 * @param expected_bytes
 *     Receives the annotated byte count, or -1 if none was buffered.
 */
static void guacclip_consume_direction(guacclip_state* state, int index,
        char** direction, int64_t* expected_bytes) {

    int i;

    *direction = NULL;
    *expected_bytes = -1;

    for (i = 0; i < GUACCLIP_MAX_PENDING; i++) {
        if (state->pending[i].in_use && state->pending[i].index == index) {
            *direction = state->pending[i].direction;
            *expected_bytes = state->pending[i].expected_bytes;
            state->pending[i].in_use = false;
            state->pending[i].direction = NULL;
            state->pending[i].index = 0;
            state->pending[i].expected_bytes = -1;
            return;
        }
    }

}

int guacclip_state_open(guacclip_state* state, int index,
        const char* mimetype) {

    /* A duplicate open on an in-use index likely indicates malformed input;
     * finalize the previous stream (marked incomplete) so its partial
     * contents are still recorded in the manifest, rather than silently
     * discarding them */
    guacclip_stream* existing = guacclip_find_stream(state, index);
    if (existing != NULL) {
        guacclip_log(GUAC_LOG_WARNING, "Clipboard stream %d reopened before "
                "being closed; finalizing previous (incomplete) contents.",
                index);
        guacclip_finalize_stream(state, existing, "stream-reopened-before-end");
    }

    /* Locate a free stream slot */
    guacclip_stream* stream = NULL;
    int i;
    for (i = 0; i < GUACCLIP_MAX_STREAMS; i++) {
        if (!state->streams[i].in_use) {
            stream = &state->streams[i];
            break;
        }
    }

    if (stream == NULL) {
        guacclip_log(GUAC_LOG_WARNING, "Too many concurrent clipboard "
                "streams; ignoring stream %d.", index);
        return 1;
    }

    /* Consume any buffered direction annotation for this stream */
    char* direction;
    int64_t expected_bytes;
    guacclip_consume_direction(state, index, &direction, &expected_bytes);

    memset(stream, 0, sizeof(*stream));
    stream->in_use = true;
    stream->index = index;
    stream->mimetype = strdup(mimetype);
    stream->direction = direction != NULL ? direction : strdup("unknown");
    stream->expected_bytes = expected_bytes;
    stream->sequence = state->next_sequence++;
    stream->sync_timestamp = state->last_sync_timestamp;
    stream->offset_ms = state->has_sync
            ? state->last_sync_timestamp - state->first_sync_timestamp : 0;
    stream->buffer = NULL;
    stream->length = 0;
    stream->capacity = 0;
    stream->oversized = false;

    /* Determine whether this stream should actually be extracted */
    bool included = guacclip_include_matches(state->options, mimetype);
    bool direction_ok = state->options->direction_filter == NULL
            || strcmp(stream->direction,
                    state->options->direction_filter) == 0;
    stream->skipped = !(included && direction_ok);

    return 0;

}

void guacclip_state_append(guacclip_state* state, int index,
        const char* data, size_t length) {

    guacclip_stream* stream = guacclip_find_stream(state, index);

    /* Ignore blobs for non-clipboard streams (img, file, pipe, argv, ...) */
    if (stream == NULL)
        return;

    /* Skipped streams and already-oversized streams accumulate nothing */
    if (stream->skipped || stream->oversized)
        return;

    /* Nothing to accumulate for an empty (or fully-invalid) blob; returning
     * early here avoids an undefined memcpy(NULL, ..., 0) below when the
     * stream's buffer has not yet been allocated */
    if (length == 0)
        return;

    /* Enforce the per-item size cap */
    size_t max = state->options->max_item_bytes;
    if (max != 0 && stream->length + length > max) {
        stream->oversized = true;
        free(stream->buffer);
        stream->buffer = NULL;
        stream->capacity = 0;
        guacclip_log(GUAC_LOG_WARNING, "Clipboard stream %d exceeded maximum "
                "item size (%zu bytes); marking oversized.", index, max);
        return;
    }

    /* Grow the accumulation buffer as needed */
    if (stream->length + length > stream->capacity) {
        size_t new_capacity = stream->capacity == 0
                ? GUACCLIP_INITIAL_BUFFER : stream->capacity;
        while (stream->length + length > new_capacity)
            new_capacity *= 2;

        char* resized = realloc(stream->buffer, new_capacity);
        if (resized == NULL) {
            guacclip_log(GUAC_LOG_ERROR, "Failed to grow buffer for clipboard "
                    "stream %d.", index);
            return;
        }

        stream->buffer = resized;
        stream->capacity = new_capacity;
    }

    /* Copy decoded bytes into the accumulation buffer (binary-safe) */
    memcpy(stream->buffer + stream->length, data, length);
    stream->length += length;

}

void guacclip_state_end(guacclip_state* state, int index) {

    guacclip_stream* stream = guacclip_find_stream(state, index);

    /* Ignore end for non-clipboard streams */
    if (stream == NULL)
        return;

    guacclip_finalize_stream(state, stream, NULL);

}

/**
 * Writes the given string to the given file as a JSON string literal,
 * including the surrounding quotes and escaping all characters as required by
 * the JSON specification.
 *
 * @param file
 *     The output file to write to.
 *
 * @param value
 *     The string to write, or NULL to write a JSON null.
 */
static void guacclip_json_string(FILE* file, const char* value) {

    if (value == NULL) {
        fprintf(file, "null");
        return;
    }

    fputc('"', file);

    const unsigned char* c = (const unsigned char*) value;
    for (; *c != '\0'; c++) {
        switch (*c) {
            case '"':  fprintf(file, "\\\""); break;
            case '\\': fprintf(file, "\\\\"); break;
            case '\b': fprintf(file, "\\b");  break;
            case '\f': fprintf(file, "\\f");  break;
            case '\n': fprintf(file, "\\n");  break;
            case '\r': fprintf(file, "\\r");  break;
            case '\t': fprintf(file, "\\t");  break;
            default:
                /* Escape both control characters and any byte outside the
                 * printable ASCII range (treating it as a Latin-1 code
                 * point), ensuring the manifest remains valid, parseable
                 * JSON even when a mimetype or other attacker-controlled
                 * string contains non-UTF-8 byte sequences */
                if (*c < 0x20 || *c > 0x7F)
                    fprintf(file, "\\u%04x", *c);
                else
                    fputc(*c, file);
                break;
        }
    }

    fputc('"', file);

}

/**
 * Writes the manifest.json file summarizing all extracted items into the
 * output directory.
 *
 * @param state
 *     The interpreter state whose items should be serialized.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guacclip_write_manifest(guacclip_state* state) {

    char path[4096];
    int len = snprintf(path, sizeof(path), "%s/manifest.json", state->outdir);
    if (len < 0 || (size_t) len >= sizeof(path)) {
        guacclip_log(GUAC_LOG_ERROR, "Manifest path too long.");
        return 1;
    }

    char tmp_path[4096];
    len = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    if (len < 0 || (size_t) len >= sizeof(tmp_path)) {
        guacclip_log(GUAC_LOG_ERROR, "Manifest temp path too long.");
        return 1;
    }

    FILE* file = guacclip_create_tmp(tmp_path);
    if (file == NULL)
        return 1;

    fprintf(file, "{\n");
    fprintf(file, "  \"recording\": ");
    guacclip_json_string(file, state->recording_path);
    fprintf(file, ",\n");
    fprintf(file, "  \"generated_items\": %d,\n", state->num_items);
    fprintf(file, "  \"items\": [");

    int i;
    for (i = 0; i < state->num_items; i++) {

        guacclip_item* item = &state->items[i];

        fprintf(file, "%s\n    {\n", i == 0 ? "" : ",");
        fprintf(file, "      \"sequence\": %d,\n", item->sequence);
        fprintf(file, "      \"stream\": %d,\n", item->stream);

        fprintf(file, "      \"direction\": ");
        guacclip_json_string(file, item->direction);
        fprintf(file, ",\n");

        fprintf(file, "      \"mimetype\": ");
        guacclip_json_string(file, item->mimetype);
        fprintf(file, ",\n");

        fprintf(file, "      \"filename\": ");
        guacclip_json_string(file, item->filename);
        fprintf(file, ",\n");

        fprintf(file, "      \"offset_ms\": %" PRId64 ",\n", item->offset_ms);
        fprintf(file, "      \"sync_timestamp\": %" PRId64 ",\n",
                item->sync_timestamp);
        fprintf(file, "      \"bytes\": %zu,\n", item->bytes);

        fprintf(file, "      \"expected_bytes\": ");
        if (item->expected_bytes >= 0)
            fprintf(file, "%" PRId64, item->expected_bytes);
        else
            fprintf(file, "null");
        fprintf(file, ",\n");

        fprintf(file, "      \"sha256\": ");
        guacclip_json_string(file, item->sha256[0] != '\0' ? item->sha256 : NULL);
        fprintf(file, ",\n");

        fprintf(file, "      \"duplicate_of\": ");
        if (item->duplicate_of >= 0)
            fprintf(file, "%d", item->duplicate_of);
        else
            fprintf(file, "null");
        fprintf(file, ",\n");

        fprintf(file, "      \"complete\": %s,\n",
                item->complete ? "true" : "false");

        fprintf(file, "      \"warnings\": [");
        int w;
        for (w = 0; w < item->num_warnings; w++) {
            if (w != 0)
                fprintf(file, ", ");
            guacclip_json_string(file, item->warnings[w]);
        }
        fprintf(file, "]\n");

        fprintf(file, "    }");

    }

    if (state->num_items > 0)
        fprintf(file, "\n  ]\n");
    else
        fprintf(file, "]\n");
    fprintf(file, "}\n");

    if (fclose(file) != 0) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to flush manifest \"%s\": %s",
                tmp_path, strerror(errno));
        unlink(tmp_path);
        return 1;
    }

    if (rename(tmp_path, path) != 0) {
        guacclip_log(GUAC_LOG_ERROR, "Failed to rename manifest into place: %s",
                strerror(errno));
        unlink(tmp_path);
        return 1;
    }

    guacclip_log(GUAC_LOG_INFO, "Wrote manifest \"%s\" (%d item(s)).",
            path, state->num_items);
    return 0;

}

int guacclip_state_free(guacclip_state* state) {

    int i;

    if (state == NULL)
        return 0;

    /* Finalize any streams still open at end-of-file */
    for (i = 0; i < GUACCLIP_MAX_STREAMS; i++) {
        if (state->streams[i].in_use)
            guacclip_finalize_stream(state, &state->streams[i],
                    "stream-open-at-eof");
    }

    /* Write the manifest describing all extracted items */
    int result = guacclip_write_manifest(state);

    /* Free all manifest items */
    for (i = 0; i < state->num_items; i++) {
        guacclip_item* item = &state->items[i];
        free(item->direction);
        free(item->mimetype);
        free(item->filename);
        int w;
        for (w = 0; w < item->num_warnings; w++)
            free(item->warnings[w]);
    }
    free(state->items);

    /* Free any remaining pending direction annotations */
    for (i = 0; i < GUACCLIP_MAX_PENDING; i++)
        free(state->pending[i].direction);

    free(state);
    return result;

}
