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

#ifndef GUAC_COMMON_JSON_H
#define GUAC_COMMON_JSON_H

#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <stdint.h>

/**
 * The current streaming state of an arbitrary JSON object, consisting of
 * any number of property name/value pairs.
 */
typedef struct guac_common_json_state {

    /**
     * Buffer of partial JSON data. The individual blobs which make up the JSON
     * body of the object being sent over the Guacamole protocol will be
     * built here.
     */
    char buffer[4096];

    /**
     * The number of bytes currently used within the JSON buffer.
     */
    int size;

    /**
     * The number of property name/value pairs written to the JSON object thus
     * far.
     */
    int properties_written;

} guac_common_json_state;

/**
 * Given a stream, the user to which it belongs, and the current stream state
 * of a JSON object, flushes the contents of the JSON buffer to a blob
 * instruction. Note that this will flush the JSON buffer only, and will not
 * necessarily flush the underlying guac_socket of the user.
 *
 * @param user
 *     The user to which the data will be flushed.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob.
 *
 * @param json_state
 *     The state object whose buffer should be flushed.
 */
void guac_common_json_flush(guac_user* user, guac_stream* stream,
        guac_common_json_state* json_state);

/**
 * Given a stream, the user to which it belongs, and the current stream state
 * of a JSON object, writes the contents of the given buffer to the JSON buffer
 * of the stream state, flushing as necessary.
 *
 * @param user
 *     The user to which the data will be flushed as necessary.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob, if
 *     data must be flushed at all.
 *
 * @param json_state
 *     The state object containing the JSON buffer to which the given buffer
 *     should be written.
 *
 * @param buffer
 *     The buffer to write.
 *
 * @param length
 *     The number of bytes in the buffer.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_write(guac_user* user, guac_stream* stream,
        guac_common_json_state* json_state, const char* buffer, int length);

/**
 * Given a stream, the user to which it belongs, and the current stream state
 * of a JSON object state, writes the given string as a proper JSON string,
 * including starting and ending quotes. The contents of the string will be
 * escaped as necessary.
 *
 * @param user
 *     The user to which the data will be flushed as necessary.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob, if
 *     data must be flushed at all.
 *
 * @param json_state
 *     The state object containing the JSON buffer to which the given string
 *     should be written as a JSON name/value pair.
 *
 * @param str
 *     The string to write.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_write_string(guac_user* user,
        guac_stream* stream, guac_common_json_state* json_state,
        const char* str);

/**
 * Given a stream, the user to which it belongs, and the current stream state
 * of a JSON object, writes the given JSON property name/value pair. The
 * name and value will be written as proper JSON strings separated by a colon.
 *
 * @param user
 *     The user to which the data will be flushed as necessary.
 *
 * @param stream
 *     The stream through which the flushed data should be sent as a blob, if
 *     data must be flushed at all.
 *
 * @param json_state
 *     The state object containing the JSON buffer to which the given strings
 *     should be written as a JSON name/value pair.
 *
 * @param name
 *     The name of the property to write.
 *
 * @param value
 *     The value of the property to write.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_write_property(guac_user* user, guac_stream* stream,
        guac_common_json_state* json_state, const char* name,
        const char* value);

/**
 * Given a stream, the user to which it belongs, and the current stream state
 * of a JSON object, initializes the state for writing a new JSON object. Note
 * that although the user and stream must be provided, no instruction or
 * blobs will be written due to any call to this function.
 *
 * @param user
 *     The user associated with the given stream.
 *
 * @param stream
 *     The stream associated with the JSON object being written.
 *
 * @param json_state
 *     The state object to initialize.
 */
/**
 * Details of a single file or directory, for inclusion as one entry of a
 * stream index (the JSON listing sent for a directory of a filesystem
 * object). Only the mimetype is required; each other detail is written only
 * if it is known.
 */
typedef struct guac_common_json_file_details {

    /**
     * The mimetype of the entry. This is GUAC_USER_STREAM_INDEX_MIMETYPE for
     * directories.
     */
    const char* mimetype;

    /**
     * Non-zero if size is known and should be written, zero otherwise.
     */
    int has_size;

    /**
     * The size of the file, in bytes.
     */
    uint64_t size;

    /**
     * Non-zero if mtime is known and should be written, zero otherwise.
     */
    int has_mtime;

    /**
     * The time the file was last modified, in seconds since the UNIX epoch.
     */
    uint64_t mtime;

    /**
     * The permissions of the file in "ls -l" form (for example "-rw-r--r--"),
     * or NULL if unknown or not applicable.
     */
    const char* permissions;

    /**
     * The name (or, if no name is available, the numeric ID) of the user that
     * owns the file, or NULL if unknown or not applicable.
     */
    const char* owner;

    /**
     * The name (or, if no name is available, the numeric ID) of the group
     * that owns the file, or NULL if unknown or not applicable.
     */
    const char* group;

} guac_common_json_file_details;

/**
 * Writes a single stream index entry, mapping the given name to the given
 * file details, into the given JSON object buffer. If the user supports
 * detailed stream index entries (see guac_user_supports_file_details()), the
 * value is written as an object containing the mimetype and every known
 * detail. Otherwise, the value is written as the mimetype string alone,
 * exactly as guac_common_json_write_property() would, so older clients are
 * unaffected. All flushing and blob handling is as for
 * guac_common_json_write_property().
 *
 * @param user
 *     The user to whom the stream index is being sent.
 *
 * @param stream
 *     The stream to which the JSON object is being written.
 *
 * @param json_state
 *     The current state of the JSON output buffer.
 *
 * @param name
 *     The name of the entry (typically its absolute path).
 *
 * @param details
 *     The details of the entry. The mimetype must be non-NULL.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_write_file_details(guac_user* user,
        guac_stream* stream, guac_common_json_state* json_state,
        const char* name, const guac_common_json_file_details* details);

void guac_common_json_begin_object(guac_user* user, guac_stream* stream,
        guac_common_json_state* json_state);

/**
 * Given a stream, the user to which it belongs, and the current stream state
 * of a JSON object, completes writing that JSON object by writing the final
 * terminating brace. This function must only be called following a
 * corresponding call to guac_common_json_begin_object().
 *
 * @param user
 *     The user associated with the given stream.
 *
 * @param stream
 *     The stream associated with the JSON object being written.
 *
 * @param json_state
 *     The state object whose in-progress JSON object should be terminated.
 *
 * @return
 *     Non-zero if at least one blob was written, zero otherwise.
 */
int guac_common_json_end_object(guac_user* user, guac_stream* stream,
        guac_common_json_state* json_state);

#endif

