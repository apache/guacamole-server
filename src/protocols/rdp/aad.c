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

#ifdef HAVE_FREERDP_AAD_SUPPORT

#include "aad.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/string.h>
#include <guacamole/user.h>

#include <curl/curl.h>
#include <winpr/json.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**
 * Azure AD OAuth2 device code endpoint URL format. The %s placeholder is
 * replaced with the tenant ID.
 */
#define GUAC_AAD_DEVICE_CODE_ENDPOINT \
    "https://login.microsoftonline.com/%s/oauth2/v2.0/devicecode"

/**
 * Azure AD OAuth2 token endpoint URL format. The %s placeholder is replaced
 * with the tenant ID.
 */
#define GUAC_AAD_TOKEN_ENDPOINT \
    "https://login.microsoftonline.com/%s/oauth2/v2.0/token"

/**
 * The mimetype of the pipe stream carrying the device code prompt (user code
 * and verification URI) to the browser for display.
 */
#define GUAC_RDP_AAD_MIMETYPE "application/x-aad-device-code+json"

/**
 * The name of the pipe stream carrying the device code prompt. The browser
 * routes the prompt by its mimetype; the name identifies the pipe's purpose.
 */
#define GUAC_RDP_AAD_PIPE_NAME "aad-device-code"

/**
 * The User-Agent header sent with HTTP requests identifying the Guacamole
 * client to Microsoft's endpoints.
 */
#define GUAC_AAD_USER_AGENT "Guacamole"

/**
 * Initial size, in bytes, of the buffer used to receive an HTTP response. The
 * buffer grows automatically if a response exceeds this size.
 */
#define GUAC_AAD_HTTP_BUFFER_SIZE 8192

/**
 * The HTTP request timeout, in seconds.
 */
#define GUAC_AAD_HTTP_TIMEOUT_SECONDS 30

/**
 * Size, in bytes, of the buffer used to construct endpoint URLs.
 */
#define GUAC_AAD_URL_BUFFER_SIZE 512

/**
 * Base size, in bytes, added to the length of parameter values when sizing an
 * HTTP POST body.
 */
#define GUAC_AAD_POST_DATA_BASE_SIZE 1024

/**
 * The polling interval, in seconds, used if the device code response does not
 * specify one.
 */
#define GUAC_AAD_DEFAULT_POLL_INTERVAL 5

/**
 * The number of seconds by which the polling interval is increased when Azure
 * AD responds with "slow_down".
 */
#define GUAC_AAD_SLOW_DOWN_INCREMENT 5

/**
 * The lifetime, in seconds, assumed for a device code if the response does not
 * specify one.
 */
#define GUAC_AAD_DEFAULT_EXPIRES_IN 900

/**
 * The result of a single device code polling attempt.
 */
typedef enum guac_rdp_aad_poll_status {

    /**
     * The user has not yet completed authentication. Polling should continue.
     */
    GUAC_RDP_AAD_POLL_PENDING = 0,

    /**
     * An access token was successfully obtained.
     */
    GUAC_RDP_AAD_POLL_SUCCESS = 1,

    /**
     * Polling is occurring too frequently. The interval should be increased
     * before polling again.
     */
    GUAC_RDP_AAD_POLL_SLOW_DOWN = 2,

    /**
     * Authentication failed or the device code expired. Polling should stop.
     */
    GUAC_RDP_AAD_POLL_ERROR = -1

} guac_rdp_aad_poll_status;

/**
 * A growable buffer holding the body of an HTTP response.
 */
typedef struct guac_rdp_aad_response {

    /**
     * The response body data, always NULL-terminated.
     */
    char* data;

    /**
     * The current length of the response data, in bytes, excluding the NULL
     * terminator.
     */
    size_t size;

    /**
     * The allocated capacity of the data buffer, in bytes.
     */
    size_t capacity;

} guac_rdp_aad_response;

/**
 * The result of a successful device code request, as returned by Azure AD.
 */
