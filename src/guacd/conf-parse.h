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

#ifndef _GUACD_CONF_PARSE_H
#define _GUACD_CONF_PARSE_H

/**
 * The maximum length of a name, in characters.
 */
#define GUACD_CONF_MAX_NAME_LENGTH 255

/**
 * The maximum length of a value, in characters.
 */
#define GUACD_CONF_MAX_VALUE_LENGTH 8191 

/**
 * A callback function which is provided to guacd_parse_conf() and is called
 * for each parameter/value pair set. The current section is always given. This
 * function will not be called for parameters outside of sections, which are
 * illegal.
 */
typedef int guacd_param_callback(const char* section, const char* param, const char* value, void* data);

/**
 * Parses an arbitrary buffer of configuration file data, calling the given
 * callback for each valid parameter/value pair. Upon success, the number of
 * characters parsed is returned. On failure, a negative value is returned, and
 * guacd_conf_parse_error and guacd_conf_parse_error_location are set. The
 * provided data will be passed to the callback for each invocation.
 */
int guacd_parse_conf(guacd_param_callback* callback, char* buffer, int length, void* data);

/**
 * Human-readable description of the current error, if any.
 */
extern char* guacd_conf_parse_error;

/**
 * The location of the most recent parse error. This will be a pointer to the
 * location of the error within the buffer passed to guacd_parse_conf().
 */
extern char* guacd_conf_parse_error_location;

#endif

