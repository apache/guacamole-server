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

#ifndef _GUAC_CLIENT_H
#define _GUAC_CLIENT_H

/**
 * Functions and structure contents for the Guacamole proxy client.
 *
 * @file client.h
 */

#include "client-fntypes.h"
#include "client-types.h"
#include "client-constants.h"
#include "instruction-types.h"
#include "layer-types.h"
#include "object-types.h"
#include "pool-types.h"
#include "socket-types.h"
#include "stream-types.h"
#include "timestamp-types.h"

#include <cairo/cairo.h>
#include <stdarg.h>

struct guac_client_info {

    /**
     * The number of pixels the remote client requests for the display width.
     * This need not be honored by a client plugin implementation, but if the
     * underlying protocol of the client plugin supports dynamic sizing of the
     * screen, honoring the display size request is recommended.
     */
    int optimal_width;

    /**
     * The number of pixels the remote client requests for the display height.
     * This need not be honored by a client plugin implementation, but if the
     * underlying protocol of the client plugin supports dynamic sizing of the
     * screen, honoring the display size request is recommended.
     */
    int optimal_height;

    /**
     * NULL-terminated array of client-supported audio mimetypes. If the client
     * does not support audio at all, this will be NULL.
     */
    const char** audio_mimetypes;

    /**
     * NULL-terminated array of client-supported video mimetypes. If the client
     * does not support video at all, this will be NULL.
     */
    const char** video_mimetypes;

    /**
     * NULL-terminated array of client-supported image mimetypes. Though all
     * supported image mimetypes will be listed here, it can be safely assumed
     * that all clients will support at least "image/png" and "image/jpeg".
     */ 
    const char** image_mimetypes;

    /**
     * The DPI of the physical remote display if configured for the optimal
     * width/height combination described here. This need not be honored by
     * a client plugin implementation, but if the underlying protocol of the
     * client plugin supports dynamic sizing of the screen, honoring the
     * stated resolution of the display size request is recommended.
     */
    int optimal_resolution;

};

struct guac_client {

    /**
     * The guac_socket structure to be used to communicate with the web-client.
     * It is expected that the implementor of any Guacamole proxy client will
     * provide their own mechanism of I/O for their protocol. The guac_socket
     * structure is used only to communicate conveniently with the Guacamole
     * web-client.
     */
    guac_socket* socket;

    /**
     * The current state of the client. When the client is first allocated,
     * this will be initialized to GUAC_CLIENT_RUNNING. It will remain at
     * GUAC_CLIENT_RUNNING until an event occurs which requires the client to
     * shutdown, at which point the state becomes GUAC_CLIENT_STOPPING.
     */
    guac_client_state state;

    /**
     * The time (in milliseconds) of receipt of the last sync message from
     * the client.
     */
    guac_timestamp last_received_timestamp;

    /**
     * The time (in milliseconds) that the last sync message was sent to the
     * client.
     */
    guac_timestamp last_sent_timestamp;

    /**
     * Information structure containing properties exposed by the remote
     * client during the initial handshake process.
     */
    guac_client_info info;

    /**
     * Arbitrary reference to proxy client-specific data. Implementors of a
     * Guacamole proxy client can store any data they want here, which can then
     * be retrieved as necessary in the message handlers.
     */
    void* data;

    /**
     * Handler for server messages. If set, this function will be called
     * occasionally by the Guacamole proxy to give the client a chance to
     * handle messages from whichever server it is connected to.
     *
     * Example:
     * @code
     *     int handle_messages(guac_client* client);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->handle_messages = handle_messages;
     *     }
     * @endcode
     */
    guac_client_handle_messages* handle_messages;

    /**
     * Handler for mouse events sent by the Gaucamole web-client.
     *
     * The handler takes the integer mouse X and Y coordinates, as well as
     * a button mask containing the bitwise OR of all button values currently
     * being pressed. Those values are:
     *
     * <table>
     *     <tr><th>Button</th>          <th>Value</th></tr>
     *     <tr><td>Left</td>            <td>1</td></tr>
     *     <tr><td>Middle</td>          <td>2</td></tr>
     *     <tr><td>Right</td>           <td>4</td></tr>
     *     <tr><td>Scrollwheel Up</td>  <td>8</td></tr>
     *     <tr><td>Scrollwheel Down</td><td>16</td></tr>
     * </table>

     * Example:
     * @code
     *     int mouse_handler(guac_client* client, int x, int y, int button_mask);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->mouse_handler = mouse_handler;
     *     }
     * @endcode
     */
    guac_client_mouse_handler* mouse_handler;

