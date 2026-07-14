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

#ifndef GUAC_MYSQL_DRIVER_H
#define GUAC_MYSQL_DRIVER_H

/**
 * Declarations for the MySQL database shell driver, which communicates
 * with MySQL and MariaDB servers via MariaDB Connector/C.
 *
 * @file mysql-driver.h
 */

#include "config.h"

#include <dbshell/dbshell.h>
#include <dbshell/driver.h>

#include <mysql.h>

#include <stdbool.h>

/**
 * The database-specific settings of a MySQL connection, parsed from the
 * arguments following the common database shell arguments.
 */
typedef struct guac_mysql_extra_settings {

    /**
     * The SSL/TLS behavior requested for the connection, one of
     * "disabled", "preferred", "required", "verify-ca", or
     * "verify-identity", or NULL if not specified.
     */
    char* ssl_mode;

    /**
     * The path, on the server hosting guacd, to the certificate authority
     * file to verify the database server certificate against, or NULL if
     * not specified.
     */
    char* ssl_ca_file;

} guac_mysql_extra_settings;

/**
 * The per-connection data of the MySQL driver.
 */
typedef struct guac_mysql_data {

    /**
     * The connection to the MySQL server.
     */
    MYSQL* mysql;

    /**
     * The server-side thread ID of the connection, as used by KILL QUERY
     * to interrupt a running statement.
     */
    unsigned long thread_id;

} guac_mysql_data;

/**
 * The database shell driver implementing the MySQL protocol.
 */
extern const guac_dbshell_driver guac_mysql_driver;

#endif
