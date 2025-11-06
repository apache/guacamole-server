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

#include "channels/rdpecam/rdpecam_caps.h"
#include "rdp.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/user.h>

#include <CUnit/CUnit.h>
#include <stdlib.h>
#include <string.h>

/**
 * Creates a minimal mock guac_client and guac_rdp_client for testing.
 * Returns the guac_client, with rdp_client stored in client->data.
 */
static guac_client* create_mock_client_with_rdp(void) {
    guac_client* client = guac_mem_zalloc(sizeof(guac_client));
    if (!client)
        return NULL;

    guac_rdp_client* rdp_client = guac_mem_zalloc(sizeof(guac_rdp_client));
    if (!rdp_client) {
        guac_mem_free(client);
        return NULL;
    }

    client->data = rdp_client;
    client->log_level = GUAC_LOG_DEBUG;

    if (guac_rwlock_init(&rdp_client->lock) != 0) {
        guac_mem_free(rdp_client);
        guac_mem_free(client);
        return NULL;
    }

    rdp_client->rdpecam_device_caps = guac_mem_zalloc(
            sizeof(guac_rdp_rdpecam_device_caps) * GUAC_RDP_RDPECAM_MAX_DEVICES);
    if (!rdp_client->rdpecam_device_caps) {
        guac_rwlock_destroy(&rdp_client->lock);
        guac_mem_free(rdp_client);
        guac_mem_free(client);
        return NULL;
    }

    return client;
}

/**
 * Frees a mock guac_client created by create_mock_client_with_rdp().
 */
static void free_mock_client_with_rdp(guac_client* client) {
    if (!client)
        return;

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    if (!rdp_client)
        return;

    /* Free device capabilities */
    for (unsigned int i = 0; i < rdp_client->rdpecam_device_caps_count; i++) {
        guac_rdp_rdpecam_device_caps* caps = &rdp_client->rdpecam_device_caps[i];
        if (caps->device_id)
            guac_mem_free(caps->device_id);
        if (caps->device_name)
            guac_mem_free(caps->device_name);
    }

    if (rdp_client->rdpecam_device_caps)
        guac_mem_free(rdp_client->rdpecam_device_caps);

    guac_rwlock_destroy(&rdp_client->lock);
    guac_mem_free(rdp_client);
    guac_mem_free(client);
}

/**
 * Creates a minimal mock guac_user for testing.
 */
static guac_user* create_mock_user(guac_client* client) {
    guac_user* user = guac_mem_zalloc(sizeof(guac_user));
    if (user) {
        user->client = client;
    }
    return user;
}

/**
 * Frees a mock guac_user created by create_mock_user().
 */
static void free_mock_user(guac_user* user) {
    if (user)
        guac_mem_free(user);
}

/**
 * Test which verifies that sanitize_device_name handles valid names.
 */
void test_rdpecam_caps__sanitize_valid_name(void) {
    char sanitized[256];
    size_t result = guac_rdp_rdpecam_sanitize_device_name("My Camera", sanitized, sizeof(sanitized));
    CU_ASSERT_EQUAL(result, strlen("My Camera"));
    CU_ASSERT_NSTRING_EQUAL(sanitized, "My Camera", result);
}

/**
 * Test which verifies that sanitize_device_name replaces invalid characters.
 */
void test_rdpecam_caps__sanitize_invalid_chars(void) {
    char sanitized[256];
    size_t result = guac_rdp_rdpecam_sanitize_device_name("Camera/Name\\Test:Device*", sanitized, sizeof(sanitized));
    CU_ASSERT(result > 0);
    CU_ASSERT(strchr(sanitized, '/') == NULL);
    CU_ASSERT(strchr(sanitized, '\\') == NULL);
    CU_ASSERT(strchr(sanitized, ':') == NULL);
    CU_ASSERT(strchr(sanitized, '*') == NULL);
}

/**
 * Test which verifies that sanitize_device_name handles NULL input.
 */
void test_rdpecam_caps__sanitize_null_name(void) {
    char sanitized[256];
    size_t result = guac_rdp_rdpecam_sanitize_device_name(NULL, sanitized, sizeof(sanitized));
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * Test which verifies that sanitize_device_name handles NULL output buffer.
 */
void test_rdpecam_caps__sanitize_null_buffer(void) {
    size_t result = guac_rdp_rdpecam_sanitize_device_name("Camera", NULL, 256);
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * Test which verifies that sanitize_device_name truncates to 255 characters.
 */
void test_rdpecam_caps__sanitize_truncate(void) {
    char long_name[300];
    memset(long_name, 'A', 299);
    long_name[299] = '\0';

    char sanitized[256];
    size_t result = guac_rdp_rdpecam_sanitize_device_name(long_name, sanitized, sizeof(sanitized));
    CU_ASSERT_EQUAL(result, 255);
    CU_ASSERT_EQUAL(strlen(sanitized), 255);
}

/**
 * Test which verifies that sanitize_device_name handles zero-length buffer.
 */
void test_rdpecam_caps__sanitize_zero_buffer(void) {
    char sanitized[256];
    size_t result = guac_rdp_rdpecam_sanitize_device_name("Camera", sanitized, 0);
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * Test which verifies that capabilities_callback parses a single device correctly.
 */
void test_rdpecam_caps__capabilities_single_device(void) {
    guac_client* client = create_mock_client_with_rdp();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_user* user = create_mock_user(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);

    const char* capabilities = "device123:My Camera|640x480@30/1,1280x720@30/1";
    int result = guac_rdp_rdpecam_capabilities_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, capabilities, NULL);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps_count, 1);
    CU_ASSERT_PTR_NOT_NULL(rdp_client->rdpecam_device_caps[0].device_id);
    CU_ASSERT_NSTRING_EQUAL(rdp_client->rdpecam_device_caps[0].device_id, "device123", strlen("device123"));
    CU_ASSERT_PTR_NOT_NULL(rdp_client->rdpecam_device_caps[0].device_name);
    CU_ASSERT_NSTRING_EQUAL(rdp_client->rdpecam_device_caps[0].device_name, "My Camera", strlen("My Camera"));
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps[0].format_count, 2);

    free_mock_user(user);
    free_mock_client_with_rdp(client);
}