    /**
     * Handler for key events sent by the Guacamole web-client.
     *
     * The handler takes the integer X11 keysym associated with the key
     * being pressed/released, and an integer representing whether the key
     * is being pressed (1) or released (0).
     *
     * Example:
     * @code
     *     int key_handler(guac_client* client, int keysym, int pressed);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->key_handler = key_handler;
     *     }
     * @endcode
     */
    guac_client_key_handler* key_handler;

    /**
     * Handler for clipboard events sent by the Guacamole web-client. This
     * handler will be called whenever the web-client sets the data of the
     * clipboard.
     *
     * The handler takes a guac_stream, which contains the stream index and
     * will persist through the duration of the transfer, and the mimetype
     * of the data being transferred.
     *
     * Example:
     * @code
     *     int clipboard_handler(guac_client* client, guac_stream* stream,
     *             char* mimetype);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->clipboard_handler = clipboard_handler;
     *     }
     * @endcode
     */
    guac_client_clipboard_handler* clipboard_handler;

    /**
     * Handler for size events sent by the Guacamole web-client.
     *
     * The handler takes an integer width and integer height, representing
     * the current visible screen area of the client.
     *
     * Example:
     * @code
     *     int size_handler(guac_client* client, int width, int height);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->size_handler = size_handler;
     *     }
     * @endcode
     */
    guac_client_size_handler* size_handler;

    /**
     * Handler for file events sent by the Guacamole web-client.
     *
     * The handler takes a guac_stream which contains the stream index and
     * will persist through the duration of the transfer, the mimetype of
     * the file being transferred, and the filename.
     *
     * Example:
     * @code
     *     int file_handler(guac_client* client, guac_stream* stream,
     *             char* mimetype, char* filename);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->file_handler = file_handler;
     *     }
     * @endcode
     */
    guac_client_file_handler* file_handler;

    /**
     * Handler for pipe events sent by the Guacamole web-client.
     *
     * The handler takes a guac_stream which contains the stream index and
     * will persist through the duration of the transfer, the mimetype of
     * the data being transferred, and the pipe name.
     *
     * Example:
     * @code
     *     int pipe_handler(guac_client* client, guac_stream* stream,
     *             char* mimetype, char* name);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->pipe_handler = pipe_handler;
     *     }
     * @endcode
     */
    guac_client_pipe_handler* pipe_handler;

    /**
     * Handler for ack events sent by the Guacamole web-client.
     *
     * The handler takes a guac_stream which contains the stream index and
     * will persist through the duration of the transfer, a string containing
     * the error or status message, and a status code.
     *
     * Example:
     * @code
     *     int ack_handler(guac_client* client, guac_stream* stream,
     *             char* error, guac_protocol_status status);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->ack_handler = ack_handler;
     *     }
     * @endcode
     */
    guac_client_ack_handler* ack_handler;

    /**
     * Handler for blob events sent by the Guacamole web-client.
     *
     * The handler takes a guac_stream which contains the stream index and
     * will persist through the duration of the transfer, an arbitrary buffer
     * containing the blob, and the length of the blob.
     *
     * Example:
     * @code
     *     int blob_handler(guac_client* client, guac_stream* stream,
     *             void* data, int length);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->blob_handler = blob_handler;
     *     }
     * @endcode
     */
    guac_client_blob_handler* blob_handler;

    /**
     * Handler for stream end events sent by the Guacamole web-client.
     *
     * The handler takes only a guac_stream which contains the stream index.
     * This guac_stream will be disposed of immediately after this event is
     * finished.
     *
     * Example:
     * @code
     *     int end_handler(guac_client* client, guac_stream* stream);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->end_handler = end_handler;
     *     }
     * @endcode
     */
    guac_client_end_handler* end_handler;

