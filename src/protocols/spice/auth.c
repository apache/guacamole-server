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

#include "auth.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <spice-client.h>

#include <string.h>

void guac_spice_session_configure(guac_client* client, SpiceSession* session) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_settings* settings = spice_client->settings;

    /* Always configure the target host */
    g_object_set(session, "host", settings->hostname, NULL);

    /* Configure plaintext port, if provided */
    if (settings->port != NULL)
        g_object_set(session, "port", settings->port, NULL);

    /* Connect through a proxy, if one was specified */
    if (settings->proxy != NULL)
        g_object_set(session, "proxy", settings->proxy, NULL);

    /* Configure TLS port and verification policy, if TLS is in use */
    if (settings->tls && settings->tls_port != NULL) {

        /* Certificate-subject matching and public-key pinning are mutually
         * exclusive identity checks (see the verify-policy comment below): only
         * one SPICE_SESSION_VERIFY_* identity flag takes effect, so configuring
         * both would silently drop the stricter public-key pin in favour of the
         * subject check. Rather than honour one and ignore the other, refuse the
         * ambiguous configuration outright so the operator resolves it. */
        if (!settings->ignore_cert && settings->cert_subject != NULL
                && settings->pubkey != NULL) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                    "TLS certificate-subject matching and public-key pinning "
                    "are mutually exclusive; configure only one.");
            return;
        }

        g_object_set(session, "tls-port", settings->tls_port, NULL);

        /* Provide CA certificate for verification, if given */
        if (settings->ca_file != NULL)
            g_object_set(session, "ca-file", settings->ca_file, NULL);

        /* Provide expected certificate subject, if given */
        if (settings->cert_subject != NULL)
            g_object_set(session, "cert-subject", settings->cert_subject, NULL);

        /* Pin the server's expected public key, if given */
        if (!settings->ignore_cert && settings->pubkey != NULL) {
            gsize pubkey_len = 0;
            guchar* pubkey_der = g_base64_decode(settings->pubkey, &pubkey_len);
            GByteArray* pubkey = g_byte_array_new_take(pubkey_der, pubkey_len);
            g_object_set(session, "pubkey", pubkey, NULL);
            g_byte_array_unref(pubkey);
        }

        /* Determine the certificate verification policy. In spice-gtk each
         * SPICE_SESSION_VERIFY_* flag is an identity check layered on top of
         * mandatory CA-chain validation (see openssl_verify() in
         * spice-common/common/ssl_verify.c): the hostname and subject checks
         * both still require the certificate to chain to a trusted CA, and
         * subject is evaluated in preference to hostname (they are mutually
         * exclusive, not additive). A pinned public key (VERIFY_PUBKEY) is the
         * sole exception — it accepts a self-signed certificate whose key
         * matches without a trusted chain. Self-signed certificates (as used by
         * default by QEMU/libvirt) therefore require either a pinned key or
         * verification disabled entirely (ignore_cert). */
        SpiceSessionVerify verify;
        if (settings->ignore_cert)
            /* Explicit opt-in: no verification at all. */
            verify = 0;
        else if (settings->cert_subject != NULL)
            /* Match the full certificate subject DN on top of CA-chain
             * validation. spice-gtk checks the subject in preference to the
             * hostname, so also requesting VERIFY_HOSTNAME would have no effect;
             * chain validation is enforced regardless. */
            verify = SPICE_SESSION_VERIFY_SUBJECT;
        else if (settings->pubkey != NULL)
            /* Pin the server's public key (exact-key match). Adding
             * VERIFY_HOSTNAME would weaken this by also accepting any other
             * CA-valid certificate for the host, defeating the pin. */
            verify = SPICE_SESSION_VERIFY_PUBKEY;
        else
            /* Default: validate the CA chain and match the hostname. No key is
             * pinned, so VERIFY_PUBKEY must not be set here. */
            verify = SPICE_SESSION_VERIFY_HOSTNAME;

        g_object_set(session, "verify", verify, NULL);

    }

    /* Configure the authentication ticket (password) and username, if
     * provided. These credentials can be replaced at runtime via the argv
     * callback (guac_spice_argv_callback), which frees and reassigns the
     * settings strings under message_lock. Read them under the same lock and
     * copy into locals so a concurrent update cannot free a string mid-read
     * (use-after-free / data race); the copies are then applied to the session
     * without holding the lock and freed afterward. */
    pthread_mutex_lock(&spice_client->message_lock);
    char* password = guac_strdup(settings->password);
    char* username = guac_strdup(settings->username);
    pthread_mutex_unlock(&spice_client->message_lock);

    if (password != NULL)
        g_object_set(session, "password", password, NULL);

    if (username != NULL)
        g_object_set(session, "username", username, NULL);

    guac_mem_free(password);
    guac_mem_free(username);

    /* Configure preferred image compression, if requested */
    if (settings->preferred_compression != NULL) {

        static const struct {
            const char* name;
            SpiceImageCompression value;
        } compression_types[] = {
            { "off",      SPICE_IMAGE_COMPRESSION_OFF      },
            { "auto-glz", SPICE_IMAGE_COMPRESSION_AUTO_GLZ },
            { "auto-lz",  SPICE_IMAGE_COMPRESSION_AUTO_LZ  },
            { "quic",     SPICE_IMAGE_COMPRESSION_QUIC     },
            { "glz",      SPICE_IMAGE_COMPRESSION_GLZ      },
            { "lz",       SPICE_IMAGE_COMPRESSION_LZ       },
            { "lz4",      SPICE_IMAGE_COMPRESSION_LZ4      },
            { NULL,       SPICE_IMAGE_COMPRESSION_INVALID  }
        };

        SpiceImageCompression compression = SPICE_IMAGE_COMPRESSION_INVALID;
        for (int i = 0; compression_types[i].name != NULL; i++) {
            if (strcmp(settings->preferred_compression,
                        compression_types[i].name) == 0) {
                compression = compression_types[i].value;
                break;
            }
        }

        if (compression == SPICE_IMAGE_COMPRESSION_INVALID)
            guac_client_log(client, GUAC_LOG_WARNING, "Ignoring unknown "
                    "preferred image compression \"%s\".",
                    settings->preferred_compression);
        else
            g_object_set(session, "preferred-compression", compression, NULL);

    }

    /* Reflect read-only state to the session */
    g_object_set(session, "read-only", settings->read_only, NULL);

    /* Enable audio negotiation if either playback or input (record) is
     * requested; spice-gtk's "enable-audio" governs both audio channels. */
    g_object_set(session, "enable-audio",
            settings->audio_enabled || settings->audio_input_enabled, NULL);

    /* USB redirection and smartcard passthrough are not meaningful for a
     * headless proxy such as guacd, which has no local physical devices to
     * redirect. Disable both to avoid advertising unsupported channels. */
    g_object_set(session, "enable-usbredir", FALSE, NULL);
    g_object_set(session, "enable-smartcard", FALSE, NULL);

#ifdef ENABLE_SPICE_WEBDAV
    /* Expose shared folder via the SPICE WebDAV channel, if requested. The
     * SPICE client library (spice-gtk) drives the underlying phodav-based
     * WebDAV server automatically once "shared-dir" is set. */
    if (settings->enable_drive && settings->drive_path != NULL) {
        g_object_set(session, "shared-dir", settings->drive_path, NULL);
        g_object_set(session, "share-dir-ro", settings->drive_read_only, NULL);
        guac_client_log(client, GUAC_LOG_INFO,
                "Sharing directory \"%s\" with the SPICE server%s.",
                settings->drive_path,
                settings->drive_read_only ? " (read-only)" : "");
    }
#else
    if (settings->enable_drive)
        guac_client_log(client, GUAC_LOG_WARNING, "Folder sharing was "
                "requested but support for the SPICE WebDAV channel "
                "(libphodav) was not available at build time. Folder sharing "
                "will be disabled.");
#endif

}
