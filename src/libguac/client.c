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

#include "encode-jpeg.h"
#include "encode-png.h"
#include "encode-webp.h"
#include "guacamole/client.h"
#include "guacamole/error.h"
#include "guacamole/layer.h"
#include "guacamole/plugin.h"
#include "guacamole/pool.h"
#include "guacamole/protocol.h"
#include "guacamole/socket.h"
#include "guacamole/stream.h"
#include "guacamole/string.h"
#include "guacamole/timestamp.h"
#include "guacamole/user.h"
#include "id.h"

#include <dlfcn.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Empty NULL-terminated array of argument names.
 */
const char* __GUAC_CLIENT_NO_ARGS[] = { NULL };

guac_layer __GUAC_DEFAULT_LAYER = {
    .index = 0
};

const guac_layer* GUAC_DEFAULT_LAYER = &__GUAC_DEFAULT_LAYER;

guac_layer* guac_client_alloc_layer(guac_client* client) {

    /* Init new layer */
    guac_layer* allocd_layer = malloc(sizeof(guac_layer));
    allocd_layer->index = guac_pool_next_int(client->__layer_pool)+1;

    return allocd_layer;

}

guac_layer* guac_client_alloc_buffer(guac_client* client) {

    /* Init new layer */
    guac_layer* allocd_layer = malloc(sizeof(guac_layer));
    allocd_layer->index = -guac_pool_next_int(client->__buffer_pool) - 1;

    return allocd_layer;

}

void guac_client_free_buffer(guac_client* client, guac_layer* layer) {

    /* Release index to pool */
    guac_pool_free_int(client->__buffer_pool, -layer->index - 1);

    /* Free layer */
    free(layer);

}

void guac_client_free_layer(guac_client* client, guac_layer* layer) {

    /* Release index to pool */
    guac_pool_free_int(client->__layer_pool, layer->index);

    /* Free layer */
    free(layer);

}

guac_stream* guac_client_alloc_stream(guac_client* client) {

    guac_stream* allocd_stream;
    int stream_index;

    /* Refuse to allocate beyond maximum */
    if (client->__stream_pool->active == GUAC_CLIENT_MAX_STREAMS)
        return NULL;

    /* Allocate stream */
    stream_index = guac_pool_next_int(client->__stream_pool);

    /* Initialize stream with odd index (even indices are user-level) */
    allocd_stream = &(client->__output_streams[stream_index]);
    allocd_stream->index = (stream_index * 2) + 1;
    allocd_stream->data = NULL;
    allocd_stream->ack_handler = NULL;
    allocd_stream->blob_handler = NULL;
    allocd_stream->end_handler = NULL;

    return allocd_stream;

}

void guac_client_free_stream(guac_client* client, guac_stream* stream) {

    /* Release index to pool */
    guac_pool_free_int(client->__stream_pool, (stream->index - 1) / 2);

    /* Mark stream as closed */
    stream->index = GUAC_CLIENT_CLOSED_STREAM_INDEX;

}

guac_client* guac_client_alloc() {

    int i;
    pthread_rwlockattr_t lock_attributes;

    /* Allocate new client */
    guac_client* client = malloc(sizeof(guac_client));
    if (client == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for client";
        return NULL;
    }

    /* Init new client */
    memset(client, 0, sizeof(guac_client));

    client->args = __GUAC_CLIENT_NO_ARGS;
    client->state = GUAC_CLIENT_RUNNING;
    client->last_sent_timestamp = guac_timestamp_current();

    /* Generate ID */
    client->connection_id = guac_generate_id(GUAC_CLIENT_ID_PREFIX);
    if (client->connection_id == NULL) {
        free(client);
        return NULL;
    }

    /* Allocate buffer and layer pools */
    client->__buffer_pool = guac_pool_alloc(GUAC_BUFFER_POOL_INITIAL_SIZE);
    client->__layer_pool = guac_pool_alloc(GUAC_BUFFER_POOL_INITIAL_SIZE);

    /* Allocate stream pool */
    client->__stream_pool = guac_pool_alloc(0);

    /* Initialize streams */
    client->__output_streams = malloc(sizeof(guac_stream) * GUAC_CLIENT_MAX_STREAMS);

    for (i=0; i<GUAC_CLIENT_MAX_STREAMS; i++) {
        client->__output_streams[i].index = GUAC_CLIENT_CLOSED_STREAM_INDEX;
    }


    /* Init locks */
    pthread_rwlockattr_init(&lock_attributes);
    pthread_rwlockattr_setpshared(&lock_attributes, PTHREAD_PROCESS_SHARED);

    pthread_rwlock_init(&(client->__users_lock), &lock_attributes);

    /* Set up socket to broadcast to all users */
    client->socket = guac_socket_broadcast(client);

    return client;

}

