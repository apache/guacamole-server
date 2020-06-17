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

#include "guacamole/error.h"
#include "guacamole/wol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
    
    struct sockaddr_in wol_dest;
    int wol_socket;
    
    /* Determine the IP version, starting with IPv4. */
    wol_dest.sin_port = htons(GUAC_WOL_PORT);
    wol_dest.sin_family = AF_INET;
    int retval = inet_pton(wol_dest.sin_family, broadcast_addr, &(wol_dest.sin_addr));
    
    /* If return value is less than zero, this system doesn't know about IPv4. */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "IPv4 address family is not supported";
        return 0;
    }
    
    /* If return value is zero, address doesn't match the IPv4, so try IPv6. */
    else if (retval == 0) {
        wol_dest.sin_family = AF_INET6;
        retval = inet_pton(wol_dest.sin_family, broadcast_addr, &(wol_dest.sin_addr));
        
        /* System does not support IPv6. */
        if (retval < 0) {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "IPv6 address family is not supported";
            return 0;
        }
        
        /* Address didn't match IPv6. */
        else if (retval == 0) {
            guac_error = GUAC_STATUS_INVALID_ARGUMENT;
            guac_error_message = "Invalid broadcast or multicast address specified for Wake-on-LAN";
            return 0;
        }
    }
    
    
    
    /* Set up the socket */
    wol_socket = socket(wol_dest.sin_family, SOCK_DGRAM, 0);
    
    /* If socket open fails, bail out. */
    if (wol_socket < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Failed to open socket to send Wake-on-LAN packet";
        return 0;
    }
    
    /* Set up socket for IPv4 broadcast. */
    if (wol_dest.sin_family == AF_INET) {
        
        /* For configuring socket broadcast */
        int wol_bcast = 1;

        /* Attempt to set IPv4 broadcast; exit with error if this fails. */
        if (setsockopt(wol_socket, SOL_SOCKET, SO_BROADCAST, &wol_bcast,
                sizeof(wol_bcast)) < 0) {
            close(wol_socket);
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "Failed to set IPv4 broadcast for Wake-on-LAN socket";
            return 0;
        }
    }
    
    /* Set up socket for IPv6 multicast. */
    else {
        
        /* Stick to a single hop for now. */
        int hops = 1;
        
        /* Attempt to set IPv6 multicast; exit with error if this fails. */
        if (setsockopt(wol_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops,
                sizeof(hops)) < 0) {
            close(wol_socket);
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "Failed to set IPv6 multicast for Wake-on-LAN socket";
            return 0;
        }
    }
    
    /* Send the packet and return number of bytes sent. */
    int bytes = sendto(wol_socket, packet, GUAC_WOL_PACKET_SIZE, 0,
            (struct sockaddr*) &wol_dest, sizeof(wol_dest));
    close(wol_socket);
    return bytes;
 
}

int guac_wol_wake(const char* mac_addr, const char* broadcast_addr) {
    
    unsigned char wol_packet[GUAC_WOL_PACKET_SIZE];
    unsigned int dest_mac[6];
    
    /* Parse mac address and return with error if parsing fails. */
    if (sscanf(mac_addr, "%x:%x:%x:%x:%x:%x",
            &(dest_mac[0]), &(dest_mac[1]), &(dest_mac[2]),
            &(dest_mac[3]), &(dest_mac[4]), &(dest_mac[5])) != 6) {
        guac_error = GUAC_STATUS_INVALID_ARGUMENT;
        guac_error_message = "Invalid argument for Wake-on-LAN MAC address";
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