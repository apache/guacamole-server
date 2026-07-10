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

#ifndef GUAC_IPMI_CHASSIS_H
#define GUAC_IPMI_CHASSIS_H

#include "settings.h"

#include <guacamole/client.h>

#include <stdbool.h>

/**
 * Performs the given chassis power action against the BMC associated with the
 * given client. A short-lived IPMI 2.0 out-of-band session, separate from the
 * Serial-over-LAN session, is opened to issue the command and closed
 * immediately afterward.
 *
 * @param client
 *     The guac_client whose settings describe the BMC to connect to.
 *
 * @param action
 *     The power action to perform. GUAC_IPMI_POWER_NONE is a no-op which
 *     returns success.
 *
 * @return
 *     Zero if the action was issued successfully, non-zero otherwise.
 */
int guac_ipmi_chassis_power(guac_client* client,
        guac_ipmi_power_action action);

/**
 * Applies a boot device override against the BMC associated with the given
 * client, affecting either the next boot only or all subsequent boots.
 *
 * @param client
 *     The guac_client whose settings describe the BMC to connect to.
 *
 * @param device
 *     The boot device to force. GUAC_IPMI_BOOT_NONE is a no-op which returns
 *     success.
 *
 * @param persistent
 *     Whether the override should persist across boots (true) or apply only to
 *     the next boot (false).
 *
 * @return
 *     Zero if the override was applied successfully, non-zero otherwise.
 */
int guac_ipmi_chassis_set_boot_device(guac_client* client,
        guac_ipmi_boot_device device, bool persistent);

/**
 * Retrieves the current chassis power status from the BMC, writing a
 * human-readable, single-line summary into the given buffer.
 *
 * @param client
 *     The guac_client whose settings describe the BMC to connect to.
 *
 * @param buffer
 *     The buffer into which the status summary should be written.
 *
 * @param size
 *     The size of the given buffer, in bytes.
 *
 * @return
 *     Zero if the status was retrieved successfully, non-zero otherwise.
 */
int guac_ipmi_chassis_status(guac_client* client, char* buffer, int size);

/**
 * Activates the chassis identify LED for the given duration, or indefinitely.
 *
 * @param client
 *     The guac_client whose settings describe the BMC to connect to.
 *
 * @param interval
 *     The number of seconds the identify LED should remain on. Ignored if
 *     force is true.
 *
 * @param force
 *     Whether the identify LED should remain on indefinitely, ignoring the
 *     given interval.
 *
 * @return
 *     Zero if the identify command was issued successfully, non-zero
 *     otherwise.
 */
int guac_ipmi_chassis_identify(guac_client* client, int interval, bool force);

/**
 * Reads the most recent entries from the BMC's System Event Log (SEL) and
 * writes them, one per line, into the given buffer as human-readable text.
 *
 * @param client
 *     The guac_client whose settings describe the BMC to connect to.
 *
 * @param buffer
 *     The buffer into which the formatted SEL entries should be written.
 *
 * @param size
 *     The size of the given buffer, in bytes. Output is truncated to fit.
 *
 * @return
 *     Zero if the System Event Log was read successfully, non-zero otherwise.
 */
int guac_ipmi_chassis_sel(guac_client* client, char* buffer, int size);

#endif
