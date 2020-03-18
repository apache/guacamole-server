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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "guacamole/wol.h"

/**
 * Generate the magic Wake-on-LAN (WoL) packet for the specified MAC address
 * and place it in the character array.
 * 
 * @param packet
 *     The character array that will contain the generated packet.
 * 
 * @param mac_address
 *     The unsigned int representation of the MAC address to place in the packet.
 */
static void __guac_wol_create_magic_packet(unsigned char packet[],
        unsigned int mac_address[]) {
    
    int i;
    unsigned char mac[6];
    
    /* Concurrently fill the first part of the packet with 0xFF, and copy the
     MAC address from the int array to the char array. */
    for (i = 0; i < 6; i++) {
        packet[i] = 0xFF;
        mac[i] = mac_address[i];
    }
    
    /* Copy the MAC address contents into the char array that is storing
     the rest of the packet. */
    for (i = 1; i <= 16; i++) {
        memcpy(&packet[i * 6], &mac, 6 * sizeof(unsigned char));
    }
    
}

/**
 * Send the magic Wake-on-LAN (WoL) packet to the specified broadcast address,
 * returning the number of bytes sent, or zero if any error occurred and nothing
 * was sent.
 * 
 * @param broadcast_addr
 *     The broadcast address to which to send the magic WoL packet.
 * 
 * @param packet
 *     The magic WoL packet to send.
 * 
 * @return 
 *     The number of bytes sent, or zero if nothing could be sent.
 */
static ssize_t __guac_wol_send_packet(const char* broadcast_addr,
        unsigned char packet[]) {
    
    int wol_bcast = 1;
    struct sockaddr_in wol_dest;
    int wol_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    /* Attempt to set broadcast; exit with error if this fails. */
    if (setsockopt(wol_socket, SOL_SOCKET, SO_BROADCAST, &wol_bcast, sizeof(wol_bcast)) < 0) {
        return 0;
    }
    
    /* Set server endpoint to the broadcast address. */
    wol_dest.sin_family = AF_INET;
    wol_dest.sin_port = htons(9);
    wol_dest.sin_addr.s_addr = inet_addr(broadcast_addr);
    
    /* Send the packet and return number of bytes sent. */
    return sendto(wol_socket, &packet, sizeof(unsigned char) * 102, 0,
            (struct sockaddr*) &wol_dest, sizeof(wol_dest));
 
}

int guac_wol_wake(const char* mac_addr, const char* broadcast_addr) {
    
    unsigned char wol_packet[GUAC_WOL_PACKET_SIZE];
    unsigned int dest_mac[6];
    
    /* Parse mac address and return with error if parsing fails. */
    if (sscanf(mac_addr, "%x:%x:%x:%x:%x:%x",
            &(dest_mac[0]), &(dest_mac[1]), &(dest_mac[2]),
            &(dest_mac[3]), &(dest_mac[4]), &(dest_mac[5])) != 6) {
        return -1;
    }
    
    /* Generate the magic packet. */
    __guac_wol_create_magic_packet(wol_packet, dest_mac);
    
    /* Send the packet and record bytes sent. */
    int bytes_sent = __guac_wol_send_packet(broadcast_addr, wol_packet);
    
    /* Return 0 if bytes were sent, otherwise return an error. */
    if (bytes_sent)
        return 0;
    
    return -1;
}