typedef struct guac_rdp_aad_device_code {

    /**
     * The device code presented back to the token endpoint when polling.
     */
    char* device_code;

    /**
     * The short code the user enters at the verification URI.
     */
    char* user_code;

    /**
     * The URI the user visits to authenticate.
     */
    char* verification_uri;

    /**
     * The verification URI with the user code already embedded, suitable for
     * encoding directly into a QR code, or NULL if not provided.
     */
    char* verification_uri_complete;

    /**
     * The interval, in seconds, to wait between polling attempts.
     */
    int interval;

    /**
     * The number of seconds until the device code expires.
     */
    int expires_in;

} guac_rdp_aad_device_code;

/**
 * State shared between the device code prompt callbacks and the polling loop.
 */
typedef struct guac_rdp_aad_prompt {

    /**
     * The device code whose user code and verification URI should be shown.
     */
    guac_rdp_aad_device_code* dc;

    /**
     * The pipe stream opened to the owner to display the prompt, or NULL if no
     * prompt is currently shown. The stream is held open for the duration of
     * the device code flow and closed once it completes; that close is the
     * browser's signal to dismiss the QR overlay.
     */
    guac_stream* stream;

} guac_rdp_aad_prompt;

/**
 * Callback for libcurl which appends received HTTP data to a
 * guac_rdp_aad_response buffer, growing the buffer as needed.
 *
 * @param contents
 *     Pointer to the received data.
 *
 * @param size
 *     The size of each data element.
 *
 * @param nmemb
 *     The number of data elements.
 *
 * @param userp
 *     The guac_rdp_aad_response to append to.
 *
 * @return
 *     The number of bytes consumed, which must equal size * nmemb for the
 *     transfer to continue.
 */
static size_t guac_rdp_aad_write_callback(void* contents, size_t size,
        size_t nmemb, void* userp) {

    size_t realsize = size * nmemb;
    guac_rdp_aad_response* response = (guac_rdp_aad_response*) userp;

    /* Grow the buffer if the incoming data (plus NULL terminator) will not fit
     * within the current capacity */
    if (response->size + realsize + 1 > response->capacity) {

        size_t new_capacity = response->capacity * 2;
        if (new_capacity < response->size + realsize + 1)
            new_capacity = response->size + realsize + 1;

        char* new_data = guac_mem_realloc(response->data, new_capacity);
        if (new_data == NULL)
            return 0;

        response->data = new_data;
        response->capacity = new_capacity;
    }

    memcpy(response->data + response->size, contents, realsize);
    response->size += realsize;
    response->data[response->size] = '\0';

    return realsize;
}

/**
 * URL-encodes a string using the given CURL handle.
 *
 * @param curl
 *     The CURL handle to use for encoding.
 *
 * @param str
 *     The string to encode, or NULL.
 *
 * @return
 *     A newly-allocated URL-encoded string, or NULL on error or NULL input.
 *     The caller must free the result with curl_free().
 */
static char* guac_rdp_aad_urlencode(CURL* curl, const char* str) {
    if (str == NULL)
        return NULL;
    return curl_easy_escape(curl, str, strlen(str));
}

/**
 * Returns a newly-allocated copy of the string value of a JSON object field,
 * or NULL if the field is absent or not a string.
 *
 * @param object
 *     The parsed JSON object to read from.
 *
 * @param key
 *     The field name to read.
 *
 * @return
 *     A newly-allocated copy of the value, or NULL. The caller must free the
 *     result with guac_mem_free().
 */
static char* guac_rdp_aad_json_string(WINPR_JSON* object, const char* key) {

    WINPR_JSON* item = WINPR_JSON_GetObjectItem(object, key);
    if (item == NULL || !WINPR_JSON_IsString(item))
        return NULL;

    const char* value = WINPR_JSON_GetStringValue(item);
    if (value == NULL)
        return NULL;

    return guac_strdup(value);
}

