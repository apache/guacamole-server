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

#ifndef GUAC_WOL_H
#define GUAC_WOL_H

/**
 * Header that provides functions and structures related to Wake-on-LAN
 * support in libguac.
 * 
 * @file wol.h
 */

#include "wol-constants.h"

/**
 * Send the wake-up packet to the specified destination, returning zero if the
 * wake was sent successfully, or non-zero if an error occurs sending the
 * wake packet.  Note that the return value does not specify whether the
 * system actually wakes up successfully, only whether or not the packet
 * is transmitted.
 * 
 * @param mac_addr
 *     The MAC address to place in the magic Wake-on-LAN packet.
 * 
 * @param broadcast_addr
 *     The broadcast address to which to send the magic Wake-on-LAN packet.
 * 
 * @return 
 *     Zero if the packet is successfully sent to the destination; non-zero
 *     if the packet cannot be sent.
 */
int guac_wol_wake(const char* mac_addr, const char* broadcast_addr);

#endif /* GUAC_WOL_H */

