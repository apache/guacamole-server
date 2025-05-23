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
#include "guacamole/tcp.h"
#include "guacamole/timestamp.h"
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
 * Converts the given IPv4 address and UDP port number into its binary
 * representation (struct sockaddr_in). The size of the structure used will be
 * stored in wol_dest_size.
 *
 * @param addr
 *     The IPv4 address to convert.
 *
 * @param udp_port
 *     The UDP port number to convert and include in the resulting sockaddr_in
 *     structure.
 *
 * @param wol_dest
 *     Storage that is sufficiently large and correctly aligned such that it
 *     may be typecast as a sockaddr_in without issue (this is guaranteed by
 *     the definition of the sockaddr_storage structure).
 *
 * @param wol_dest_size
 *     Pointer to a socklen_t that should receive the size of the sockaddr_in
 *     structure. This value is assigned only if this call succeeds.
 *
 * @return
 *     A positive value if conversion succeeded, zero if conversion failed, and
 *     a negative value if IPv4 is unsupported by this system.
 */
static int __guac_wol_convert_addr_ipv4(const char* addr, const unsigned short udp_port,
        struct sockaddr_storage* wol_dest, socklen_t* wol_dest_size) {

    struct sockaddr_in* wol_dest_ipv4 = (struct sockaddr_in*) wol_dest;
    *wol_dest_size = sizeof(struct sockaddr_in);

    /* Init address structure for IPv4 details */
    memset(wol_dest, 0, *wol_dest_size);
    wol_dest_ipv4->sin_port = htons(udp_port);
    wol_dest_ipv4->sin_family = AF_INET;

    return inet_pton(wol_dest_ipv4->sin_family, addr, &(wol_dest_ipv4->sin_addr));

}

/**
 * Converts the given IPv6 address and UDP port number into its binary
 * representation (struct sockaddr_in6). The size of the structure used will be
 * stored in wol_dest_size.
 *
 * @param addr
 *     The IPv6 address to convert.
 *
 * @param udp_port
 *     The UDP port number to convert and include in the resulting sockaddr_in6
 *     structure.
 *
 * @param wol_dest
 *     Storage that is sufficiently large and correctly aligned such that it
 *     may be typecast as a sockaddr_in6 without issue (this is guaranteed by
 *     the definition of the sockaddr_storage structure).
 *
 * @param wol_dest_size
 *     Pointer to a socklen_t that should receive the size of the sockaddr_in6
 *     structure. This value is assigned only if this call succeeds.
 *
 * @return
 *     A positive value if conversion succeeded, zero if conversion failed, and
 *     a negative value if IPv6 is unsupported by this system.
 */
static int __guac_wol_convert_addr_ipv6(const char* addr, const unsigned short udp_port,
         struct sockaddr_storage* wol_dest, socklen_t* wol_dest_size) {

    struct sockaddr_in6* wol_dest_ipv6 = (struct sockaddr_in6*) wol_dest;
    *wol_dest_size = sizeof(struct sockaddr_in6);

    /* Init address structure for IPv6 details */
    memset(wol_dest, 0, *wol_dest_size);
    wol_dest_ipv6->sin6_family = AF_INET6;
    wol_dest_ipv6->sin6_port = htons(udp_port);

    return inet_pton(wol_dest_ipv6->sin6_family, addr, &(wol_dest_ipv6->sin6_addr));

}

/**
 * Converts the given IPv4 or IPv6 address and UDP port number into its binary
 * representation (either a struct sockaddr_in or struct sockaddr_in6). The
 * size of the structure used will be stored in wol_dest_size. If conversion
 * fails, guac_error and guac_error_message are set appropriately.
 *
 * @param addr
 *     The address to convert.
 *
 * @param udp_port
 *     The UDP port number to convert and include in the resulting structure.
 *
 * @param wol_dest
 *     Storage that is sufficiently large and correctly aligned such that it
 *     may be typecast as a sockaddr_in OR sockaddr_in6 without issue (this is
 *     guaranteed by the definition of the sockaddr_storage structure).
 *
 * @param wol_dest_size
 *     Pointer to a socklen_t that should receive the size of the structure
 *     ultimately used. This value is assigned only if this call succeeds.
 *
 * @return
 *     Non-zero if conversion succeeded, zero otherwise.
 */
