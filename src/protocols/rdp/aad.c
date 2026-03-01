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
#include <guacamole/string.h>

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

/**
 * Azure AD OAuth2 token endpoint URL format.
 * The %s placeholder is replaced with the tenant ID.
 */
#define GUAC_AAD_TOKEN_ENDPOINT \
    "https://login.microsoftonline.com/%s/oauth2/v2.0/token"

/**
 * Azure AD OAuth2 authorization endpoint URL format.
 * The %s placeholder is replaced with the tenant ID.
 */
#define GUAC_AAD_AUTHORIZE_ENDPOINT \
    "https://login.microsoftonline.com/%s/oauth2/v2.0/authorize"

/**
 * The native client redirect URI used for the authorization code flow.
 * This is a special Microsoft-provided redirect URI for non-web applications.
 */
#define GUAC_AAD_NATIVE_REDIRECT_URI \
    "https://login.microsoftonline.com/common/oauth2/nativeclient"

/**
 * Maximum size for the login page HTML response.
 */
#define GUAC_AAD_LOGIN_PAGE_MAX_SIZE (64 * 1024)

/**
 * HTTP request timeout in seconds.
 */
#define GUAC_AAD_HTTP_TIMEOUT_SECONDS 30

/**
 * User-Agent string sent with all HTTP requests to Microsoft login endpoints.
 * A browser-like UA is required to avoid "unsupported browser" responses.
 */
#define GUAC_AAD_USER_AGENT \
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 " \
    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"

/**
 * HTTP response structure for AAD requests.
 */
typedef struct guac_rdp_aad_response {

    /**
     * The response body data.
     */
    char* data;

    /**
     * The current size of the response data.
     */
    size_t size;

} guac_rdp_aad_response;

/**
 * Callback function for libcurl to write received HTTP data into a
 * guac_rdp_aad_response buffer.
 *
 * @param contents
 *     Pointer to the received data.
 *
 * @param size
 *     Size of each data element.
 *
 * @param nmemb
 *     Number of data elements.
 *
 * @param userp
 *     User-provided pointer (guac_rdp_aad_response structure).
 *
 * @return
 *     The number of bytes processed.
 */
