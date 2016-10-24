
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

#ifndef __GUAC_DRV_H
#define __GUAC_DRV_H

#include "config.h"

#include <xorg-server.h>
#include <xf86.h>

/**
 * The vendor of this driver.
 */
#define GUAC_DRV_VENDOR "Apache Software Foundation"

/**
 * Version number of this driver.
 */
#define GUAC_DRV_VERSION 0x0900

/**
 * Name of this driver.
 */
#define GUAC_DRV_NAME "guac"

/**
 * The amount of video RAM to claim (in kilobytes).
 */
#define GUAC_DRV_VRAM 128*1024

/**
 * The number of milliseconds to wait for messages in any phase before
 * timing out and closing the connection with an error.
 */
#define GUAC_DRV_TIMEOUT 15000

/**
 * The number of microseconds to wait for messages in any phase before
 * timing out and closing the conncetion with an error. This is always
 * equal to GUAC_DRV_TIMEOUT * 1000.
 */
#define GUAC_DRV_USEC_TIMEOUT (GUAC_DRV_TIMEOUT*1000)

/**
 * The unique indices of all available options for the Guacamole X.Org driver.
 * These indices correspond to the storage locations for options which can be
 * specified within xorg.conf to configure the Guacamole X.Org driver.
 */
enum {

    /**
     * The host or address that the instance of guacd built into the Guacamole
     * X.Org driver should listen on.
     */
    GUAC_DRV_OPTION_LISTEN_ADDRESS,

    /**
     * The port that the instance of guacd built into the Guacamole X.Org
     * driver should listen on.
     */
    GUAC_DRV_OPTION_LISTEN_PORT,

    /**
     * The total number of options defined for the Guacamole X.Org driver.
     */
    GUAC_DRV_OPTION_COUNT,

    /**
     * The total number of elements which must exist in an OptionInfoRec array
     * describing all available options defined for the Guacamole X.Org driver,
     * including terminator.
     */
    GUAC_DRV_OPTIONINFOREC_SIZE

};


/**
 * The unique index of the driver option which specifies the port that the
 * instance of guacd built into the Guacamole X.Org driver should listen on.
 */

/**
 * Temporarily remove the Guacamole wrapper function, restoring the previous
 * state.
 */
#define GUAC_DRV_UNWRAP(current, previous) do { \
    current = previous;                         \
} while (0)

/**
 * Save the previous state, while assigning a new wrapper function specific to
 * Guacamole.
 */
#define GUAC_DRV_WRAP(current, previous, wrapper) do { \
    previous = current;                                \
    current = wrapper;                                 \
} while (0)

/**
 * Entry point for this module.
 */
MODULESETUPPROTO(guac_drv_setup);

/**
 * Logs a message to the Xorg logs identifying this driver.
 */
void guac_drv_identify(int flags);

/**
 * Returns available options for the driver.
 */
const OptionInfoRec* guac_drv_available_options(int chipid, int busid);

/**
 * Finds all screens tied to this driver.
 */
Bool guac_drv_probe(DriverPtr drv, int flags);

/**
 * All available options for the driver.
 */
extern const OptionInfoRec GUAC_OPTIONS[GUAC_DRV_OPTIONINFOREC_SIZE];

#endif