    /**
     * Handler for freeing data when the client is being unloaded.
     *
     * This handler will be called when the client needs to be unloaded
     * by the proxy, and any data allocated by the proxy client should be
     * freed.
     *
     * Note that this handler will NOT be called if the client's
     * guac_client_init() function fails.
     *
     * Implement this handler if you store data inside the client.
     *
     * Example:
     * @code
     *     int free_handler(guac_client* client);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->free_handler = free_handler;
     *     }
     * @endcode
     */
    guac_client_free_handler* free_handler;

    /**
     * Logging handler. This handler will be called via guac_client_log() when
     * the client needs to log messages of any type.
     *
     * In general, only programs loading the client should implement this
     * handler, as those are the programs that would provide the logging
     * facilities.
     *
     * Client implementations should expect these handlers to already be
     * set.
     *
     * Example:
     * @code
     *     void log_handler(guac_client* client, guac_client_log_level level, const char* format, va_list args);
     *
     *     void function_of_daemon() {
     *
     *         guac_client* client = [pass log_handler to guac_client_plugin_get_client()];
     *
     *     }
     * @endcode
     */
    guac_client_log_handler* log_handler;

    /**
     * Handler for get events sent by the Guacamole web-client.
     *
     * The handler takes a guac_object, containing the object index which will
     * persist through the duration of the transfer, and the name of the stream
     * being requested. It is up to the get handler to create the required body
     * stream.
     *
     * Example:
     * @code
     *     int get_handler(guac_client* client, guac_object* object,
     *             char* name);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->get_handler = get_handler;
     *     }
     * @endcode
     */
    guac_client_get_handler* get_handler;

    /**
     * Handler for put events sent by the Guacamole web-client.
     *
     * The handler takes a guac_object and guac_stream, which each contain their
     * respective indices which will persist through the duration of the
     * transfer, the mimetype of the data being transferred, and the name of
     * the stream within the object being written to.
     *
     * Example:
     * @code
     *     int put_handler(guac_client* client, guac_object* object,
     *             guac_stream* stream, char* mimetype, char* name);
     *
     *     int guac_client_init(guac_client* client, int argc, char** argv) {
     *         client->put_handler = put_handler;
     *     }
     * @endcode
     */
    guac_client_put_handler* put_handler;

    /**
     * Pool of buffer indices. Buffers are simply layers with negative indices.
     * Note that because guac_pool always gives non-negative indices starting
     * at 0, the output of this guac_pool will be adjusted.
     */
    guac_pool* __buffer_pool;

    /**
     * Pool of layer indices. Note that because guac_pool always gives
     * non-negative indices starting at 0, the output of this guac_pool will
     * be adjusted.
     */
    guac_pool* __layer_pool;

    /**
     * Pool of stream indices.
     */
    guac_pool* __stream_pool;

    /**
     * All available output streams (data going to connected client).
     */
    guac_stream* __output_streams;

    /**
     * All available input streams (data coming from connected client).
     */
    guac_stream* __input_streams;

    /**
     * Pool of object indices.
     */
    guac_pool* __object_pool;

    /**
     * All available objects (arbitrary sets of named streams).
     */
    guac_object* __objects;

    /**
     * The unique identifier allocated for the connection, which may
     * be used within the Guacamole protocol to refer to this connection.
     * This identifier is guaranteed to be unique from all existing
     * connections and will not collide with any available protocol
     * names.
     */
    char* connection_id;

};

/**
 * Returns a new, barebones guac_client. This new guac_client has no handlers
 * set, but is otherwise usable.
 *
 * @return A pointer to the new client.
 */
guac_client* guac_client_alloc();

/**
 * Free all resources associated with the given client.
 *
 * @param client The proxy client to free all reasources of.
 */
void guac_client_free(guac_client* client);

/**
 * Call the appropriate handler defined by the given client for the given
 * instruction. A comparison is made between the instruction opcode and the
 * initial handler lookup table defined in client-handlers.c. The intial
 * handlers will in turn call the client's handler (if defined).
 *
 * @param client The proxy client whose handlers should be called.
 * @param instruction The instruction to pass to the proxy client via the
 *                    appropriate handler.
 */
int guac_client_handle_instruction(guac_client* client, guac_instruction* instruction);

