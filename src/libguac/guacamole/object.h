/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_OBJECT_H
#define GUAC_OBJECT_H

/**
 * Provides functions and structures required for allocating and using objects.
 *
 * @file object.h
 */

#include "object-types.h"
#include "user-fntypes.h"

struct guac_object {

    /**
     * The index of this object.
     */
    int index;

    /**
     * Arbitrary data associated with this object.
     */
    void* data;

    /**
     * Handler for get events sent by the Guacamole web-client.
     *
     * The handler takes a guac_object, containing the object index which will
     * persist through the duration of the transfer, and the name of the stream
     * being requested. It is up to the get handler to create the required body
     * stream.
     *
     * Example:
     * @code
     *     int get_handler(guac_user* user, guac_object* object,
     *             char* name);
     *
     *     int some_function(guac_user* user) {
     *
     *         guac_object* object = guac_user_alloc_object(user);
     *         object->get_handler = get_handler;
     *
     *     }
     * @endcode
     */
    guac_user_get_handler* get_handler;

    /**
     * Handler for put events sent by the Guacamole web-client.
     *
     * The handler takes a guac_object and guac_stream, which each contain their
     * respective indices which will persist through the duration of the
     * transfer, the mimetype of the data being transferred, and the name of
     * the stream within the object being written to.
     *
     * Example:
     * @code
     *     int put_handler(guac_user* user, guac_object* object,
     *             guac_stream* stream, char* mimetype, char* name);
     *
     *     int some_function(guac_user* user) {
     *
     *         guac_object* object = guac_user_alloc_object(user);
     *         object->put_handler = put_handler;
     *
     *     }
     * @endcode
     */
    guac_user_put_handler* put_handler;

};

#endif

