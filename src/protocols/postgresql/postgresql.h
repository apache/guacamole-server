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

#ifndef GUAC_POSTGRESQL_H
#define GUAC_POSTGRESQL_H

/**
 * Declarations for the PostgreSQL database shell driver, which
 * communicates with PostgreSQL servers via libpq.
 *
 * @file postgresql.h
 */

#include "config.h"

#include <dbshell/dbshell.h>
#include <dbshell/driver.h>

#include <libpq-fe.h>

/**
 * The database-specific settings of a PostgreSQL connection, parsed from
 * the arguments following the common database shell arguments.
 */
typedef struct guac_postgresql_extra_settings {

    /**
     * The libpq sslmode of the connection, one of "disable", "allow",
     * "prefer", "require", "verify-ca", or "verify-full", or NULL if not
     * specified.
     */
    char* ssl_mode;

    /**
     * The path, on the server hosting guacd, to the certificate authority
     * file to verify the database server certificate against, or NULL if
     * not specified.
     */
    char* ssl_ca_file;

} guac_postgresql_extra_settings;

/**
 * The per-connection data of the PostgreSQL driver.
 */
typedef struct guac_postgresql_data {

    /**
     * The connection to the PostgreSQL server.
     */
    PGconn* connection;

    /**
     * The cancellation handle of the connection, created when the
     * connection is established and usable from any thread.
     */
    PGcancel* cancel;

} guac_postgresql_data;

/**
 * The database shell driver implementing the PostgreSQL protocol.
 */
extern const guac_dbshell_driver guac_postgresql_driver;

#endif
