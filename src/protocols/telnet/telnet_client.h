/*
 * Copyright (C) 2013 Glyptodon LLC
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

#ifndef GUAC_TELNET__TELNET_CLIENT_H
#define GUAC_TELNET__TELNET_CLIENT_H

#include "config.h"

#include <libtelnet.h>

#include <stdint.h>

/**
 * Main telnet client thread, handling transfer of telnet output to STDOUT.
 */
void* guac_telnet_client_thread(void* data);

/**
 * Send a telnet NAWS message indicating the given terminal window dimensions
 * in characters.
 */
void guac_telnet_send_naws(telnet_t* telnet, uint16_t width, uint16_t height);

/**
 * Sends the given username by setting the remote USER environment variable
 * using the telnet NEW-ENVIRON option.
 */
void guac_telnet_send_user(telnet_t* telnet, const char* username);

#endif