/**
 * Returns the integer value of a JSON object field, or the given default if the
 * field is absent or not a number.
 *
 * @param object
 *     The parsed JSON object to read from.
 *
 * @param key
 *     The field name to read.
 *
 * @param default_value
 *     The value to return if the field is absent or not a number.
 *
 * @return
 *     The integer value of the field, or default_value.
 */
static int guac_rdp_aad_json_int(WINPR_JSON* object, const char* key,
        int default_value) {

    WINPR_JSON* item = WINPR_JSON_GetObjectItem(object, key);
    if (item == NULL || !WINPR_JSON_IsNumber(item))
        return default_value;

    return (int) WINPR_JSON_GetNumberValue(item);
}

/**
 * Logs the error description from a parsed Azure AD JSON error response, if
 * present.
 *
 * @param client
 *     The guac_client to log to.
 *
 * @param object
 *     The parsed JSON error response.
 */
static void guac_rdp_aad_log_error(guac_client* client, WINPR_JSON* object) {

    char* error_desc = guac_rdp_aad_json_string(object, "error_description");
    if (error_desc != NULL) {
        guac_client_log(client, GUAC_LOG_ERROR, "AAD: %s", error_desc);
        guac_mem_free(error_desc);
    }
}

/**
 * Allocates a new, empty guac_rdp_aad_response.
 *
 * @return
 *     A newly-allocated response, or NULL on allocation failure. The caller
 *     must free the result with guac_rdp_aad_response_free().
 */
static guac_rdp_aad_response* guac_rdp_aad_response_alloc(void) {

    guac_rdp_aad_response* response =
            guac_mem_zalloc(sizeof(guac_rdp_aad_response));

    if (response == NULL)
        return NULL;

    response->capacity = GUAC_AAD_HTTP_BUFFER_SIZE;
    response->data = guac_mem_alloc(response->capacity);

    if (response->data == NULL) {
        guac_mem_free(response);
        return NULL;
    }

    response->data[0] = '\0';
    return response;
}

/**
 * Frees a guac_rdp_aad_response and its buffer.
 *
 * @param response
 *     The response to free, or NULL.
 */
static void guac_rdp_aad_response_free(guac_rdp_aad_response* response) {
    if (response == NULL)
        return;

    guac_mem_free(response->data);
    guac_mem_free(response);
}

/**
 * Performs an application/x-www-form-urlencoded HTTP POST and returns the
 * response body.
 *
 * @param client
 *     The guac_client to log to.
 *
 * @param url
 *     The URL to POST to.
 *
 * @param post_data
 *     The form-encoded POST body.
 *
 * @return
 *     A newly-allocated response on success, or NULL on failure. The caller
 *     must free the result with guac_rdp_aad_response_free().
 */
static guac_rdp_aad_response* guac_rdp_aad_http_post(guac_client* client,
        const char* url, const char* post_data) {

    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to initialize libcurl");
        return NULL;
    }

    guac_rdp_aad_response* response = guac_rdp_aad_response_alloc();
    if (response == NULL) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    struct curl_slist* headers = curl_slist_append(NULL,
            "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, guac_rdp_aad_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, GUAC_AAD_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,
            (long) GUAC_AAD_HTTP_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: HTTP request failed: %s", curl_easy_strerror(res));
        guac_rdp_aad_response_free(response);
        return NULL;
    }

    return response;
}

char* guac_rdp_percent_decode(const char* str) {

    if (str == NULL)
        return NULL;

    size_t len = strlen(str);
    char* decoded = guac_mem_alloc(len + 1);
    size_t out_pos = 0;

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '%' && i + 2 < len) {
            char hex[3] = { str[i + 1], str[i + 2], '\0' };
            char* hex_end;
            long byte_val = strtol(hex, &hex_end, 16);
            if (hex_end == hex + 2) {
                decoded[out_pos++] = (char) byte_val;
                i += 2;
                continue;
            }
        }
        decoded[out_pos++] = str[i];
    }

    decoded[out_pos] = '\0';
    return decoded;
}

