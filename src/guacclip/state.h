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

#ifndef GUACCLIP_STATE_H
#define GUACCLIP_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * The maximum number of clipboard streams which may be simultaneously open at
 * any one time before additional streams are ignored.
 */
#define GUACCLIP_MAX_STREAMS 256

/**
 * The maximum number of buffered direction annotations (from "log"
 * instructions) which may be pending at any one time.
 */
#define GUACCLIP_MAX_PENDING 256

/**
 * The maximum number of warnings which may be recorded for any single
 * extracted clipboard item.
 */
#define GUACCLIP_MAX_WARNINGS 8

/**
 * The maximum number of manifest items which may be recorded for a single
 * recording. This serves as a safety cap against unbounded memory growth
 * when processing a hostile or malformed recording containing an extremely
 * large number of clipboard open/end pairs. Once this limit is reached,
 * further clipboard items are dropped (with a single warning logged) rather
 * than being added to the manifest.
 */
#define GUACCLIP_MAX_ITEMS 100000

/**
 * Which categories of clipboard content should be extracted.
 */
typedef enum guacclip_include_mode {

    /**
     * Extract only image clipboard content (image mimetypes).
     */
    GUACCLIP_INCLUDE_IMAGE,

    /**
     * Extract only textual clipboard content (text mimetypes).
     */
    GUACCLIP_INCLUDE_TEXT,

    /**
     * Extract all clipboard content regardless of mimetype.
     */
    GUACCLIP_INCLUDE_ALL

} guacclip_include_mode;

/**
 * The options controlling how a recording is processed. These are populated
 * from the command line and remain constant for the lifetime of interpreting
 * a single recording.
 */
typedef struct guacclip_options {

    /**
     * The directory into which extracted items and the manifest should be
     * written. This directory (and its "items" subdirectory) will be created
     * if it does not already exist.
     */
    const char* outdir;

    /**
     * The only transfer direction to extract, or NULL to extract all
     * directions. When non-NULL, this is either "guest-to-client" or
     * "client-to-guest".
     */
    const char* direction_filter;

    /**
     * Which categories of clipboard content should be extracted.
     */
    guacclip_include_mode include;

    /**
     * The maximum number of bytes to accumulate for any single clipboard item,
     * or zero if the caller has explicitly opted out of any limit (via
     * "--max-item-bytes 0"). Items which exceed this limit are marked
     * oversized and are not written to disk. Callers which have not
     * explicitly specified a limit should use GUACCLIP_DEFAULT_MAX_ITEM_BYTES
     * rather than zero, to avoid unbounded memory growth against hostile
     * input.
     */
    size_t max_item_bytes;

} guacclip_options;

/**
 * A single in-progress clipboard stream being reassembled from "blob"
 * instructions.
 */
typedef struct guacclip_stream {

    /**
     * Whether this stream slot is currently in use.
     */
    bool in_use;

    /**
     * The Guacamole stream index which opened this clipboard stream.
     */
    int index;

    /**
     * The mimetype of the clipboard content, as declared by the "clipboard"
     * instruction. This string is owned by the stream and freed when the
     * stream is released.
     */
    char* mimetype;

    /**
     * The transfer direction of this clipboard content ("guest-to-client",
     * "client-to-guest", or "unknown"). This string is owned by the stream and
     * freed when the stream is released.
     */
    char* direction;

    /**
     * The sequence number assigned to this stream when it was opened.
     */
    int sequence;

    /**
     * The absolute sync timestamp (in milliseconds) most recently seen at the
     * time this stream was opened.
     */
    int64_t sync_timestamp;

    /**
     * The offset, in milliseconds, of this stream relative to the first sync
     * timestamp seen in the recording.
     */
    int64_t offset_ms;

    /**
     * The expected size of the clipboard content in bytes, as annotated by the
     * recording's "log" instruction, or -1 if unknown.
     */
    int64_t expected_bytes;

    /**
     * Whether this stream is being tracked purely to correctly consume its
     * "blob"/"end" instructions, but its content should not be extracted (due
     * to direction or include filtering).
     */
    bool skipped;

    /**
     * Whether this stream has exceeded the configured maximum item size.
     */
    bool oversized;

    /**
     * Heap buffer accumulating the decoded clipboard bytes.
     */
    char* buffer;

    /**
     * The number of decoded bytes currently stored within the buffer.
     */
    size_t length;

    /**
     * The current allocated capacity of the buffer, in bytes.
     */
    size_t capacity;

} guacclip_stream;

/**
 * A buffered direction annotation parsed from a "log" instruction, awaiting
 * the "clipboard" instruction it describes.
 */
typedef struct guacclip_pending_direction {

    /**
     * Whether this pending-direction slot is currently in use.
     */
    bool in_use;

    /**
     * The stream index this annotation applies to.
     */
    int index;

    /**
     * The transfer direction ("guest-to-client" or "client-to-guest"). Owned
     * by this slot and freed when consumed.
     */
    char* direction;

    /**
     * The annotated content size in bytes, or -1 if the annotation did not
     * include a byte count.
     */
    int64_t expected_bytes;

} guacclip_pending_direction;

/**
 * A completed manifest entry describing a single extracted (or attempted)
 * clipboard item.
 */
