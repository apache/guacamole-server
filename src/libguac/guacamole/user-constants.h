/*
 * Copyright (C) 2014 Glyptodon LLC
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

#ifndef _GUAC_USER_CONSTANTS_H
#define _GUAC_USER_CONSTANTS_H

/**
 * Constants related to the Guacamole user structure, guac_user.
 *
 * @file user-constants.h
 */

/**
 * The character prefix which identifies a user ID.
 */
#define GUAC_USER_ID_PREFIX '@'

/**
 * The maximum number of inbound or outbound streams supported by any one
 * guac_user.
 */
#define GUAC_USER_MAX_STREAMS 64

/**
 * The index of a closed stream.
 */
#define GUAC_USER_CLOSED_STREAM_INDEX -1

/**
 * The maximum number of objects supported by any one guac_client.
 */
#define GUAC_USER_MAX_OBJECTS 64

/**
 * The index of an object which has not been defined.
 */
#define GUAC_USER_UNDEFINED_OBJECT_INDEX -1

/**
 * The stream name reserved for the root of a Guacamole protocol object.
 */
#define GUAC_USER_OBJECT_ROOT_NAME "/"

/**
 * The mimetype of a stream containing a map of available stream names to their
 * corresponding mimetypes. The root of a Guacamole protocol object is
 * guaranteed to have this type.
 */
#define GUAC_USER_STREAM_INDEX_MIMETYPE "application/vnd.glyptodon.guacamole.stream-index+json"

#endif