/**
 * Frees a guac_rdp_aad_device_code and all associated strings.
 *
 * @param dc
 *     The device code to free, or NULL.
 */
static void guac_rdp_aad_device_code_free(guac_rdp_aad_device_code* dc) {
    if (dc == NULL)
        return;

    guac_mem_free(dc->device_code);
    guac_mem_free(dc->user_code);
    guac_mem_free(dc->verification_uri);
    guac_mem_free(dc->verification_uri_complete);
    guac_mem_free(dc);
}

/**
 * Requests a device code from Azure AD to begin the OAuth2 device authorization
 * grant (RFC 8628).
 *
 * @param client
 *     The guac_client associated with the RDP connection, used for logging.
 *
 * @param tenant_id
 *     The Azure AD tenant ID (or "organizations").
 *
 * @param client_id
 *     The application (client) ID of the Azure AD app registration.
 *
 * @param scope
 *     The OAuth2 scope to request.
 *
 * @return
 *     A newly-allocated device code on success, or NULL on failure. The caller
 *     must free the result with guac_rdp_aad_device_code_free().
 */
static guac_rdp_aad_device_code* guac_rdp_aad_device_code_request(
        guac_client* client, const char* tenant_id, const char* client_id,
        const char* scope) {

    if (client_id == NULL || tenant_id == NULL || scope == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: client_id, tenant_id, and scope are required for the "
                "device code request");
        return NULL;
    }

    char url[GUAC_AAD_URL_BUFFER_SIZE];
    snprintf(url, sizeof(url), GUAC_AAD_DEVICE_CODE_ENDPOINT, tenant_id);

    /* URL-encode the request parameters */
    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to initialize libcurl for URL encoding");
        return NULL;
    }

    char* encoded_client_id = guac_rdp_aad_urlencode(curl, client_id);
    char* encoded_scope = guac_rdp_aad_urlencode(curl, scope);
    curl_easy_cleanup(curl);

    if (encoded_client_id == NULL || encoded_scope == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to URL-encode device code parameters");
        curl_free(encoded_client_id);
        curl_free(encoded_scope);
        return NULL;
    }

    size_t post_data_size = GUAC_AAD_POST_DATA_BASE_SIZE
            + strlen(encoded_client_id) + strlen(encoded_scope);
    char* post_data = guac_mem_alloc(post_data_size);
    snprintf(post_data, post_data_size, "client_id=%s&scope=%s",
            encoded_client_id, encoded_scope);

    curl_free(encoded_client_id);
    curl_free(encoded_scope);

    guac_rdp_aad_response* response =
            guac_rdp_aad_http_post(client, url, post_data);
    guac_mem_free(post_data);

    if (response == NULL)
        return NULL;

    WINPR_JSON* json = WINPR_JSON_Parse(response->data);
    if (json == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Device code response was not valid JSON");
        guac_rdp_aad_response_free(response);
        return NULL;
    }

    /* Parse the device code fields from the response */
    guac_rdp_aad_device_code* dc =
            guac_mem_zalloc(sizeof(guac_rdp_aad_device_code));

    dc->device_code = guac_rdp_aad_json_string(json, "device_code");
    dc->user_code = guac_rdp_aad_json_string(json, "user_code");
    dc->verification_uri = guac_rdp_aad_json_string(json, "verification_uri");
    dc->verification_uri_complete =
            guac_rdp_aad_json_string(json, "verification_uri_complete");
    dc->interval = guac_rdp_aad_json_int(json, "interval",
            GUAC_AAD_DEFAULT_POLL_INTERVAL);
    dc->expires_in = guac_rdp_aad_json_int(json, "expires_in",
            GUAC_AAD_DEFAULT_EXPIRES_IN);

    /* The device code request failed if the required fields are absent */
    if (dc->device_code == NULL || dc->user_code == NULL
            || dc->verification_uri == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Device code response missing required fields");
        guac_rdp_aad_log_error(client, json);
        WINPR_JSON_Delete(json);
        guac_rdp_aad_response_free(response);
        guac_rdp_aad_device_code_free(dc);
        return NULL;
    }

    WINPR_JSON_Delete(json);
    guac_rdp_aad_response_free(response);

    guac_client_log(client, GUAC_LOG_INFO,
            "AAD: Device code obtained (user code: %s, verification URI: %s, "
            "expires in %d seconds)",
            dc->user_code, dc->verification_uri, dc->expires_in);

    return dc;
}

