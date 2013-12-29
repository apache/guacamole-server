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

#include <stdarg.h>

#include "instruction.h"
#include "layer.h"
#include "pool.h"
#include "protocol.h"
#include "socket.h"
#include "stream.h"
#include "timestamp.h"

/**
 * Provides functions and structures required for defining (and handling) a proxy client.
 *
 * @file client.h
 */

/**
 * The maximum number of inbound streams supported by any one guac_client.
 */
#define GUAC_CLIENT_MAX_STREAMS 64

/**
 * The index of a closed stream.
 */
#define GUAC_CLIENT_CLOSED_STREAM_INDEX -1

typedef struct guac_client guac_client;

/**
 * Handler for server messages (where "server" refers to the server that
 * the proxy client is connected to).
 */
typedef int guac_client_handle_messages(guac_client* client);

/**
 * Handler for Guacamole mouse events.
 */
typedef int guac_client_mouse_handler(guac_client* client, int x, int y, int button_mask);

/**
 * Handler for Guacamole key events.
 */
typedef int guac_client_key_handler(guac_client* client, int keysym, int pressed);

/**
 * Handler for Guacamole clipboard events.
 */
typedef int guac_client_clipboard_handler(guac_client* client, char* copied);

/**
 * Handler for Guacamole screen size events.
 */
typedef int guac_client_size_handler(guac_client* client,
        int width, int height);

/**
 * Handler for Guacamole file transfer events.
 */
typedef int guac_client_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename);

/**
 * Handler for Guacamole stream blob events.
 */
typedef int guac_client_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length);

/**
 * Handler for Guacamole stream ack events.
 */
typedef int guac_client_ack_handler(guac_client* client, guac_stream* stream,
        char* error, guac_protocol_status status);

/**
 * Handler for Guacamole stream end events.
 */
typedef int guac_client_end_handler(guac_client* client, guac_stream* stream);

/**
 * Handler for Guacamole audio format events.
 */
typedef int guac_client_audio_handler(guac_client* client, char* mimetype);

/**
 * Handler for Guacamole video format events.
 */
typedef int guac_client_video_handler(guac_client* client, char* mimetype);

/**
 * Handler for freeing up any extra data allocated by the client
 * implementation.
 */
typedef int guac_client_free_handler(guac_client* client);

/**
 * Handler for logging messages
 */
typedef void guac_client_log_handler(guac_client* client, const char* format, va_list args); 

/**
 * Handler which should initialize the given guac_client.
 */
typedef int guac_client_init_handler(guac_client* client, int argc, char** argv);

/**
 * The flag set in the mouse button mask when the left mouse button is down.
 */
#define GUAC_CLIENT_MOUSE_LEFT        0x01

/**
 * The flag set in the mouse button mask when the middle mouse button is down.
 */
#define GUAC_CLIENT_MOUSE_MIDDLE      0x02

/**
 * The flag set in the mouse button mask when the right mouse button is down.
 */
#define GUAC_CLIENT_MOUSE_RIGHT       0x04

/**
 * The flag set in the mouse button mask when the mouse scrollwheel is scrolled
 * up. Note that mouse scrollwheels are actually sets of two buttons. One
 * button is pressed and released for an upward scroll, and the other is
 * pressed and released for a downward scroll. Some mice may actually implement
 * these as separate buttons, not a wheel.
 */
#define GUAC_CLIENT_MOUSE_SCROLL_UP   0x08

/**
 * The flag set in the mouse button mask when the mouse scrollwheel is scrolled
 * down. Note that mouse scrollwheels are actually sets of two buttons. One
 * button is pressed and released for an upward scroll, and the other is
 * pressed and released for a downward scroll. Some mice may actually implement
 * these as separate buttons, not a wheel.
 */
#define GUAC_CLIENT_MOUSE_SCROLL_DOWN 0x10

/**
 * The minimum number of buffers to create before allowing free'd buffers to
 * be reclaimed. In the case a protocol rapidly creates, uses, and destroys
 * buffers, this can prevent unnecessary reuse of the same buffer (which
 * would make draw operations unnecessarily synchronous).
 */
#define GUAC_BUFFER_POOL_INITIAL_SIZE 1024

/**
 * Possible current states of the Guacamole client. Currently, the only
 * two states are GUAC_CLIENT_RUNNING and GUAC_CLIENT_STOPPING.
 */