static int __guac_wol_convert_addr(const char* addr, const unsigned short udp_port,
          struct sockaddr_storage* wol_dest, socklen_t* wol_dest_size) {

    /* Attempt to resolve as an IPv4 address first */
    int ipv4_retval = __guac_wol_convert_addr_ipv4(addr, udp_port, wol_dest, wol_dest_size);
    if (ipv4_retval > 0)
        return 1;

    /* Failing that, reattempt as IPv6 */
    int ipv6_retval = __guac_wol_convert_addr_ipv6(addr, udp_port, wol_dest, wol_dest_size);
    if (ipv6_retval > 0)
        return 1;

    /*
     * Translate all possible IPv4 / IPv6 failure combinations into a single,
     * human-readable message.
     */

    /* The provided address is not valid IPv4 ... */
    if (ipv4_retval == 0) {

        /* ... and also not valid IPv6. */
        if (ipv6_retval == 0) {
            guac_error = GUAC_STATUS_INVALID_ARGUMENT;
            guac_error_message = "The broadcast or multicast address "
                "specified for Wake-on-LAN is not a valid IPv4 or IPv6 "
                "address";
        }

        /* ... but we can't try IPv6 because this system doesn't support it. */
        else {
            guac_error = GUAC_STATUS_INVALID_ARGUMENT;
            guac_error_message = "IPv6 is not supported by this system and "
                "the broadcast/multicast address specified for Wake-on-LAN is "
                "not a valid IPv4 address";
        }

    }

    /* This system bizarrely doesn't support IPv4 ... */
    else {

        /* ... and the provided address is not valid IPv6. */
        if (ipv6_retval == 0) {
            guac_error = GUAC_STATUS_INVALID_ARGUMENT;
            guac_error_message = "IPv4 is not supported by this system and "
                "the broadcast/multicast address specified for Wake-on-LAN is "
                "not a valid IPv6 address";
        }

        /* ... nor IPv6 (should be impossible). */
        else {
            guac_error = GUAC_STATUS_NOT_SUPPORTED;
            guac_error_message = "Neither IPv4 nor IPv6 is supported by this "
                "system (this should not be possible)";
        }

    }

    return 0;

}

/**
 * Send the magic Wake-on-LAN (WoL) packet to the specified broadcast address,
 * returning the number of bytes sent, or zero if any error occurred and nothing
 * was sent.
 * 
 * @param broadcast_addr
 *     The broadcast address to which to send the magic WoL packet.
 * 
 * @param udp_port
 *     The UDP port to use when sending the WoL packet.
 * 
 * @param packet
 *     The magic WoL packet to send.
 * 
 * @return 
 *     The number of bytes sent, or zero if nothing could be sent.
 */
static ssize_t __guac_wol_send_packet(const char* broadcast_addr,
        const unsigned short udp_port, unsigned char packet[]) {

    struct sockaddr_storage wol_dest;
    socklen_t wol_dest_size;

    /* Resolve broadcast address */
    if (!__guac_wol_convert_addr(broadcast_addr, udp_port, &wol_dest, &wol_dest_size))
        return 0;

    /* Set up the socket */
    int wol_socket = socket(wol_dest.ss_family, SOCK_DGRAM, 0);
    
    /* If socket open fails, bail out. */
    if (wol_socket < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Failed to open socket to send Wake-on-LAN packet";
        return 0;
    }

    /* Set up socket for IPv4 broadcast. */
    if (wol_dest.ss_family == AF_INET) {

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
            (struct sockaddr*) &wol_dest, wol_dest_size);
    close(wol_socket);
    return bytes;
 
}

int guac_wol_wake(const char* mac_addr, const char* broadcast_addr,
        const unsigned short udp_port) {
    
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
    int bytes_sent = __guac_wol_send_packet(broadcast_addr, udp_port, 
            wol_packet);
    
    /* Return 0 if bytes were sent, otherwise return an error. */
    if (bytes_sent)
        return 0;
    
    return -1;
}

int guac_wol_wake_and_wait(const char* mac_addr, const char* broadcast_addr,
        const unsigned short udp_port, int wait_time, int retries,
        const char* hostname, const char* port, const int timeout) {

    /* Attempt to connect, first. */
    int sockfd = guac_tcp_connect(hostname, port, timeout);

    /* If connection succeeds, no need to wake the system. */
    if (sockfd > 0) {
        close(sockfd);
        return 0;
    }

    /* Close the fd to avoid resource leak. */
    close(sockfd);

    /* Send the magic WOL packet and store return value. */
    int retval = guac_wol_wake(mac_addr, broadcast_addr, udp_port);

    /* If sending WOL packet fails, just return the received return value. */
    if (retval)
        return retval;

    /* Try to connect on the specified TCP port and hostname or IP. */
    for (int i = 0; i < retries; i++) {

        sockfd = guac_tcp_connect(hostname, port, timeout);

        /* Connection succeeded - close socket and exit. */
        if (sockfd > 0) {
            close(sockfd);
            return 0;
        }

        /**
         * Connection did not succed - close the socket and sleep for the
         * specified amount of time before retrying.
         */
        close(sockfd);
        guac_timestamp_msleep(wait_time * 1000);
    }

    /* Failed to connect, set error message and return an error. */
    guac_error = GUAC_STATUS_REFUSED;
    guac_error_message = "Unable to connect to remote host.";
    return -1;

}