static size_t guac_rdp_aad_write_callback(void* contents, size_t size,
        size_t nmemb, void* userp) {

    size_t total_size = size * nmemb;
    guac_rdp_aad_response* response = (guac_rdp_aad_response*) userp;

    /* Reject responses that exceed the maximum login page size */
    if (response->size + total_size > GUAC_AAD_LOGIN_PAGE_MAX_SIZE)
        return 0;

    /* Copy data into response buffer and null-terminate */
    memcpy(response->data + response->size, contents, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';

    return total_size;
}

/**
 * URL-encodes a string for use in HTTP POST data or query parameters.
 *
 * @param curl
 *     The CURL handle to use for encoding.
 *
 * @param str
 *     The string to encode.
 *
 * @return
 *     A newly allocated URL-encoded string, or NULL on error. The caller
 *     must free this string using curl_free().
 */
static char* guac_rdp_aad_urlencode(CURL* curl, const char* str) {
    if (str == NULL)
        return NULL;
    return curl_easy_escape(curl, str, strlen(str));
}

/**
 * Allocates and initializes a new guac_rdp_aad_response structure with a
 * fixed buffer large enough to hold the maximum allowed response.
 *
 * @return
 *     A newly allocated response structure, or NULL on allocation failure.
 *     The caller must free this with guac_rdp_aad_response_free().
 */
static guac_rdp_aad_response* guac_rdp_aad_response_alloc(void) {

    guac_rdp_aad_response* response =
            guac_mem_zalloc(sizeof(guac_rdp_aad_response));

    if (response == NULL)
        return NULL;

    /* Allocate the maximum allowed size upfront so the write callback
     * never needs to reallocate */
    response->data = guac_mem_alloc(GUAC_AAD_LOGIN_PAGE_MAX_SIZE + 1);

    if (response->data == NULL) {
        guac_mem_free(response);
        return NULL;
    }

    response->data[0] = '\0';

    return response;
}

/**
 * Extracts a string value from the $Config JavaScript object embedded in
 * the Microsoft login page HTML. Searches for the pattern "key":" and
 * returns the value up to the next unescaped double-quote.
 *
 * @param html
 *     The HTML string to search.
 *
 * @param key
 *     The JSON key name to find (without quotes).
 *
 * @return
 *     A newly allocated string containing the extracted value, or NULL if
 *     the key was not found. The caller must free with guac_mem_free().
 */
static char* guac_rdp_aad_extract_config_value(const char* html,
        const char* key) {

    if (html == NULL || key == NULL)
        return NULL;

    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

    const char* value_start = strstr(html, pattern);
    if (value_start == NULL)
        return NULL;

    value_start += strlen(pattern);

    /* Find closing quote, skipping escaped characters */
    const char* value_end = value_start;
    while (*value_end != '\0') {
        if (*value_end == '\\' && *(value_end + 1) != '\0') {
            /* Skip escaped character */
            value_end += 2;
            continue;
        }
        if (*value_end == '"')
            break;
        value_end++;
    }

    if (*value_end != '"')
        return NULL;

    size_t value_len = value_end - value_start;
    return guac_strndup(value_start, value_len);
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
 * Parses the JSON response from a token exchange request and extracts the
 * access token. If the response contains an error_description field instead,
 * that error is logged.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param json_response
 *     The raw JSON response body from the token endpoint.
 *
 * @return
 *     A newly allocated string containing the access token, or NULL if
 *     parsing failed or the response contained an error. The caller must
 *     free the returned string with guac_mem_free().
 */
static char* guac_rdp_aad_parse_token_response(guac_client* client,
        const char* json_response) {

    if (json_response == NULL)
        return NULL;

    /* Look for access_token in the JSON response */
    const char* token_key = "\"access_token\"";
    const char* token_pos = strstr(json_response, token_key);

    if (token_pos == NULL) {

        /* Log error description if present instead */
        const char* error_desc_key = "\"error_description\"";
        const char* error_desc_pos = strstr(json_response, error_desc_key);
        if (error_desc_pos != NULL)
            error_desc_pos = strchr(error_desc_pos
                    + strlen(error_desc_key), '"');
        if (error_desc_pos != NULL) {
            error_desc_pos++;
            const char* error_end = strchr(error_desc_pos, '"');
            if (error_end != NULL && error_end > error_desc_pos)
                guac_client_log(client, GUAC_LOG_ERROR,
                        "AAD authentication error: %.*s",
                        (int)(error_end - error_desc_pos), error_desc_pos);
        }

        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: No access_token found in response");
        return NULL;
    }

    /* Extract token value from JSON */
    const char* value_start = strchr(token_pos + strlen(token_key), '"');
    if (value_start == NULL)
        return NULL;

    value_start++;

    const char* value_end = strchr(value_start, '"');
    if (value_end == NULL)
        return NULL;

    size_t token_length = value_end - value_start;
    if (token_length == 0) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Empty access token in response");
        return NULL;
    }

    return guac_strndup(value_start, token_length);
}

/**
 * Frees a guac_rdp_aad_response structure and its associated data buffer.
 *
 * @param response
 *     The response structure to free, or NULL (in which case this function
 *     is a no-op).
 */
static void guac_rdp_aad_response_free(guac_rdp_aad_response* response) {
    if (response == NULL)
        return;

    guac_mem_free(response->data);
    guac_mem_free(response);
}

/**
 * Builds the OAuth2 authorization URL for the Azure AD login endpoint,
 * including all required query parameters.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param params
 *     The AAD authentication parameters containing tenant ID, client ID,
 *     and scope.
 *
 * @param url_buffer
 *     Buffer to receive the constructed authorization URL.
 *
 * @param buffer_size
 *     Size of url_buffer in bytes.
 *
 * @return
 *     Zero on success, non-zero if the URL could not be constructed.
 */
static int guac_rdp_aad_build_auth_url(guac_client* client,
        guac_rdp_aad_params* params, char* url_buffer, size_t buffer_size) {

    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to initialize curl for URL building");
        return 1;
    }

    /* URL-encode query parameters */
    char* encoded_client_id = guac_rdp_aad_urlencode(curl, params->client_id);
    char* encoded_scope = guac_rdp_aad_urlencode(curl, params->scope);
    char* encoded_redirect_uri = guac_rdp_aad_urlencode(curl,
            GUAC_AAD_NATIVE_REDIRECT_URI);

    if (!encoded_client_id || !encoded_scope || !encoded_redirect_uri) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to URL-encode authorization parameters");

        if (encoded_client_id)
            curl_free(encoded_client_id);
        if (encoded_scope)
            curl_free(encoded_scope);
        if (encoded_redirect_uri)
            curl_free(encoded_redirect_uri);
        curl_easy_cleanup(curl);
        return 1;
    }

    /* Build authorization URL with query parameters */
    char authorize_url[512];
    snprintf(authorize_url, sizeof(authorize_url),
            GUAC_AAD_AUTHORIZE_ENDPOINT, params->tenant_id);

    int written = snprintf(url_buffer, buffer_size,
            "%s?client_id=%s"
            "&response_type=code"
            "&redirect_uri=%s"
            "&scope=%s"
            "&response_mode=query",
            authorize_url,
            encoded_client_id,
            encoded_redirect_uri,
            encoded_scope);

    curl_free(encoded_client_id);
    curl_free(encoded_scope);
    curl_free(encoded_redirect_uri);
    curl_easy_cleanup(curl);

    if (written < 0 || (size_t) written >= buffer_size) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Authorization URL exceeds buffer size");
        return 1;
    }

    return 0;
}

