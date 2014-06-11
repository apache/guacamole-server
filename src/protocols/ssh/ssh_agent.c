/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "ssh_agent.h"
#include "ssh_buffer.h"

#include <guacamole/client.h>
#include <libssh2.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void ssh_auth_agent_sign(ssh_auth_agent* agent, char* data, int data_length) {

    LIBSSH2_CHANNEL* channel = agent->channel;
    ssh_key* key = agent->identity;

    char sig[4096];
    int sig_len;

    char buffer[4096];
    char* pos = buffer;

    /* Sign with key */
    sig_len = ssh_key_sign(key, data, data_length, (u_char*) sig);
    if (sig_len < 0)
        return;

    buffer_write_uint32(&pos, 1+4 + 4+7 + 4+sig_len);

    buffer_write_byte(&pos, SSH2_AGENT_SIGN_RESPONSE);
    buffer_write_uint32(&pos, 4+7 + 4+sig_len);

    /* Write key type */
    if (key->type == SSH_KEY_RSA)
        buffer_write_string(&pos, "ssh-rsa", 7);
    else if (key->type == SSH_KEY_DSA)
        buffer_write_string(&pos, "ssh-dss", 7);
    else
        return;

    /* Write signature */
    buffer_write_string(&pos, sig, sig_len);

    libssh2_channel_write(channel, buffer, pos-buffer);
    libssh2_channel_flush(channel);

}

void ssh_auth_agent_list_identities(ssh_auth_agent* auth_agent) {

    LIBSSH2_CHANNEL* channel = auth_agent->channel;
    ssh_key* key = auth_agent->identity;

    char buffer[4096];
    char* pos = buffer;

    buffer_write_uint32(&pos, 1+4
                              + key->public_key_length+4
                              + sizeof(SSH_AGENT_COMMENT)+3);

    buffer_write_byte(&pos, SSH2_AGENT_IDENTITIES_ANSWER);
    buffer_write_uint32(&pos, 1);

    buffer_write_string(&pos, key->public_key, key->public_key_length);
    buffer_write_string(&pos, SSH_AGENT_COMMENT, sizeof(SSH_AGENT_COMMENT)-1);

    libssh2_channel_write(channel, buffer, pos-buffer);
    libssh2_channel_flush(channel);

}

void ssh_auth_agent_handle_packet(ssh_auth_agent* auth_agent, uint8_t type,
        char* data, int data_length) {

    switch (type) {

        /* List identities */
        case SSH2_AGENT_REQUEST_IDENTITIES:
            ssh_auth_agent_list_identities(auth_agent);
            break;

        /* Sign request */
        case SSH2_AGENT_SIGN_REQUEST: {

            char* pos = data;

            int key_blob_length;
            int sign_data_length;
            char* sign_data;

            /* Skip past key, read data, ignore flags */
            buffer_read_string(&pos, &key_blob_length);
            sign_data = buffer_read_string(&pos, &sign_data_length);

            /* Sign given data */
            ssh_auth_agent_sign(auth_agent, sign_data, sign_data_length);
            break;
        }

        /* Otherwise, return failure */
        default:
            libssh2_channel_write(auth_agent->channel,
                    UNSUPPORTED, sizeof(UNSUPPORTED)-1);

    }

}

int ssh_auth_agent_read(ssh_auth_agent* auth_agent) {

    LIBSSH2_CHANNEL* channel = auth_agent->channel;

    int bytes_read;

    if (libssh2_channel_eof(channel))
        return -1;

    /* Read header if available */
    if (auth_agent->buffer_length >= 5) {

        uint32_t length =
              (((unsigned char*) auth_agent->buffer)[0] << 24)
            | (((unsigned char*) auth_agent->buffer)[1] << 16)
            | (((unsigned char*) auth_agent->buffer)[2] <<  8)
            |  ((unsigned char*) auth_agent->buffer)[3];

        uint8_t type = ((unsigned char*) auth_agent->buffer)[4];

        /* If enough data read, call handler, shift data */
        if (auth_agent->buffer_length >= length+4) {

            ssh_auth_agent_handle_packet(auth_agent, type,
                    auth_agent->buffer+5, length-1);

            auth_agent->buffer_length -= length+4;
            memmove(auth_agent->buffer,
                    auth_agent->buffer+length+4, auth_agent->buffer_length);
            return length+4;
        }

    }

    /* Read data into buffer */
    bytes_read = libssh2_channel_read(channel,
            auth_agent->buffer+auth_agent->buffer_length,
            sizeof(auth_agent->buffer)-auth_agent->buffer_length);

    /* If unsuccessful, return error */
    if (bytes_read < 0)
        return bytes_read;

    /* Update buffer length */
    auth_agent->buffer_length += bytes_read;
    return bytes_read;

}

void ssh_auth_agent_callback(LIBSSH2_SESSION *session,
        LIBSSH2_CHANNEL *channel, void **abstract) {

    /* Get client data */
    guac_client* client = (guac_client*) *abstract;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    /* Init auth agent */
    ssh_auth_agent* auth_agent = malloc(sizeof(ssh_auth_agent));
    auth_agent->channel = channel;
    auth_agent->identity = client_data->key;
    auth_agent->buffer_length = 0;

    /* Store auth agent */
    client_data->auth_agent = auth_agent;

}

