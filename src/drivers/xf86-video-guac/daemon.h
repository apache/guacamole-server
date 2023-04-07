
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

#ifndef _GUAC_DRV_DAEMON_H
#define _GUAC_DRV_DAEMON_H

/**
 * The time to allow between sync responses in milliseconds. If a sync
 * instruction is sent to the client and no response is received within this
 * timeframe, server messages will not be handled until a sync instruction is
 * received from the client.
 */
#define GUACD_SYNC_THRESHOLD 500

/**
 * The time to allow between server sync messages in milliseconds. A sync
 * message from the server will be sent every GUACD_SYNC_FREQUENCY milliseconds.
 * As this will induce a response from a client that is not malfunctioning,
 * this is used to detect when a client has died. This must be set to a
 * reasonable value to avoid clients being disconnected unnecessarily due
 * to timeout.
 */
#define GUACD_SYNC_FREQUENCY 5000

/**
 * The number of milliseconds to wait for messages in any phase before
 * timing out and closing the connection with an error.
 */
#define GUACD_TIMEOUT      15000

/**
 * The number of microseconds to wait for messages in any phase before
 * timing out and closing the conncetion with an error. This is always
 * equal to GUACD_TIMEOUT * 1000.
 */
#define GUACD_USEC_TIMEOUT (GUACD_TIMEOUT*1000)

/**
 * Thread which listens for connections, assigning each an associated
 * guac_client. This thread function takes a ScreenPtr as an argument.
 */
void* guac_drv_listen_thread(void* arg);

#endif