/**
 * Extracts the authorization code from a redirect URL returned after
 * successful authentication. If the URL contains an error response instead,
 * the error description is logged.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param url
 *     The redirect URL containing either a "code=" parameter on success
 *     or an "error=" parameter on failure.
 *
 * @return
 *     A newly allocated string containing the authorization code, or NULL
 *     if the code could not be extracted. The caller must free the returned
 *     string with guac_mem_free().
 */
static char* guac_rdp_aad_extract_auth_code(guac_client* client,
        const char* url) {

    if (url == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Cannot extract auth code from NULL URL");
        return NULL;
    }

    /* Check for error response in the URL */
    const char* error_pos = strstr(url, "error=");
    if (error_pos != NULL) {
        const char* error_desc = strstr(url, "error_description=");
        if (error_desc != NULL) {
            error_desc += strlen("error_description=");
            const char* error_end = strchr(error_desc, '&');
            size_t error_len = error_end ?
                    (size_t)(error_end - error_desc) : strlen(error_desc);

            if (error_len > 0) {
                char* decoded_error = guac_rdp_percent_decode(error_desc);
                if (decoded_error) {

                    /* Truncate at first & if present */
                    char* separator = strchr(decoded_error, '&');
                    if (separator)
                        *separator = '\0';
                    guac_client_log(client, GUAC_LOG_ERROR,
                            "AAD: Authorization error: %s", decoded_error);
                    guac_mem_free(decoded_error);
                }
            }
        }
        return NULL;
    }

    /* Look for "code=" in the URL */
    const char* code_pos = strstr(url, "code=");
    if (code_pos == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: No authorization code found in redirect URL");
        return NULL;
    }

    code_pos += strlen("code=");

    const char* code_end = strchr(code_pos, '&');
    size_t code_len = code_end ?
            (size_t)(code_end - code_pos) : strlen(code_pos);

    if (code_len == 0) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Empty authorization code in redirect URL");
        return NULL;
    }

    return guac_strndup(code_pos, code_len);
}

/**
 * Exchanges an authorization code for an access token by POSTing to the
 * Azure AD token endpoint.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param params
 *     The AAD authentication parameters containing tenant ID, client ID,
 *     and scope.
 *
 * @param auth_code
 *     The authorization code obtained from the login redirect.
 *
 * @return
 *     A newly allocated string containing the access token, or NULL if
 *     the exchange failed. The caller must free the returned string with
 *     guac_mem_free().
 */
