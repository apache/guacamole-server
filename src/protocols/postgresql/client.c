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

#include "postgresql.h"

#include <dbshell/client.h>
#include <dbshell/settings.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/user.h>

#include <stddef.h>

/**
 * The TCP port used by PostgreSQL servers, if no other port is specified.
 */
#define GUAC_POSTGRESQL_DEFAULT_PORT 5432

/**
 * All arguments accepted by the PostgreSQL protocol plugin: the arguments
 * common to all database shells, followed by the PostgreSQL-specific TLS
 * arguments.
 */
static const char* GUAC_POSTGRESQL_CLIENT_ARGS[] = {
    GUAC_DBSHELL_COMMON_ARGS,
    "ssl-mode",
    "ssl-ca-file",
    NULL
};

/**
 * The indices of the PostgreSQL-specific arguments, following the common
 * database shell arguments.
 */
enum GUAC_POSTGRESQL_ARGS_IDX {

    /**
     * The libpq sslmode of the connection, one of "disable", "allow",
     * "prefer", "require", "verify-ca", or "verify-full". Optional.
     */
    IDX_SSL_MODE = GUAC_DBSHELL_COMMON_ARG_COUNT,

    /**
     * The path, on the server hosting guacd, to the certificate authority
     * file to verify the database server certificate against. Optional.
     */
    IDX_SSL_CA_FILE,

    GUAC_POSTGRESQL_ARGS_COUNT

} ;

/**
 * Parses the PostgreSQL-specific arguments into a newly-allocated
 * guac_postgresql_extra_settings structure. This implements
 * guac_dbshell_plugin.parse_extra_args.
 *
 * @param user
 *     The user who submitted the given arguments while joining the
 *     connection.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @return
 *     A newly-allocated guac_postgresql_extra_settings structure.
 */
static void* guac_postgresql_parse_extra_args(guac_user* user, int argc,
        const char** argv) {

    guac_postgresql_extra_settings* extra =
        guac_mem_zalloc(sizeof(guac_postgresql_extra_settings));

    extra->ssl_mode =
        guac_user_parse_args_string(user, GUAC_POSTGRESQL_CLIENT_ARGS,
                argv, IDX_SSL_MODE, NULL);

    extra->ssl_ca_file =
        guac_user_parse_args_string(user, GUAC_POSTGRESQL_CLIENT_ARGS,
                argv, IDX_SSL_CA_FILE, NULL);

    return extra;

}

/**
 * Frees the given guac_postgresql_extra_settings structure. This
 * implements guac_dbshell_plugin.free_extra_args.
 *
 * @param data
 *     The structure to free.
 */
static void guac_postgresql_free_extra_args(void* data) {

    guac_postgresql_extra_settings* extra =
        (guac_postgresql_extra_settings*) data;
    if (extra == NULL)
        return;

    guac_mem_free(extra->ssl_mode);
    guac_mem_free(extra->ssl_ca_file);
    guac_mem_free(extra);

}

/**
 * The plugin definition of the PostgreSQL protocol.
 */
static const guac_dbshell_plugin guac_postgresql_plugin = {
    .driver = &guac_postgresql_driver,
    .args = GUAC_POSTGRESQL_CLIENT_ARGS,
    .argc = GUAC_POSTGRESQL_ARGS_COUNT,
    .default_port = GUAC_POSTGRESQL_DEFAULT_PORT,
    .parse_extra_args = guac_postgresql_parse_extra_args,
    .free_extra_args = guac_postgresql_free_extra_args
};

int guac_client_init(guac_client* client) {
    return guac_dbshell_client_init(client, &guac_postgresql_plugin);
}