/**
 * Writes a message in the log used by the given client. The logger used will
 * normally be defined by guacd (or whichever program loads the proxy client)
 * by setting the logging handlers of the client when it is loaded.
 *
 * @param client The proxy client logging this message.
 * @param level The level at which to log this message.
 * @param format A printf-style format string to log.
 * @param ... Arguments to use when filling the format string for printing.
 */
void guac_client_log(guac_client* client, guac_client_log_level level,
        const char* format, ...);

/**
 * Writes a message in the log used by the given client. The logger used will
 * normally be defined by guacd (or whichever program loads the proxy client)
 * by setting the logging handlers of the client when it is loaded.
 *
 * @param client The proxy client logging this message.
 * @param level The level at which to log this message.
 * @param format A printf-style format string to log.
 * @param ap The va_list containing the arguments to be used when filling the
 *           format string for printing.
 */
void vguac_client_log(guac_client* client, guac_client_log_level level,
        const char* format, va_list ap);

/**
 * Signals the given client to stop gracefully. This is a completely
 * cooperative signal, and can be ignored by the client or the hosting
 * daemon.
 *
 * @param client The proxy client to signal to stop.
 */
void guac_client_stop(guac_client* client);

/**
 * Signals the given client to stop gracefully, while also signalling via the
 * Guacamole protocol that an error has occurred. Note that this is a completely
 * cooperative signal, and can be ignored by the client or the hosting
 * daemon. The message given will be logged to the system logs.
 *
 * @param client The proxy client to signal to stop.
 * @param status The status to send over the Guacamole protocol.
 * @param format A printf-style format string to log.
 * @param ... Arguments to use when filling the format string for printing.
 */
void guac_client_abort(guac_client* client, guac_protocol_status status,
        const char* format, ...);

/**
 * Signals the given client to stop gracefully, while also signalling via the
 * Guacamole protocol that an error has occurred. Note that this is a completely
 * cooperative signal, and can be ignored by the client or the hosting
 * daemon. The message given will be logged to the system logs.
 *
 * @param client The proxy client to signal to stop.
 * @param status The status to send over the Guacamole protocol.
 * @param format A printf-style format string to log.
 * @param ap The va_list containing the arguments to be used when filling the
 *           format string for printing.
 */
void vguac_client_abort(guac_client* client, guac_protocol_status status,
        const char* format, va_list ap);

/**
 * Allocates a new buffer (invisible layer). An arbitrary index is
 * automatically assigned if no existing buffer is available for use.
 *
 * @param client The proxy client to allocate the buffer for.
 * @return The next available buffer, or a newly allocated buffer.
 */
guac_layer* guac_client_alloc_buffer(guac_client* client);

/**
 * Allocates a new layer. An arbitrary index is automatically assigned
 * if no existing layer is available for use.
 *
 * @param client The proxy client to allocate the layer buffer for.
 * @return The next available layer, or a newly allocated layer.
 */
guac_layer* guac_client_alloc_layer(guac_client* client);

/**
 * Returns the given buffer to the pool of available buffers, such that it
 * can be reused by any subsequent call to guac_client_allow_buffer().
 *
 * @param client The proxy client to return the buffer to.
 * @param layer The buffer to return to the pool of available buffers.
 */
void guac_client_free_buffer(guac_client* client, guac_layer* layer);

/**
 * Returns the given layer to the pool of available layers, such that it
 * can be reused by any subsequent call to guac_client_allow_layer().
 *
 * @param client The proxy client to return the layer to.
 * @param layer The buffer to return to the pool of available layer.
 */
void guac_client_free_layer(guac_client* client, guac_layer* layer);

/**
 * Allocates a new stream. An arbitrary index is automatically assigned
 * if no previously-allocated stream is available for use.
 *
 * @param client The proxy client to allocate the layer buffer for.
 * @return The next available stream, or a newly allocated stream.
 */
guac_stream* guac_client_alloc_stream(guac_client* client);

/**
 * Returns the given stream to the pool of available streams, such that it
 * can be reused by any subsequent call to guac_client_alloc_stream().
 *
 * @param client The proxy client to return the buffer to.
 * @param stream The stream to return to the pool of available stream.
 */
void guac_client_free_stream(guac_client* client, guac_stream* stream);