typedef enum guac_client_state {

    /**
     * The state of the client from when it has been allocated by the main
     * daemon until it is killed or disconnected.
     */
    GUAC_CLIENT_RUNNING,

    /**
     * The state of the client when a stop has been requested, signalling the
     * I/O threads to shutdown.
     */
    GUAC_CLIENT_STOPPING

} guac_client_state;

/**
 * Information exposed by the remote client during the connection handshake
 * which can be used by a client plugin.
 */
typedef struct guac_client_info {

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
     * The DPI of the physical remote display if configured for the optimal
     * width/height combination described here. This need not be honored by
     * a client plugin implementation, but if the underlying protocol of the
     * client plugin supports dynamic sizing of the screen, honoring the
     * stated resolution of the display size request is recommended.
     */
    int optimal_resolution;

} guac_client_info;

/**
 * Guacamole proxy client.
 *
 * Represents a Guacamole proxy client (the client which communicates to
 * a server on behalf of Guacamole, on behalf of the web-client).
 */
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
     * This handler takes a single string which contains the text which
     * has been set in the clipboard. This text is already unescaped from
     * the Guacamole escaped version sent within the clipboard message
     * in the protocol.
     *
     * Example:
     * @code
     *     int clipboard_handler(guac_client* client, char* copied);
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
     * Handler for logging informational messages. This handler will be called
     * via guac_client_log_info() when the client needs to log information.
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
     *     void log_handler(guac_client* client, const char* format, va_list args);
     *
     *     void function_of_daemon() {
     *
     *         guac_client* client = [pass log_handler to guac_client_plugin_get_client()];
     *
     *     }
     * @endcode
     */
    guac_client_log_handler* log_info_handler;

    /**
     * Handler for logging error messages. This handler will be called
     * via guac_client_log_error() when the client needs to log an error.
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
     *     void log_handler(guac_client* client, const char* format, va_list args);
     *
     *     void function_of_daemon() {
     *
     *         guac_client* client = [pass log_handler to guac_client_plugin_get_client()];
     *
     *     }
     * @endcode
     */
    guac_client_log_handler* log_error_handler;

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
    guac_stream __output_streams[GUAC_CLIENT_MAX_STREAMS];

    /**
     * All available input streams (data coming from connected client).
     */
    guac_stream __input_streams[GUAC_CLIENT_MAX_STREAMS];

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
 * Logs an informational message in the log used by the given client. The
 * logger used will normally be defined by guacd (or whichever program loads
 * the proxy client) by setting the logging handlers of the client when it is
 * loaded.
 *
 * @param client The proxy client to log an informational message for.
 * @param format A printf-style format string to log.
 * @param ... Arguments to use when filling the format string for printing.
 */
void guac_client_log_info(guac_client* client, const char* format, ...);

/**
 * Logs an error message in the log used by the given client. The logger
 * used will normally be defined by guacd (or whichever program loads the
 * proxy client) by setting the logging handlers of the client when it is
 * loaded.
 *
 * @param client The proxy client to log an error for.
 * @param format A printf-style format string to log.
 * @param ... Arguments to use when filling the format string for printing.
 */
void guac_client_log_error(guac_client* client, const char* format, ...);

/**
 * Logs an informational message in the log used by the given client. The
 * logger used will normally be defined by guacd (or whichever program loads
 * the proxy client) by setting the logging handlers of the client when it is
 * loaded.
 *
 * @param client The proxy client to log an informational message for.
 * @param format A printf-style format string to log.
 * @param ap The va_list containing the arguments to be used when filling the
 *           format string for printing.
 */
void vguac_client_log_info(guac_client* client, const char* format, va_list ap);

/**
 * Logs an error message in the log used by the given client. The logger
 * used will normally be defined by guacd (or whichever program loads the
 * proxy client) by setting the logging handlers of the client when it is
 * loaded.
 *
 * @param client The proxy client to log an error for.
 * @param format A printf-style format string to log.
 * @param ap The va_list containing the arguments to be used when filling the
 *           format string for printing.
 */
void vguac_client_log_error(guac_client* client, const char* format, va_list ap);

/**
 * Signals the given client to stop gracefully. This is a completely
 * cooperative signal, and can be ignored by the client or the hosting
 * daemon.
 *
 * @param client The proxy client to signal to stop.
 */
void guac_client_stop(guac_client* client);

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
 * The default Guacamole client layer, layer 0.
 */
extern const guac_layer* GUAC_DEFAULT_LAYER;

#endif