/**
 * Test which verifies that capabilities_callback parses multiple devices correctly.
 */
void test_rdpecam_caps__capabilities_multiple_devices(void) {
    guac_client* client = create_mock_client_with_rdp();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_user* user = create_mock_user(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);

    const char* capabilities = "device1:Camera 1|640x480@30/1;device2:Camera 2|1280x720@60/1";
    int result = guac_rdp_rdpecam_capabilities_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, capabilities, NULL);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps_count, 2);
    CU_ASSERT_PTR_NOT_NULL(rdp_client->rdpecam_device_caps[0].device_id);
    CU_ASSERT_PTR_NOT_NULL(rdp_client->rdpecam_device_caps[1].device_id);

    free_mock_user(user);
    free_mock_client_with_rdp(client);
}

/**
 * Test which verifies that capabilities_callback handles invalid format.
 */
void test_rdpecam_caps__capabilities_invalid_format(void) {
    guac_client* client = create_mock_client_with_rdp();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_user* user = create_mock_user(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);

    const char* capabilities = "invalid-format-without-semicolon";
    int result = guac_rdp_rdpecam_capabilities_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, capabilities, NULL);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps_count, 0);

    free_mock_user(user);
    free_mock_client_with_rdp(client);
}

/**
 * Test which verifies that capabilities_callback handles NULL user.
 */
void test_rdpecam_caps__capabilities_null_user(void) {
    int result = guac_rdp_rdpecam_capabilities_callback(NULL, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, "device1:Camera|640x480@30/1", NULL);
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * Test which verifies that capabilities_callback handles NULL value.
 */
void test_rdpecam_caps__capabilities_null_value(void) {
    guac_client* client = create_mock_client_with_rdp();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_user* user = create_mock_user(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);

    int result = guac_rdp_rdpecam_capabilities_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, NULL, NULL);

    CU_ASSERT_EQUAL(result, 0);

    free_mock_user(user);
    free_mock_client_with_rdp(client);
}

/**
 * Test which verifies that capabilities_update_callback handles empty string.
 */
void test_rdpecam_caps__capabilities_update_empty(void) {
    guac_client* client = create_mock_client_with_rdp();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_user* user = create_mock_user(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);

    /* First set some capabilities */
    const char* capabilities = "device1:Camera|640x480@30/1";
    guac_rdp_rdpecam_capabilities_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, capabilities, NULL);
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps_count, 1);

    /* Then clear them with empty update */
    int result = guac_rdp_rdpecam_capabilities_update_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES_UPDATE, "", NULL);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps_count, 0);

    free_mock_user(user);
    free_mock_client_with_rdp(client);
}

/**
 * Test which verifies that capabilities_callback handles device without colon separator.
 */
void test_rdpecam_caps__capabilities_no_colon(void) {
    guac_client* client = create_mock_client_with_rdp();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_user* user = create_mock_user(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);

    const char* capabilities = "device1|640x480@30/1;device2:Camera|1280x720@30/1";
    int result = guac_rdp_rdpecam_capabilities_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, capabilities, NULL);

    CU_ASSERT_EQUAL(result, 0);
    /* First device should be skipped, second should be parsed */
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps_count, 1);

    free_mock_user(user);
    free_mock_client_with_rdp(client);
}

/**
 * Test which verifies that capabilities_callback handles device without formats.
 */
void test_rdpecam_caps__capabilities_no_formats(void) {
    guac_client* client = create_mock_client_with_rdp();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_user* user = create_mock_user(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);

    const char* capabilities = "device1:Camera|;device2:Camera 2|640x480@30/1";
    int result = guac_rdp_rdpecam_capabilities_callback(user, NULL,
            GUAC_RDPECAM_ARG_CAPABILITIES, capabilities, NULL);

    CU_ASSERT_EQUAL(result, 0);
    /* First device should be skipped (no formats), second should be parsed */
    CU_ASSERT_EQUAL(rdp_client->rdpecam_device_caps_count, 1);

    free_mock_user(user);
    free_mock_client_with_rdp(client);
}

