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

#include "mongodb.h"

#include <dbshell/client.h>
#include <dbshell/settings.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/user.h>

#include <stddef.h>

/**
 * The TCP port used by MongoDB servers, if no other port is specified.
 */
#define GUAC_MONGODB_DEFAULT_PORT 27017

/**
 * All arguments accepted by the MongoDB protocol plugin: the arguments
 * common to all database shells, followed by the MongoDB-specific
 * arguments.
 */
static const char* GUAC_MONGODB_CLIENT_ARGS[] = {
    GUAC_DBSHELL_COMMON_ARGS,
    "auth-database",
    "use-ssl",
    "ssl-ca-file",
    NULL
};

/**
 * The indices of the MongoDB-specific arguments, following the common
 * database shell arguments.
 */
enum GUAC_MONGODB_ARGS_IDX {

    /**
     * The name of the database to authenticate against (authSource). If
     * omitted, authentication is performed against the connected
     * database.
     */
    IDX_AUTH_DATABASE = GUAC_DBSHELL_COMMON_ARG_COUNT,

    /**
     * Whether TLS should be used for the connection. If omitted, TLS is
     * not used.
     */
    IDX_USE_SSL,

    /**
     * The path, on the server hosting guacd, to the certificate authority
     * file to verify the database server certificate against. Optional.
     */
    IDX_SSL_CA_FILE,

    GUAC_MONGODB_ARGS_COUNT

} ;

/**
 * Parses the MongoDB-specific arguments into a newly-allocated
 * guac_mongodb_extra_settings structure. This implements
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
 *     A newly-allocated guac_mongodb_extra_settings structure.
 */
static void* guac_mongodb_parse_extra_args(guac_user* user, int argc,
        const char** argv) {

    guac_mongodb_extra_settings* extra =
        guac_mem_zalloc(sizeof(guac_mongodb_extra_settings));

    extra->auth_database =
        guac_user_parse_args_string(user, GUAC_MONGODB_CLIENT_ARGS, argv,
                IDX_AUTH_DATABASE, NULL);

    extra->use_ssl =
        guac_user_parse_args_boolean(user, GUAC_MONGODB_CLIENT_ARGS, argv,
                IDX_USE_SSL, false);

    extra->ssl_ca_file =
        guac_user_parse_args_string(user, GUAC_MONGODB_CLIENT_ARGS, argv,
                IDX_SSL_CA_FILE, NULL);

    return extra;

}

/**
 * Frees the given guac_mongodb_extra_settings structure. This implements
 * guac_dbshell_plugin.free_extra_args.
 *
 * @param data
 *     The structure to free.
 */
static void guac_mongodb_free_extra_args(void* data) {

    guac_mongodb_extra_settings* extra =
        (guac_mongodb_extra_settings*) data;
    if (extra == NULL)
        return;

    guac_mem_free(extra->auth_database);
    guac_mem_free(extra->ssl_ca_file);
    guac_mem_free(extra);

}

/**
 * The plugin definition of the MongoDB protocol.
 */
static const guac_dbshell_plugin guac_mongodb_plugin = {
    .driver = &guac_mongodb_driver,
    .args = GUAC_MONGODB_CLIENT_ARGS,
    .argc = GUAC_MONGODB_ARGS_COUNT,
    .default_port = GUAC_MONGODB_DEFAULT_PORT,
    .parse_extra_args = guac_mongodb_parse_extra_args,
    .free_extra_args = guac_mongodb_free_extra_args
};

int guac_client_init(guac_client* client) {
    return guac_dbshell_client_init(client, &guac_mongodb_plugin);
}
