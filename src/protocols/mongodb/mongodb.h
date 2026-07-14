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

#ifndef GUAC_MONGODB_H
#define GUAC_MONGODB_H

/**
 * Declarations for the MongoDB database shell driver, which communicates
 * with MongoDB servers via the MongoDB C Driver (libmongoc). The shell
 * accepts MongoDB database commands as Extended JSON documents, one
 * command per statement.
 *
 * @file mongodb.h
 */

#include "config.h"

#include <dbshell/dbshell.h>
#include <dbshell/driver.h>

#include <mongoc/mongoc.h>

/**
 * The database-specific settings of a MongoDB connection, parsed from the
 * arguments following the common database shell arguments.
 */
typedef struct guac_mongodb_extra_settings {

    /**
     * The name of the database to authenticate against (authSource), or
     * NULL to authenticate against the connected database.
     */
    char* auth_database;

    /**
     * Whether TLS should be used for the connection.
     */
    bool use_ssl;

    /**
     * The path, on the server hosting guacd, to the certificate authority
     * file to verify the database server certificate against, or NULL if
     * not specified.
     */
    char* ssl_ca_file;

} guac_mongodb_extra_settings;

/**
 * The per-connection data of the MongoDB driver.
 */
typedef struct guac_mongodb_data {

    /**
     * The MongoDB client handle.
     */
    mongoc_client_t* client;

    /**
     * The name of the database commands are currently directed to, as
     * changed by the \use meta-command.
     */
    char* database;

} guac_mongodb_data;

/**
 * The database shell driver implementing the MongoDB protocol.
 */
extern const guac_dbshell_driver guac_mongodb_driver;

#endif