void guac_client_free(guac_client* client) {

    /* Remove all users */
    while (client->__users != NULL)
        guac_client_remove_user(client, client->__users);

    if (client->free_handler) {

        /* FIXME: Errors currently ignored... */
        client->free_handler(client);

    }

    /* Free socket */
    guac_socket_free(client->socket);

    /* Free layer pools */
    guac_pool_free(client->__buffer_pool);
    guac_pool_free(client->__layer_pool);

    /* Free streams */
    free(client->__output_streams);

    /* Free stream pool */
    guac_pool_free(client->__stream_pool);

    /* Close associated plugin */
    if (client->__plugin_handle != NULL) {
        if (dlclose(client->__plugin_handle))
            guac_client_log(client, GUAC_LOG_ERROR, "Unable to close plugin: %s", dlerror());
    }

    pthread_rwlock_destroy(&(client->__users_lock));
    free(client->connection_id);
    free(client);
}

void vguac_client_log(guac_client* client, guac_client_log_level level,
        const char* format, va_list ap) {

    /* Call handler if defined */
    if (client->log_handler != NULL)
        client->log_handler(client, level, format, ap);

}

void guac_client_log(guac_client* client, guac_client_log_level level,
        const char* format, ...) {

    va_list args;
    va_start(args, format);

    vguac_client_log(client, level, format, args);

    va_end(args);

}

void guac_client_stop(guac_client* client) {
    client->state = GUAC_CLIENT_STOPPING;
}

void vguac_client_abort(guac_client* client, guac_protocol_status status,
        const char* format, va_list ap) {

    /* Only relevant if client is running */
    if (client->state == GUAC_CLIENT_RUNNING) {

        /* Log detail of error */
        vguac_client_log(client, GUAC_LOG_ERROR, format, ap);

        /* Send error immediately, limit information given */
        guac_protocol_send_error(client->socket, "Aborted. See logs.", status);
        guac_socket_flush(client->socket);

        /* Stop client */
        guac_client_stop(client);

    }

}

void guac_client_abort(guac_client* client, guac_protocol_status status,
        const char* format, ...) {

    va_list args;
    va_start(args, format);

    vguac_client_abort(client, status, format, args);

    va_end(args);

}

int guac_client_add_user(guac_client* client, guac_user* user, int argc, char** argv) {

    int retval = 0;

    /* Call handler, if defined */
    if (client->join_handler)
        retval = client->join_handler(user, argc, argv);

    pthread_rwlock_wrlock(&(client->__users_lock));

    /* Add to list if join was successful */
    if (retval == 0) {

        user->__prev = NULL;
        user->__next = client->__users;

        if (client->__users != NULL)
            client->__users->__prev = user;

        client->__users = user;
        client->connected_users++;

        /* Update owner pointer if user is owner */
        if (user->owner)
            client->__owner = user;

    }

    pthread_rwlock_unlock(&(client->__users_lock));

    return retval;

}

void guac_client_remove_user(guac_client* client, guac_user* user) {

    pthread_rwlock_wrlock(&(client->__users_lock));

    /* Update prev / head */
    if (user->__prev != NULL)
        user->__prev->__next = user->__next;
    else
        client->__users = user->__next;

    /* Update next */
    if (user->__next != NULL)
        user->__next->__prev = user->__prev;

    client->connected_users--;

    /* Update owner pointer if user was owner */
    if (user->owner)
        client->__owner = NULL;

    pthread_rwlock_unlock(&(client->__users_lock));

    /* Call handler, if defined */
    if (user->leave_handler)
        user->leave_handler(user);
    else if (client->leave_handler)
        client->leave_handler(user);

}

void guac_client_foreach_user(guac_client* client, guac_user_callback* callback, void* data) {

    guac_user* current;

    pthread_rwlock_rdlock(&(client->__users_lock));

    /* Call function on each user */
    current = client->__users;
    while (current != NULL) {
        callback(current, data);
        current = current->__next;
    }

    pthread_rwlock_unlock(&(client->__users_lock));

}

void* guac_client_for_owner(guac_client* client, guac_user_callback* callback,
        void* data) {

    void* retval;

    pthread_rwlock_rdlock(&(client->__users_lock));

    /* Invoke callback with current owner */
    retval = callback(client->__owner, data);

    pthread_rwlock_unlock(&(client->__users_lock));

    /* Return value from callback */
    return retval;

}

