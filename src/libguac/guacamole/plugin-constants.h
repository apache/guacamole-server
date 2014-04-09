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

#ifndef _GUAC_PLUGIN_CONSTANTS_H
#define _GUAC_PLUGIN_CONSTANTS_H

/**
 * Constants related to client plugins.
 *
 * @file plugin-constants.h
 */

/**
 * String prefix which begins the library filename of all client plugins.
 */
#define GUAC_PROTOCOL_LIBRARY_PREFIX "libguac-client-"

/**
 * String suffix which ends the library filename of all client plugins.
 */
#define GUAC_PROTOCOL_LIBRARY_SUFFIX ".so"

/**
 * The maximum number of characters (COUNTING NULL TERMINATOR) to allow
 * for protocol names within the library filename of client plugins.
 */
#define GUAC_PROTOCOL_NAME_LIMIT 256

/**
 * The maximum number of characters (INCLUDING NULL TERMINATOR) that a
 * character array containing the concatenation of the library prefix,
 * protocol name, and suffix can contain, assuming the protocol name is
 * limited to GUAC_PROTOCOL_NAME_LIMIT characters.
 */
#define GUAC_PROTOCOL_LIBRARY_LIMIT (                                  \
                                                                       \
      sizeof(GUAC_PROTOCOL_LIBRARY_PREFIX) - 1 /* "libguac-client-" */ \
    +        GUAC_PROTOCOL_NAME_LIMIT      - 1 /* [up to 256 chars] */ \
    + sizeof(GUAC_PROTOCOL_LIBRARY_SUFFIX) - 1 /* ".so"             */ \
    + 1                                        /* NULL terminator   */ \
                                                                       \
)

#endif