/**
 * Allocates a new object. An arbitrary index is automatically assigned
 * if no previously-allocated object is available for use.
 *
 * @param client
 *     The proxy client to allocate the object for.
 *
 * @return
 *     The next available object, or a newly allocated object.
 */
guac_object* guac_client_alloc_object(guac_client* client);

/**
 * Returns the given object to the pool of available objects, such that it
 * can be reused by any subsequent call to guac_client_alloc_object().
 *
 * @param client
 *     The proxy client to return the object to.
 *
 * @param object
 *     The object to return to the pool of available object.
 */
void guac_client_free_object(guac_client* client, guac_object* object);

/**
 * Streams the image data of the given surface over an image stream ("img"
 * instruction) as PNG-encoded data. The image stream will be automatically
 * allocated and freed.
 *
 * @param client
 *     The Guacamole client from which the image stream should be allocated.
 *
 * @param socket
 *     The socket over which instructions associated with the image stream
 *     should be sent.
 *
 * @param mode
 *     The composite mode to use when rendering the image over the given layer.
 *
 * @param layer
 *     The destination layer.
 *
 * @param x
 *     The X coordinate of the upper-left corner of the destination rectangle
 *     within the given layer.
 *
 * @param y
 *     The Y coordinate of the upper-left corner of the destination rectangle
 *     within the given layer.
 *
 * @param surface
 *     A Cairo surface containing the image data to be streamed.
 */
void guac_client_stream_png(guac_client* client, guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer, int x, int y,
        cairo_surface_t* surface);

/**
 * Streams the image data of the given surface over an image stream ("img"
 * instruction) as JPEG-encoded data at the given quality. The image stream
 * will be automatically allocated and freed.
 *
 * @param client
 *     The Guacamole client from which the image stream should be allocated.
 *
 * @param socket
 *     The socket over which instructions associated with the image stream
 *     should be sent.
 *
 * @param mode
 *     The composite mode to use when rendering the image over the given layer.
 *
 * @param layer
 *     The destination layer.
 *
 * @param x
 *     The X coordinate of the upper-left corner of the destination rectangle
 *     within the given layer.
 *
 * @param y
 *     The Y coordinate of the upper-left corner of the destination rectangle
 *     within the given layer.
 *
 * @param surface
 *     A Cairo surface containing the image data to be streamed.
 *
 * @param quality
 *     The JPEG image quality, which must be an integer value between 0 and
 *     100 inclusive.
 */
void guac_client_stream_jpeg(guac_client* client, guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer, int x, int y,
        cairo_surface_t* surface, int quality);

/**
 * Streams the image data of the given surface over an image stream ("img"
 * instruction) as WebP-encoded data at the given quality. The image stream
 * will be automatically allocated and freed. If the server does not support
 * WebP, this function has no effect, so be sure to check the result of
 * guac_client_supports_webp() prior to calling this function.
 *
 * @param client
 *     The Guacamole client from which the image stream should be allocated.
 *
 * @param socket
 *     The socket over which instructions associated with the image stream
 *     should be sent.
 *
 * @param mode
 *     The composite mode to use when rendering the image over the given layer.
 *
 * @param layer
 *     The destination layer.
 *
 * @param x
 *     The X coordinate of the upper-left corner of the destination rectangle
 *     within the given layer.
 *
 * @param y
 *     The Y coordinate of the upper-left corner of the destination rectangle
 *     within the given layer.
 *
 * @param surface
 *     A Cairo surface containing the image data to be streamed.
 *
 * @param quality
 *     The WebP image quality, which must be an integer value between 0 and
 *     100 inclusive.
 */
void guac_client_stream_webp(guac_client* client, guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer, int x, int y,
        cairo_surface_t* surface, int quality);

/**
 * Returns whether the given client supports WebP. If the client does not
 * support WebP, or the server cannot encode WebP images, zero is returned.
 *
 * @param client
 *     The Guacamole client to check for WebP support.
 *
 * @return
 *     Non-zero if the given client claims to support WebP and the server has
 *     been built with WebP support, zero otherwise.
 */
int guac_client_supports_webp(guac_client* client);

/**
 * The default Guacamole client layer, layer 0.
 */
extern const guac_layer* GUAC_DEFAULT_LAYER;

#endif