void* guac_client_for_user(guac_client* client, guac_user* user,
        guac_user_callback* callback, void* data) {

    guac_user* current;

    int user_valid = 0;
    void* retval;

    pthread_rwlock_rdlock(&(client->__users_lock));

    /* Loop through all users, searching for a pointer to the given user */
    current = client->__users;
    while (current != NULL) {

        /* If the user's pointer exists in the list, they are indeed valid */
        if (current == user) {
            user_valid = 1;
            break;
        }

        current = current->__next;
    }

    /* Use NULL if user does not actually exist */
    if (!user_valid)
        user = NULL;

    /* Invoke callback with requested user (if they exist) */
    retval = callback(user, data);

    pthread_rwlock_unlock(&(client->__users_lock));

    /* Return value from callback */
    return retval;

}

int guac_client_end_frame(guac_client* client) {

    /* Update and send timestamp */
    client->last_sent_timestamp = guac_timestamp_current();

    /* Log received timestamp and calculated lag (at TRACE level only) */
    guac_client_log(client, GUAC_LOG_TRACE, "Server completed "
            "frame %" PRIu64 "ms.", client->last_sent_timestamp);

    return guac_protocol_send_sync(client->socket, client->last_sent_timestamp);

}

int guac_client_load_plugin(guac_client* client, const char* protocol) {

    /* Reference to dlopen()'d plugin */
    void* client_plugin_handle;

    /* Pluggable client */
    char protocol_lib[GUAC_PROTOCOL_LIBRARY_LIMIT] =
        GUAC_PROTOCOL_LIBRARY_PREFIX;

    /* Type-pun for the sake of dlsym() - cannot typecast a void* to a function
     * pointer otherwise */ 
    union {
        guac_client_init_handler* client_init;
        void* obj;
    } alias;

    /* Add protocol and .so suffix to protocol_lib */
    guac_strlcat(protocol_lib, protocol, sizeof(protocol_lib));
    if (guac_strlcat(protocol_lib, GUAC_PROTOCOL_LIBRARY_SUFFIX,
                sizeof(protocol_lib)) >= sizeof(protocol_lib)) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Protocol name is too long";
        return -1;
    }

    /* Load client plugin */
    client_plugin_handle = dlopen(protocol_lib, RTLD_LAZY);
    if (!client_plugin_handle) {
        guac_error = GUAC_STATUS_NOT_FOUND;
        guac_error_message = dlerror();
        return -1;
    }

    dlerror(); /* Clear errors */

    /* Get init function */
    alias.obj = dlsym(client_plugin_handle, "guac_client_init");

    /* Fail if cannot find guac_client_init */
    if (dlerror() != NULL) {
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = dlerror();
        dlclose(client_plugin_handle);
        return -1;
    }

    /* Init client */
    client->__plugin_handle = client_plugin_handle;

    return alias.client_init(client);

}

/**
 * A callback function which is invoked by guac_client_owner_send_required() to
 * send the required parameters to the specified user, who is the owner of the
 * client session.
 * 
 * @param user
 *     The guac_user that will receive the required parameters, who is the owner
 *     of the client.
 * 
 * @param data
 *     A pointer to a NULL-terminated array of required parameters that will be
 *     passed on to the owner to continue the connection.
 * 
 * @return
 *     Zero if the operation succeeds or non-zero on failure, cast as a void*.
 */
static void* guac_client_owner_send_required_callback(guac_user* user, void* data) {
    
    const char** required = (const char **) data;
    
    /* Send required parameters to owner. */
    if (user != NULL)
        return (void*) ((intptr_t) guac_protocol_send_required(user->socket, required));
    
    return (void*) ((intptr_t) -1);
    
}

int guac_client_owner_send_required(guac_client* client, const char** required) {

    /* Don't send required instruction if client does not support it. */
    if (!guac_client_owner_supports_required(client))
        return -1;
    
    return (int) ((intptr_t) guac_client_for_owner(client, guac_client_owner_send_required_callback, required));

}

/**
 * Updates the provided approximate processing lag, taking into account the
 * processing lag of the given user.
 *
 * @param user
 *     The guac_user to use to update the approximate processing lag.
 *
 * @param data
 *     Pointer to an int containing the current approximate processing lag.
 *     The int will be updated according to the processing lag of the given
 *     user.
 *
 * @return
 *     Always NULL.
 */
static void* __calculate_lag(guac_user* user, void* data) {

    int* processing_lag = (int*) data;

    /* Simply find maximum */
    if (user->processing_lag > *processing_lag)
        *processing_lag = user->processing_lag;

    return NULL;

}

int guac_client_get_processing_lag(guac_client* client) {

    int processing_lag = 0;

    /* Approximate the processing lag of all users */
    guac_client_foreach_user(client, __calculate_lag, &processing_lag);

    return processing_lag;

}