static char* guac_rdp_aad_exchange_code_for_token(guac_client* client,
        guac_rdp_aad_params* params, const char* auth_code) {

    CURL* curl = NULL;
    char* token = NULL;
    char* post_data = NULL;

    curl = curl_easy_init();
    if (curl == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to initialize curl for token exchange");
        return NULL;
    }

    guac_rdp_aad_response* response = guac_rdp_aad_response_alloc();
    if (response == NULL) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    char token_url[512];
    snprintf(token_url, sizeof(token_url), GUAC_AAD_TOKEN_ENDPOINT,
            params->tenant_id);

    guac_client_log(client, GUAC_LOG_DEBUG,
            "AAD: Exchanging authorization code for access token");

    /* URL-encode token exchange parameters */
    char* encoded_client_id = guac_rdp_aad_urlencode(curl, params->client_id);
    char* encoded_code = guac_rdp_aad_urlencode(curl, auth_code);
    char* encoded_redirect_uri = guac_rdp_aad_urlencode(curl,
            GUAC_AAD_NATIVE_REDIRECT_URI);
    char* encoded_scope = guac_rdp_aad_urlencode(curl, params->scope);
    char* encoded_req_cnf = params->req_cnf ?
            guac_rdp_aad_urlencode(curl, params->req_cnf) : NULL;

    if (!encoded_client_id || !encoded_code || !encoded_redirect_uri
            || !encoded_scope) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to URL-encode token exchange parameters");
        goto cleanup;
    }

    /* Build token exchange POST body */
    size_t post_data_size = 1024
            + strlen(encoded_client_id) + strlen(encoded_code)
            + strlen(encoded_redirect_uri) + strlen(encoded_scope)
            + (encoded_req_cnf ? strlen(encoded_req_cnf) : 0);

    post_data = guac_mem_alloc(post_data_size);

    int written = snprintf(post_data, post_data_size,
            "grant_type=authorization_code"
            "&client_id=%s"
            "&code=%s"
            "&redirect_uri=%s"
            "&scope=%s",
            encoded_client_id,
            encoded_code,
            encoded_redirect_uri,
            encoded_scope);

    /* Append req_cnf (Proof-of-Possession) if provided by FreeRDP */
    if (encoded_req_cnf && written > 0
            && (size_t) written < post_data_size - 1) {
        snprintf(post_data + written, post_data_size - written,
                "&req_cnf=%s", encoded_req_cnf);
    }

    /* Configure and send the token request */
    curl_easy_setopt(curl, CURLOPT_URL, token_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            guac_rdp_aad_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, GUAC_AAD_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,
            (long) GUAC_AAD_HTTP_TIMEOUT_SECONDS);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Token exchange HTTP request failed: %s",
                curl_easy_strerror(res));
        goto cleanup;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Token exchange failed with HTTP %ld", http_code);
    }

    /* Parse access token from response */
    token = guac_rdp_aad_parse_token_response(client, response->data);

cleanup:
    if (headers)
        curl_slist_free_all(headers);
    if (encoded_client_id)
        curl_free(encoded_client_id);
    if (encoded_code)
        curl_free(encoded_code);
    if (encoded_redirect_uri)
        curl_free(encoded_redirect_uri);
    if (encoded_scope)
        curl_free(encoded_scope);
    if (encoded_req_cnf)
        curl_free(encoded_req_cnf);

    guac_mem_free(post_data);
    curl_easy_cleanup(curl);
    guac_rdp_aad_response_free(response);

    return token;
}

/**
 * Calls the Microsoft GetCredentialType API to update server-side session
 * state and obtain a fresh flow token for credential submission. Without
 * this intermediate call, the credential POST returns a ConvergedError.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param curl
 *     An initialized CURL handle with cookies enabled.
 *
 * @param params
 *     The AAD authentication parameters (username, tenant_id, etc.).
 *
 * @param auth_url
 *     The original authorization URL, used as the Referer header.
 *
 * @param flow_token
 *     Pointer to the current flow token string. On success, the old token
 *     is freed and replaced with the updated token from the API response.
 *
 * @param ctx
 *     The session context value from the login page $Config.
 *
 * @param api_canary
 *     The API canary token from the login page, or NULL if not available.
 */