/**
 * Polls the Azure AD token endpoint once for the result of the device code
 * flow.
 *
 * @param client
 *     The guac_client associated with the RDP connection, used for logging.
 *
 * @param tenant_id
 *     The Azure AD tenant ID (or "organizations").
 *
 * @param client_id
 *     The application (client) ID of the Azure AD app registration.
 *
 * @param device_code
 *     The device code returned by the device code request.
 *
 * @param req_cnf
 *     The proof-of-possession key confirmation supplied by FreeRDP, or NULL.
 *
 * @param poll_status
 *     Pointer to a guac_rdp_aad_poll_status which receives the outcome of the
 *     poll.
 *
 * @return
 *     A newly-allocated access token if authentication has completed, or NULL
 *     otherwise (with poll_status indicating whether to keep polling). The
 *     caller must free any returned token with guac_mem_free().
 */
static char* guac_rdp_aad_device_code_poll(guac_client* client,
        const char* tenant_id, const char* client_id, const char* device_code,
        const char* req_cnf, int* poll_status) {

    *poll_status = GUAC_RDP_AAD_POLL_PENDING;

    char url[GUAC_AAD_URL_BUFFER_SIZE];
    snprintf(url, sizeof(url), GUAC_AAD_TOKEN_ENDPOINT, tenant_id);

    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        *poll_status = GUAC_RDP_AAD_POLL_ERROR;
        return NULL;
    }

    char* encoded_client_id = guac_rdp_aad_urlencode(curl, client_id);
    char* encoded_device_code = guac_rdp_aad_urlencode(curl, device_code);

    /* req_cnf (proof-of-possession) is included only when FreeRDP provides one */
    char* encoded_req_cnf = (req_cnf != NULL && strlen(req_cnf) > 0)
            ? guac_rdp_aad_urlencode(curl, req_cnf) : NULL;

    curl_easy_cleanup(curl);

    if (encoded_client_id == NULL || encoded_device_code == NULL) {
        curl_free(encoded_client_id);
        curl_free(encoded_device_code);
        curl_free(encoded_req_cnf);
        *poll_status = GUAC_RDP_AAD_POLL_ERROR;
        return NULL;
    }

    /* The grant_type value is percent-encoded literally to avoid a second
     * round of encoding on its own reserved characters */
    size_t post_data_size = GUAC_AAD_POST_DATA_BASE_SIZE
            + strlen(encoded_client_id) + strlen(encoded_device_code)
            + (encoded_req_cnf ? strlen(encoded_req_cnf) : 0);
    char* post_data = guac_mem_alloc(post_data_size);

    int written = snprintf(post_data, post_data_size,
            "grant_type=urn%%3Aietf%%3Aparams%%3Aoauth%%3Agrant-type%%3Adevice_code"
            "&client_id=%s&device_code=%s",
            encoded_client_id, encoded_device_code);

    if (encoded_req_cnf != NULL && written > 0
            && (size_t) written < post_data_size - 1)
        snprintf(post_data + written, post_data_size - written,
                "&req_cnf=%s", encoded_req_cnf);

    curl_free(encoded_client_id);
    curl_free(encoded_device_code);
    curl_free(encoded_req_cnf);

    guac_rdp_aad_response* response =
            guac_rdp_aad_http_post(client, url, post_data);
    guac_mem_free(post_data);

    if (response == NULL) {
        *poll_status = GUAC_RDP_AAD_POLL_ERROR;
        return NULL;
    }

    WINPR_JSON* json = WINPR_JSON_Parse(response->data);
    guac_rdp_aad_response_free(response);

    if (json == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Token response was not valid JSON");
        *poll_status = GUAC_RDP_AAD_POLL_ERROR;
        return NULL;
    }

    /* An access token in the response indicates authentication has completed */
    char* token = guac_rdp_aad_json_string(json, "access_token");

    if (token != NULL) {
        char* token_type = guac_rdp_aad_json_string(json, "token_type");
        guac_client_log(client, GUAC_LOG_INFO,
                "AAD: Access token obtained (token_type: %s)",
                token_type != NULL ? token_type : "unknown");
        guac_mem_free(token_type);
        WINPR_JSON_Delete(json);
        *poll_status = GUAC_RDP_AAD_POLL_SUCCESS;
        return token;
    }

    /* Otherwise the response describes the current state of the flow */
    char* error = guac_rdp_aad_json_string(json, "error");

    if (error == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Unexpected response from token endpoint");
        *poll_status = GUAC_RDP_AAD_POLL_ERROR;
    }
    else if (strcmp(error, "authorization_pending") == 0)
        *poll_status = GUAC_RDP_AAD_POLL_PENDING;
    else if (strcmp(error, "slow_down") == 0)
        *poll_status = GUAC_RDP_AAD_POLL_SLOW_DOWN;
    else {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Device code flow failed: %s", error);
        guac_rdp_aad_log_error(client, json);
        *poll_status = GUAC_RDP_AAD_POLL_ERROR;
    }

    guac_mem_free(error);
    WINPR_JSON_Delete(json);
    return NULL;
}

