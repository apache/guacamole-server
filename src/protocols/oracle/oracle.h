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

#ifndef GUAC_ORACLE_H
#define GUAC_ORACLE_H

/**
 * Declarations for the Oracle Database shell driver, which communicates
 * with Oracle Database servers via the Oracle Call Interface (OCI) of the
 * Oracle Instant Client.
 *
 * @file oracle.h
 */

#include "config.h"

#include <dbshell/dbshell.h>
#include <dbshell/driver.h>

#include <oci.h>

/**
 * The database-specific settings of an Oracle Database connection, parsed
 * from the arguments following the common database shell arguments.
 */
typedef struct guac_oracle_extra_settings {

    /**
     * The service name of the database to connect to, as used within an
     * EZConnect connection string ("//host:port/service_name").
     */
    char* service_name;

} guac_oracle_extra_settings;

/**
 * The per-connection data of the Oracle Database driver.
 */
typedef struct guac_oracle_data {

    /**
     * The OCI environment handle.
     */
    OCIEnv* environment;

    /**
     * The OCI error handle used by the session thread.
     */
    OCIError* error;

    /**
     * The OCI error handle used by cancellation requests, which arrive on
     * a different thread than the session thread.
     */
    OCIError* cancel_error;

    /**
     * The OCI service context of the established connection.
     */
    OCISvcCtx* service;

} guac_oracle_data;

/**
 * The database shell driver implementing the Oracle Database protocol.
 */
extern const guac_dbshell_driver guac_oracle_driver;

#endif