typedef struct guacclip_item {

    /**
     * The sequence number of the item.
     */
    int sequence;

    /**
     * The Guacamole stream index the item was extracted from.
     */
    int stream;

    /**
     * The transfer direction of the item. Owned by the item.
     */
    char* direction;

    /**
     * The mimetype of the item. Owned by the item.
     */
    char* mimetype;

    /**
     * The relative path (within the output directory) of the written item
     * file, or NULL if no file was written. Owned by the item.
     */
    char* filename;

    /**
     * The offset of the item in milliseconds relative to the recording start.
     */
    int64_t offset_ms;

    /**
     * The absolute sync timestamp at which the item's stream opened.
     */
    int64_t sync_timestamp;

    /**
     * The number of bytes reassembled for the item.
     */
    size_t bytes;

    /**
     * The expected size of the item in bytes as annotated by the recording, or
     * -1 if unknown.
     */
    int64_t expected_bytes;

    /**
     * The lowercase hexadecimal SHA-256 digest of the item's bytes. Empty if
     * no digest was computed.
     */
    char sha256[65];

    /**
     * Whether the item was completely and consistently reassembled.
     */
    bool complete;

    /**
     * Human-readable warnings associated with the item. Each entry is owned by
     * the item.
     */
    char* warnings[GUACCLIP_MAX_WARNINGS];

    /**
     * The number of warnings recorded for the item.
     */
    int num_warnings;

} guacclip_item;

/**
 * The overall state of the guacclip interpreter while processing a single
 * recording.
 */
typedef struct guacclip_state {

    /**
     * The options controlling extraction.
     */
    const guacclip_options* options;

    /**
     * The path to the recording being interpreted (recorded in the manifest).
     */
    const char* recording_path;

    /**
     * The output directory into which items and the manifest are written.
     */
    const char* outdir;

    /**
     * Whether a sync timestamp has been observed yet.
     */
    bool has_sync;

    /**
     * The first sync timestamp observed within the recording.
     */
    int64_t first_sync_timestamp;

    /**
     * The most recent sync timestamp observed within the recording.
     */
    int64_t last_sync_timestamp;

    /**
     * The next sequence number to assign to an opened clipboard stream.
     */
    int next_sequence;

    /**
     * Active clipboard streams being reassembled.
     */
    guacclip_stream streams[GUACCLIP_MAX_STREAMS];

    /**
     * Buffered direction annotations awaiting their "clipboard" instruction.
     */
    guacclip_pending_direction pending[GUACCLIP_MAX_PENDING];

    /**
     * Dynamic array of completed manifest items.
     */
    guacclip_item* items;

    /**
     * The number of completed manifest items.
     */
    int num_items;

    /**
     * The allocated capacity of the items array.
     */
    int items_capacity;

    /**
     * Whether the GUACCLIP_MAX_ITEMS warning has already been logged. Used to
     * ensure the warning is only logged once per recording, even if many
     * further items are dropped.
     */
    bool item_limit_warned;

} guacclip_state;

/**
 * Allocates and initializes a new guacclip interpreter state for the given
 * recording, creating the output directory and its "items" subdirectory.
 *
 * @param recording_path
 *     The path to the recording being interpreted.
 *
 * @param options
 *     The options controlling extraction. Must remain valid for the lifetime
 *     of the returned state.
 *
 * @return
 *     A newly-allocated guacclip_state, or NULL if allocation or output
 *     directory creation failed.
 */
guacclip_state* guacclip_state_alloc(const char* recording_path,
        const guacclip_options* options);

/**
 * Frees all memory associated with the given guacclip interpreter state after
 * finalizing any streams left open and writing the manifest. If the given
 * state is NULL, this function has no effect.
 *
 * @param state
 *     The guacclip interpreter state to free, which may be NULL.
 *
 * @return
 *     Zero if finalization (including manifest writing) succeeded, non-zero
 *     otherwise.
 */
int guacclip_state_free(guacclip_state* state);

/**
 * Records the given sync timestamp within the interpreter state.
 *
 * @param state
 *     The interpreter state to update.
 *
 * @param timestamp
 *     The millisecond sync timestamp.
 */
void guacclip_state_sync(guacclip_state* state, int64_t timestamp);

/**
 * Buffers a direction annotation (parsed from a "log" instruction) for the
 * given stream index, to be consumed when that clipboard stream opens.
 *
 * @param state
 *     The interpreter state to update.
 *
 * @param index
 *     The stream index the annotation applies to.
 *
 * @param direction
 *     The transfer direction ("guest-to-client" or "client-to-guest").
 *
 * @param expected_bytes
 *     The annotated content size in bytes, or -1 if not present.
 */
void guacclip_state_buffer_direction(guacclip_state* state, int index,
        const char* direction, int64_t expected_bytes);

/**
 * Opens a new clipboard stream on the given stream index with the given
 * mimetype, consuming any buffered direction annotation.
 *
 * @param state
 *     The interpreter state to update.
 *
 * @param index
 *     The stream index being opened.
 *
 * @param mimetype
 *     The mimetype of the clipboard content.
 *
 * @return
 *     Zero on success, non-zero if the stream could not be opened.
 */
int guacclip_state_open(guacclip_state* state, int index,
        const char* mimetype);

/**
 * Appends the given decoded bytes to the open clipboard stream having the given
 * stream index. If no clipboard stream is open on that index, this is a no-op
 * (the blob belongs to some other, non-clipboard stream).
 *
 * @param state
 *     The interpreter state to update.
 *
 * @param index
 *     The stream index the blob belongs to.
 *
 * @param data
 *     The decoded bytes to append.
 *
 * @param length
 *     The number of bytes to append.
 */
void guacclip_state_append(guacclip_state* state, int index,
        const char* data, size_t length);

/**
 * Ends the clipboard stream having the given stream index, writing its
 * extracted item to disk (if applicable) and appending a manifest entry. If no
 * clipboard stream is open on that index, this is a no-op.
 *
 * @param state
 *     The interpreter state to update.
 *
 * @param index
 *     The stream index being ended.
 */
void guacclip_state_end(guacclip_state* state, int index);

#endif