/**
 * Callback which pushes the device code prompt to the session owner over a pipe
 * stream so the browser can display it (as a QR code). The stream is left open,
 * to be closed by guac_rdp_aad_close_prompt() once the flow completes. Invoked
 * via guac_client_for_owner().
 *
 * @param user
 *     The session owner to send the prompt to, or NULL if the connection has no
 *     owner.
 *
 * @param data
 *     The guac_rdp_aad_prompt describing the prompt to display. Its stream
 *     field receives the opened pipe stream.
 *
 * @return
 *     Always NULL.
 */
static void* guac_rdp_aad_send_prompt(guac_user* user, void* data) {

    /* There may be no owner to display the prompt to */
    if (user == NULL)
        return NULL;

    guac_rdp_aad_prompt* prompt = (guac_rdp_aad_prompt*) data;
    guac_rdp_aad_device_code* dc = prompt->dc;

    /* Build the prompt as JSON, letting WINPR escape the values */
    WINPR_JSON* json = WINPR_JSON_CreateObject();
    if (json == NULL)
        return NULL;

    if (!WINPR_JSON_AddStringToObject(json, "user_code", dc->user_code)
            || !WINPR_JSON_AddStringToObject(json, "verification_uri",
                    dc->verification_uri)
            || !WINPR_JSON_AddStringToObject(json, "verification_uri_complete",
                    dc->verification_uri_complete != NULL
                            ? dc->verification_uri_complete : "")
            || !WINPR_JSON_AddNumberToObject(json, "expires_in",
                    dc->expires_in)) {
        WINPR_JSON_Delete(json);
        return NULL;
    }

    char* body = WINPR_JSON_PrintUnformatted(json);
    WINPR_JSON_Delete(json);
    if (body == NULL)
        return NULL;

    guac_stream* stream = guac_user_alloc_stream(user);
    if (stream == NULL) {
        guac_client_log(user->client, GUAC_LOG_WARNING,
                "AAD: No stream available to display the sign-in prompt");
        free(body);
        return NULL;
    }

    int failed = guac_protocol_send_pipe(user->socket, stream,
                    GUAC_RDP_AAD_MIMETYPE, GUAC_RDP_AAD_PIPE_NAME)
            || guac_protocol_send_blob(user->socket, stream, body, strlen(body))
            || guac_socket_flush(user->socket);

    free(body);

    if (failed) {
        guac_client_log(user->client, GUAC_LOG_DEBUG,
                "AAD: Failed to send the sign-in prompt to the owner");
        guac_user_free_stream(user, stream);
        return NULL;
    }

    prompt->stream = stream;
    return NULL;
}