static void guac_rdp_aad_get_credential_type(guac_client* client,
        CURL* curl, guac_rdp_aad_params* params, const char* auth_url,
        char** flow_token, const char* ctx, const char* api_canary) {

    guac_client_log(client, GUAC_LOG_DEBUG,
            "AAD: Calling GetCredentialType API");

    char gct_url[512];
    snprintf(gct_url, sizeof(gct_url),
            "https://login.microsoftonline.com/%s/GetCredentialType?mkt=en",
            params->tenant_id);

    /* Build GetCredentialType JSON request body */
    size_t gct_body_size = 256 + strlen(*flow_token) + strlen(ctx)
            + strlen(params->username);
    char* gct_body = guac_mem_alloc(gct_body_size);
    snprintf(gct_body, gct_body_size,
            "{\"username\":\"%s\","
            "\"originalRequest\":\"%s\","
            "\"flowToken\":\"%s\"}",
            params->username, ctx, *flow_token);

    /* Set required headers */
    struct curl_slist* gct_headers = NULL;
    gct_headers = curl_slist_append(gct_headers,
            "Content-Type: application/json");
    gct_headers = curl_slist_append(gct_headers,
            "Origin: https://login.microsoftonline.com");

    /* Add API canary as header if available */
    if (api_canary != NULL) {
        char canary_header[4096];
        snprintf(canary_header, sizeof(canary_header),
                "canary: %s", api_canary);
        gct_headers = curl_slist_append(gct_headers, canary_header);
    }

    char referer_header[2048];
    snprintf(referer_header, sizeof(referer_header), "Referer: %s", auth_url);
    gct_headers = curl_slist_append(gct_headers, referer_header);

    guac_rdp_aad_response* gct_response = guac_rdp_aad_response_alloc();
    if (gct_response == NULL) {
        curl_slist_free_all(gct_headers);
        guac_mem_free(gct_body);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, gct_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, gct_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, gct_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, gct_response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(gct_headers);

    if (res == CURLE_OK && gct_response->data != NULL) {

        /* Extract the updated flow token from the JSON response */
        const char* flow_token_key = "\"FlowToken\":\"";
        const char* flow_token_start = strstr(gct_response->data,
                flow_token_key);
        if (flow_token_start != NULL) {
            flow_token_start += strlen(flow_token_key);
            const char* flow_token_end = strchr(flow_token_start, '"');
            if (flow_token_end != NULL) {
                guac_mem_free(*flow_token);
                size_t flow_token_len = flow_token_end - flow_token_start;
                *flow_token = guac_strndup(flow_token_start, flow_token_len);
            }
        }
    }

    /* GetCredentialType call failed */
    else {
        guac_client_log(client, GUAC_LOG_WARNING,
                "AAD: GetCredentialType call failed, continuing with "
                "original flow token");
    }

    guac_rdp_aad_response_free(gct_response);
    guac_mem_free(gct_body);
}

/**
 * Performs the full automated browser-based login flow against the Microsoft
 * login endpoint. Fetches the login page, parses session tokens from $Config,
 * calls GetCredentialType, and posts credentials to obtain an authorization
 * code.
 *
 * @param client
 *     The guac_client associated with the current RDP connection.
 *
 * @param auth_url
 *     The full authorization URL to start the login flow.
 *
 * @param params
 *     The AAD authentication parameters including username and password.
 *
 * @return
 *     A newly allocated string containing the authorization code, or NULL
 *     if login failed. The caller must free the returned string with
 *     guac_mem_free().
 */
static char* guac_rdp_aad_automated_login(guac_client* client,
        const char* auth_url, guac_rdp_aad_params* params) {

    char* auth_code = NULL;
    CURL* curl = NULL;
    char* flow_token = NULL;
    char* ctx = NULL;
    char* post_url = NULL;
    char* canary = NULL;
    char* post_data = NULL;

    curl = curl_easy_init();
    if (curl == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to initialize curl for automated login");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, GUAC_AAD_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,
            (long) GUAC_AAD_HTTP_TIMEOUT_SECONDS);

    /* Step 1: GET the authorization URL to get the login page */

    guac_client_log(client, GUAC_LOG_DEBUG,
            "AAD: Fetching login page from authorization URL");

    guac_rdp_aad_response* login_page = guac_rdp_aad_response_alloc();
    if (login_page == NULL)
        goto cleanup;

    curl_easy_setopt(curl, CURLOPT_URL, auth_url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            guac_rdp_aad_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, login_page);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to fetch login page: %s",
                curl_easy_strerror(res));
        goto cleanup;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Login page returned HTTP %ld", http_code);
        goto cleanup;
    }

    /* Step 2: Parse $Config from the login page HTML */

    guac_client_log(client, GUAC_LOG_DEBUG,
            "AAD: Parsing $Config from login page (%zu bytes)",
            login_page->size);

    flow_token = guac_rdp_aad_extract_config_value(login_page->data, "sFT");
    ctx = guac_rdp_aad_extract_config_value(login_page->data, "sCtx");
    post_url = guac_rdp_aad_extract_config_value(login_page->data, "urlPost");
    canary = guac_rdp_aad_extract_config_value(login_page->data, "canary");

    /* Extract API canary (used for JSON API calls like GetCredentialType) */
    char* api_canary = guac_rdp_aad_extract_config_value(login_page->data,
            "apiCanary");

    if (flow_token == NULL || ctx == NULL || post_url == NULL
            || canary == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to parse login page $Config "
                "(sFT=%s, sCtx=%s, urlPost=%s, canary=%s)",
                flow_token ? "found" : "MISSING",
                ctx ? "found" : "MISSING",
                post_url ? "found" : "MISSING",
                canary ? "found" : "MISSING");
        guac_mem_free(api_canary);
        goto cleanup;
    }

    /* Update server-side session state and get a fresh flow token */
    guac_rdp_aad_get_credential_type(client, curl, params, auth_url,
            &flow_token, ctx, api_canary);
    guac_mem_free(api_canary);

    /* Step 3: POST credentials */

    guac_client_log(client, GUAC_LOG_DEBUG,
            "AAD: Posting credentials to login endpoint");

    /* URL-encode credential parameters */
    char* encoded_login = guac_rdp_aad_urlencode(curl, params->username);
    char* encoded_passwd = guac_rdp_aad_urlencode(curl, params->password);
    char* encoded_ctx = guac_rdp_aad_urlencode(curl, ctx);
    char* encoded_flowtoken = guac_rdp_aad_urlencode(curl, flow_token);
    char* encoded_canary = guac_rdp_aad_urlencode(curl, canary);

    if (!encoded_login || !encoded_passwd || !encoded_ctx
            || !encoded_flowtoken || !encoded_canary) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to URL-encode login credentials");
        if (encoded_login)
            curl_free(encoded_login);
        if (encoded_passwd)
            curl_free(encoded_passwd);
        if (encoded_ctx)
            curl_free(encoded_ctx);
        if (encoded_flowtoken)
            curl_free(encoded_flowtoken);
        if (encoded_canary)
            curl_free(encoded_canary);
        goto cleanup;
    }

    size_t post_data_size = 1024
            + (strlen(encoded_login) * 2)
            + strlen(encoded_passwd)
            + strlen(encoded_ctx) + strlen(encoded_flowtoken)
            + strlen(encoded_canary);

    post_data = guac_mem_alloc(post_data_size);

    /* Build credential POST body. Both "login" and "loginfmt" are required
     * by Microsoft. The canary, ctx, and flowtoken are CSRF/session tokens
     * from the login page $Config. type=11 indicates password auth. */
    snprintf(post_data, post_data_size,
            "login=%s"
            "&loginfmt=%s"
            "&passwd=%s"
            "&canary=%s"
            "&ctx=%s"
            "&flowtoken=%s"
            "&type=11",
            encoded_login,
            encoded_login,
            encoded_passwd,
            encoded_canary,
            encoded_ctx,
            encoded_flowtoken);

    curl_free(encoded_login);
    curl_free(encoded_passwd);
    curl_free(encoded_ctx);
    curl_free(encoded_flowtoken);
    curl_free(encoded_canary);

    /* Reset response buffer for the credential POST */
    guac_rdp_aad_response_free(login_page);
    login_page = guac_rdp_aad_response_alloc();
    if (login_page == NULL)
        goto cleanup;

    /* Configure credential POST request */
    curl_easy_setopt(curl, CURLOPT_URL, post_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, login_page);

    /* Set headers that the browser normally sends. Microsoft's login endpoint
     * checks Origin and Referer for CSRF protection beyond the canary token. */
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers,
            "Origin: https://login.microsoftonline.com");

    char referer_header[2048];
    snprintf(referer_header, sizeof(referer_header), "Referer: %s", auth_url);
    headers = curl_slist_append(headers, referer_header);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);

    if (headers)
        curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Credential POST failed: %s",
                curl_easy_strerror(res));
        goto cleanup;
    }

    /* Step 4: Check the result of the credential POST */

    char* effective_url = NULL;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

    guac_client_log(client, GUAC_LOG_DEBUG,
            "AAD: Credential POST redirected to: %s",
            effective_url ? effective_url : "(NULL)");

    if (effective_url != NULL
            && strncmp(effective_url, GUAC_AAD_NATIVE_REDIRECT_URI,
                       strlen(GUAC_AAD_NATIVE_REDIRECT_URI)) == 0) {

        auth_code = guac_rdp_aad_extract_auth_code(client, effective_url);
    }

    /* Credential POST did not redirect to the native client URI */
    else {

        /* Log any error from the effective URL */
        if (effective_url != NULL
                && strstr(effective_url, "error=") != NULL)
            guac_rdp_aad_extract_auth_code(client, effective_url);

        /* Check for error code in the response body */
        if (login_page->data != NULL) {

            char* error_code = guac_rdp_aad_extract_config_value(
                    login_page->data, "sErrorCode");

            if (error_code != NULL && strlen(error_code) > 0
                    && strcmp(error_code, "0") != 0) {
                guac_client_log(client, GUAC_LOG_ERROR,
                        "AAD: Login failed with error code: %s",
                        error_code);
            }

            guac_mem_free(error_code);
        }

        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Automated login failed - did not reach "
                "redirect URI");
    }