void guac_client_stream_argv(guac_client* client, guac_socket* socket,
        const char* mimetype, const char* name, const char* value) {

    /* Allocate new stream for argument value */
    guac_stream* stream = guac_client_alloc_stream(client);

    /* Declare stream as containing connection parameter data */
    guac_protocol_send_argv(socket, stream, mimetype, name);

    /* Write parameter data */
    guac_protocol_send_blobs(socket, stream, value, strlen(value));

    /* Terminate stream */
    guac_protocol_send_end(socket, stream);

    /* Free allocated stream */
    guac_client_free_stream(client, stream);

}

void guac_client_stream_png(guac_client* client, guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer, int x, int y,
        cairo_surface_t* surface) {

    /* Allocate new stream for image */
    guac_stream* stream = guac_client_alloc_stream(client);

    /* Declare stream as containing image data */
    guac_protocol_send_img(socket, stream, mode, layer, "image/png", x, y);

    /* Write PNG data */
    guac_png_write(socket, stream, surface);

    /* Terminate stream */
    guac_protocol_send_end(socket, stream);

    /* Free allocated stream */
    guac_client_free_stream(client, stream);

}

void guac_client_stream_jpeg(guac_client* client, guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer, int x, int y,
        cairo_surface_t* surface, int quality) {

    /* Allocate new stream for image */
    guac_stream* stream = guac_client_alloc_stream(client);

    /* Declare stream as containing image data */
    guac_protocol_send_img(socket, stream, mode, layer, "image/jpeg", x, y);

    /* Write JPEG data */
    guac_jpeg_write(socket, stream, surface, quality);

    /* Terminate stream */
    guac_protocol_send_end(socket, stream);

    /* Free allocated stream */
    guac_client_free_stream(client, stream);

}

void guac_client_stream_webp(guac_client* client, guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer, int x, int y,
        cairo_surface_t* surface, int quality, int lossless) {

#ifdef ENABLE_WEBP
    /* Allocate new stream for image */
    guac_stream* stream = guac_client_alloc_stream(client);

    /* Declare stream as containing image data */
    guac_protocol_send_img(socket, stream, mode, layer, "image/webp", x, y);

    /* Write WebP data */
    guac_webp_write(socket, stream, surface, quality, lossless);

    /* Terminate stream */
    guac_protocol_send_end(socket, stream);

    /* Free allocated stream */
    guac_client_free_stream(client, stream);
#else
    /* Do nothing if WebP support is not built in */
#endif

}

#ifdef ENABLE_WEBP
/**
 * Callback which is invoked by guac_client_supports_webp() for each user
 * associated with the given client, thus updating an overall support flag
 * describing the WebP support state for the client as a whole.
 *
 * @param user
 *     The user to check for WebP support.
 *
 * @param data
 *     Pointer to an int containing the current WebP support status for the
 *     client associated with the given user. This flag will be 0 if any user
 *     already checked has lacked WebP support, or 1 otherwise.
 *
 * @return
 *     Always NULL.
 */
static void* __webp_support_callback(guac_user* user, void* data) {

    int* webp_supported = (int*) data;

    /* Check whether current user supports WebP */
    if (*webp_supported)
        *webp_supported = guac_user_supports_webp(user);

    return NULL;

}
#endif

/**
 * A callback function which is invoked by guac_client_owner_supports_required()
 * to determine if the owner of a client supports the "required" instruction,
 * returning zero if the user does not support the instruction or non-zero if
 * the user supports it.
 * 
 * @param user
 *     The guac_user that will be checked for "required" instruction support.
 * 
 * @param data
 *     Data provided to the callback. This value is never used within this
 *     callback.
 * 
 * @return
 *     A non-zero integer if the provided user who owns the connection supports
 *     the "required" instruction, or zero if the user does not. The integer
 *     is cast as a void*.
 */
static void* guac_owner_supports_required_callback(guac_user* user, void* data) {
    
    return (void*) ((intptr_t) guac_user_supports_required(user));
    
}

int guac_client_owner_supports_required(guac_client* client) {
    
    return (int) ((intptr_t) guac_client_for_owner(client, guac_owner_supports_required_callback, NULL));
    
}

int guac_client_supports_webp(guac_client* client) {

#ifdef ENABLE_WEBP
    int webp_supported = 1;

    /* WebP is supported for entire client only if each user supports it */
    guac_client_foreach_user(client, __webp_support_callback, &webp_supported);

    return webp_supported;
#else
    /* Support for WebP is completely absent */
    return 0;
#endif

}

