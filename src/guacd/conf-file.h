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

#ifndef _GUACD_CONF_FILE_H
#define _GUACD_CONF_FILE_H

#include "config.h"

/**
 * The contents of a guacd configuration file.
 */
typedef struct guacd_config {

    /**
     * The host to bind on.
     */
    char* bind_host;

    /**
     * The port to bind on.
     */
    char* bind_port;

    /**
     * The file to write the PID in, if any.
     */
    char* pidfile;

    /**
     * Whether guacd should run in the foreground.
     */
    int foreground;

#ifdef ENABLE_SSL
    /**
     * SSL certificate file.
     */
    char* cert_file;

    /**
     * SSL private key file.
     */
    char* key_file;
#endif

} guacd_config;

/**
 * Reads the given file descriptor, parsing its contents into the guacd_config.
 * On success, zero is returned. If parsing fails, non-zero is returned, and an
 * error message is printed to stderr.
 */
int guacd_conf_parse_file(guacd_config* conf, int fd);

/**
 * Loads the configuration from any of several default locations, if found. If
 * parsing fails, NULL is returned, and an error message is printed to stderr.
 */
guacd_config* guacd_conf_load();

#endif

