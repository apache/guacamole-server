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

#include "config.h"

#include "conf-file.h"
#include "conf-parse.h"

#include <guacamole/client.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * Updates the configuration with the given parameter/value pair, flagging
 * errors as necessary.
 */
static int guacd_conf_callback(const char* section, const char* param, const char* value, void* data) {

    guacd_config* config = (guacd_config*) data;

    /* Network server options */
    if (strcmp(section, "server") == 0) {

        /* Bind host */
        if (strcmp(param, "bind_host") == 0) {
            free(config->bind_host);
            config->bind_host = strdup(value);
            return 0;
        }

        /* Bind port */
        else if (strcmp(param, "bind_port") == 0) {
            free(config->bind_port);
            config->bind_port = strdup(value);
            return 0;
        }

    }

    /* Options related to daemon startup */
    else if (strcmp(section, "daemon") == 0) {

        /* PID file */
        if (strcmp(param, "pid_file") == 0) {
            free(config->pidfile);
            config->pidfile = strdup(value);
            return 0;
        }

        /* Max log level */
        else if (strcmp(param, "log_level") == 0) {

            /* Translate level name */
            if (strcmp(value, "info") == 0)
                config->max_log_level = GUAC_LOG_INFO;
            else if (strcmp(value, "error") == 0)
                config->max_log_level = GUAC_LOG_ERROR;
            else if (strcmp(value, "warning") == 0)
                config->max_log_level = GUAC_LOG_WARNING;
            else if (strcmp(value, "debug") == 0)
                config->max_log_level = GUAC_LOG_DEBUG;

            /* Invalid log level */
            else {
                guacd_conf_parse_error = "Invalid log level. Valid levels are: \"debug\", \"info\", \"warning\", and \"error\".";
                return 1;
            }

            /* Valid log level */
            return 0;

        }

    }

    /* SSL-specific options */
    else if (strcmp(section, "ssl") == 0) {
#ifdef ENABLE_SSL
        /* SSL certificate */
        if (strcmp(param, "server_certificate") == 0) {
            free(config->cert_file);
            config->cert_file = strdup(value);
            return 0;
        }

        /* SSL key */
        else if (strcmp(param, "server_key") == 0) {
            free(config->key_file);
            config->key_file = strdup(value);
            return 0;
        }
#else
        guacd_conf_parse_error = "SSL support not compiled in";
        return 1;
#endif

    }

    /* If still unhandled, the parameter/section is invalid */
    guacd_conf_parse_error = "Invalid parameter or section name";
    return 1;

}

int guacd_conf_parse_file(guacd_config* conf, int fd) {

    int chars_read;

    char buffer[8192];
    int length = 0;

    int line = 1;
    char* line_start = buffer;
    int parsed = 0;

    /* Attempt to fill remaining space in buffer */
    while ((chars_read = read(fd, buffer + length, sizeof(buffer) -  length)) > 0) {

        length += chars_read;

        line_start = buffer;

        /* Attempt to parse entire buffer */
        while ((parsed = guacd_parse_conf(guacd_conf_callback, line_start, length, conf)) > 0) {
            line_start += parsed;
            length -= parsed;
            line++;
        }

        /* Shift contents to front */
        memmove(buffer, line_start, length);

    }

    /* Handle parse errors */
    if (parsed < 0) {
        int column = guacd_conf_parse_error_location - line_start + 1;
        fprintf(stderr, "Parse error at line %i, column %i: %s.\n",
                line, column, guacd_conf_parse_error);
        return 1;
    }

    /* Check for error conditions */
    if (chars_read < 0) {
        fprintf(stderr, "Error reading configuration: %s\n", strerror(errno));
        return 1;
    }

    /* Read successfully */
    return 0;

}

guacd_config* guacd_conf_load() {

    guacd_config* conf = malloc(sizeof(guacd_config));
    if (conf == NULL)
        return NULL;

    /* Load defaults */
    conf->bind_host = NULL;
    conf->bind_port = strdup("4822");
    conf->pidfile = NULL;
    conf->foreground = 0;
    conf->max_log_level = GUAC_LOG_INFO;

#ifdef ENABLE_SSL
    conf->cert_file = NULL;
    conf->key_file = NULL;
#endif

    /* Read configuration from file */
    int fd = open(GUACD_CONF_FILE, O_RDONLY);
    if (fd > 0) {

        int retval = guacd_conf_parse_file(conf, fd);
        close(fd);

        if (retval != 0) {
            fprintf(stderr, "Unable to parse \"" GUACD_CONF_FILE "\".\n");
            return NULL;
        }

    }

    /* Notify of errors preventing reading */
    else if (errno != ENOENT) {
        fprintf(stderr, "Unable to open \"" GUACD_CONF_FILE "\": %s\n", strerror(errno));
        return NULL;
    }

    return conf;

}