/**
 * Callback which closes the pipe stream opened by guac_rdp_aad_send_prompt(),
 * signaling the browser to dismiss the QR overlay. Invoked via
 * guac_client_for_owner() once the device code flow completes.
 *
 * @param user
 *     The session owner the prompt was sent to, or NULL if the connection no
 *     longer has an owner.
 *
 * @param data
 *     The guac_rdp_aad_prompt whose stream should be closed.
 *
 * @return
 *     Always NULL.
 */
static void* guac_rdp_aad_close_prompt(guac_user* user, void* data) {

    guac_rdp_aad_prompt* prompt = (guac_rdp_aad_prompt*) data;

    /* Nothing to close if the owner left or no prompt was shown. A departed
     * owner has its streams freed on disconnect; the connection is a single
     * owner's, so the stream never outlives that owner. */
    if (user == NULL || prompt->stream == NULL)
        return NULL;

    if (guac_protocol_send_end(user->socket, prompt->stream)
            || guac_socket_flush(user->socket))
        guac_client_log(user->client, GUAC_LOG_DEBUG,
                "AAD: Failed to close the sign-in prompt");

    guac_user_free_stream(user, prompt->stream);
    prompt->stream = NULL;
    return NULL;
}

char* guac_rdp_aad_get_token(guac_client* client, const char* tenant_id,
        const char* client_id, const char* scope, const char* req_cnf) {

    if (tenant_id == NULL || client_id == NULL || scope == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: tenant, client ID, and scope are all required for "
                "authentication");
        return NULL;
    }

    /* Begin the device authorization grant */
    guac_rdp_aad_device_code* dc = guac_rdp_aad_device_code_request(client,
            tenant_id, client_id, scope);
    if (dc == NULL)
        return NULL;

    /* Show the user code and verification URI to the session owner, holding the
     * stream open so it can be closed to dismiss the prompt when done */
    guac_rdp_aad_prompt prompt = { dc, NULL };
    guac_client_for_owner(client, guac_rdp_aad_send_prompt, &prompt);

    guac_client_log(client, GUAC_LOG_INFO,
            "AAD: Waiting for the user to complete sign-in...");

    /* Poll until the user completes sign-in, the code expires, or the
     * connection closes */
    int interval = dc->interval > 0
            ? dc->interval : GUAC_AAD_DEFAULT_POLL_INTERVAL;
    time_t deadline = time(NULL) + dc->expires_in;
    char* token = NULL;

    while (client->state == GUAC_CLIENT_RUNNING && time(NULL) < deadline) {

        sleep(interval);

        int poll_status;
        token = guac_rdp_aad_device_code_poll(client, tenant_id, client_id,
                dc->device_code, req_cnf, &poll_status);

        if (poll_status == GUAC_RDP_AAD_POLL_SUCCESS)
            break;

        if (poll_status == GUAC_RDP_AAD_POLL_ERROR)
            break;

        if (poll_status == GUAC_RDP_AAD_POLL_SLOW_DOWN)
            interval += GUAC_AAD_SLOW_DOWN_INCREMENT;

        /* GUAC_RDP_AAD_POLL_PENDING: keep polling */
    }

    /* Dismiss the prompt in the browser now that the flow has completed */
    guac_client_for_owner(client, guac_rdp_aad_close_prompt, &prompt);

    guac_rdp_aad_device_code_free(dc);

    if (token == NULL)
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: No access token was obtained (sign-in was not completed)");

    return token;
}

#endif /* HAVE_FREERDP_AAD_SUPPORT */