cleanup:
    guac_mem_free(flow_token);
    guac_mem_free(ctx);
    guac_mem_free(post_url);
    guac_mem_free(canary);
    guac_mem_free(post_data);

    guac_rdp_aad_response_free(login_page);
    curl_easy_cleanup(curl);

    return auth_code;
}

char* guac_rdp_aad_get_token_authcode(guac_client* client,
        guac_rdp_aad_params* params) {

    /* Require client_id, tenant_id, username, password, and scope */
    if (params == NULL || params->client_id == NULL
            || params->tenant_id == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Missing required parameters (client_id and "
                "tenant_id) for authorization code flow");
        return NULL;
    }

    if (params->username == NULL || params->password == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Username and password are required for "
                "authorization code flow");
        return NULL;
    }

    if (params->scope == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Scope is required for authorization code flow");
        return NULL;
    }

    /* Step 1: Build the authorization URL */
    char auth_url[2048];
    if (guac_rdp_aad_build_auth_url(client, params,
            auth_url, sizeof(auth_url)) != 0) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to build authorization URL");
        return NULL;
    }

    /* Step 2: Automated login to get the authorization code */
    guac_client_log(client, GUAC_LOG_INFO,
            "AAD: Starting automated authorization code flow "
            "for user: %s", params->username);

    char* auth_code = guac_rdp_aad_automated_login(client, auth_url, params);

    if (auth_code == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "AAD: Failed to obtain authorization code");
        return NULL;
    }

    /* Step 3: Exchange the code for an access token */
    char* token = guac_rdp_aad_exchange_code_for_token(client, params,
            auth_code);

    guac_mem_free(auth_code);

    return token;
}

#endif /* HAVE_FREERDP_AAD_SUPPORT */
