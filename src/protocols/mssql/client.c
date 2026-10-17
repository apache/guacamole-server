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

#include "mssql.h"

#include <dbshell/client.h>
#include <dbshell/settings.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/user.h>

#include <stddef.h>

/**
 * The TCP port used by SQL Server, if no other port is specified.
 */
#define GUAC_MSSQL_DEFAULT_PORT 1433

/**
 * All arguments accepted by the SQL Server protocol plugin: the arguments
 * common to all database shells, followed by the SQL Server-specific
 * arguments.
 */
static const char* GUAC_MSSQL_CLIENT_ARGS[] = {
    GUAC_DBSHELL_COMMON_ARGS,
    "tds-version",
    NULL
};

/**
 * The indices of the SQL Server-specific arguments, following the common
 * database shell arguments.
 */
enum GUAC_MSSQL_ARGS_IDX {

    /**
     * The TDS protocol version to use, one of "7.1", "7.2", "7.3", or
     * "7.4". If omitted, the version is negotiated automatically.
     */
    IDX_TDS_VERSION = GUAC_DBSHELL_COMMON_ARG_COUNT,

    GUAC_MSSQL_ARGS_COUNT

} ;

/**
 * Parses the SQL Server-specific arguments into a newly-allocated
 * guac_mssql_extra_settings structure. This implements
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
 *     A newly-allocated guac_mssql_extra_settings structure.
 */
static void* guac_mssql_parse_extra_args(guac_user* user, int argc,
        const char** argv) {

    guac_mssql_extra_settings* extra =
        guac_mem_zalloc(sizeof(guac_mssql_extra_settings));

    extra->tds_version =
        guac_user_parse_args_string(user, GUAC_MSSQL_CLIENT_ARGS, argv,
                IDX_TDS_VERSION, NULL);

    return extra;

}

/**
 * Frees the given guac_mssql_extra_settings structure. This implements
 * guac_dbshell_plugin.free_extra_args.
 *
 * @param data
 *     The structure to free.
 */
static void guac_mssql_free_extra_args(void* data) {

    guac_mssql_extra_settings* extra = (guac_mssql_extra_settings*) data;
    if (extra == NULL)
        return;

    guac_mem_free(extra->tds_version);
    guac_mem_free(extra);

}

/**
 * The plugin definition of the SQL Server protocol.
 */
static const guac_dbshell_plugin guac_mssql_plugin = {
    .driver = &guac_mssql_driver,
    .args = GUAC_MSSQL_CLIENT_ARGS,
    .argc = GUAC_MSSQL_ARGS_COUNT,
    .default_port = GUAC_MSSQL_DEFAULT_PORT,
    .parse_extra_args = guac_mssql_parse_extra_args,
    .free_extra_args = guac_mssql_free_extra_args
};

int guac_client_init(guac_client* client) {
    return guac_dbshell_client_init(client, &guac_mssql_plugin);
}
