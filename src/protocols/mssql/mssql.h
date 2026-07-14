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

#ifndef GUAC_MSSQL_H
#define GUAC_MSSQL_H

/**
 * Declarations for the SQL Server database shell driver, which
 * communicates with Microsoft SQL Server (and Sybase) servers via the
 * CT-Library interface of FreeTDS.
 *
 * @file mssql.h
 */

#include "config.h"

#include <dbshell/dbshell.h>
#include <dbshell/driver.h>

#include <ctpublic.h>

/**
 * The database-specific settings of a SQL Server connection, parsed from
 * the arguments following the common database shell arguments.
 */
typedef struct guac_mssql_extra_settings {

    /**
     * The TDS protocol version to use, one of "7.1", "7.2", "7.3", or
     * "7.4", or NULL if the version should be negotiated automatically.
     */
    char* tds_version;

} guac_mssql_extra_settings;

/**
 * The per-connection data of the SQL Server driver.
 */
typedef struct guac_mssql_data {

    /**
     * The CT-Library context.
     */
    CS_CONTEXT* context;

    /**
     * The connection to the SQL Server database.
     */
    CS_CONNECTION* connection;

} guac_mssql_data;

/**
 * The database shell driver implementing the SQL Server protocol.
 */
extern const guac_dbshell_driver guac_mssql_driver;

#endif
