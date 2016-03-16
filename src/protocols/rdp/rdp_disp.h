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

#ifndef GUAC_RDP_DISP_H
#define GUAC_RDP_DISP_H

#include "rdp_settings.h"

#include <freerdp/freerdp.h>

#ifdef HAVE_FREERDP_CLIENT_DISP_H
#include <freerdp/client/disp.h>
#endif

/**
 * The minimum value for width or height, in pixels.
 */
#define GUAC_RDP_DISP_MIN_SIZE 200

/**
 * The maximum value for width or height, in pixels.
 */
#define GUAC_RDP_DISP_MAX_SIZE 8192

/**
 * The minimum amount of time that must elapse between display size updates,
 * in milliseconds.
 */
#define GUAC_RDP_DISP_UPDATE_INTERVAL 500

/**
 * Display size update module.
 */
typedef struct guac_rdp_disp {

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    /**
     * Display control interface.
     */
    DispClientContext* disp;
#endif

    /**
     * The timestamp of the last display update request, or 0 if no request
     * has been sent yet.
     */
    guac_timestamp last_request;

    /**
     * The last requested screen width, in pixels.
     */
    int requested_width;

    /**
     * The last requested screen height, in pixels.
     */
    int requested_height;

    /**
     * Whether the size has changed and the RDP connection must be closed and
     * reestablished.
     */
    int reconnect_needed;

} guac_rdp_disp;

/**
 * Allocates a new display update module, which will ultimately control the
 * display update channel once conected.
 *
 * @return A new display update module.
 */
guac_rdp_disp* guac_rdp_disp_alloc();

/**
 * Frees the given display update module.
 *
 * @param disp The display update module to free.
 */
void guac_rdp_disp_free(guac_rdp_disp* disp);

/**
 * Loads the "disp" plugin for FreeRDP. It is still up to external code to
 * detect when the "disp" channel is connected, and update the guac_rdp_disp
 * with a call to guac_rdp_disp_connect().
 *
 * @param context The rdpContext associated with the active RDP session.
 */
void guac_rdp_disp_load_plugin(rdpContext* context);

/**
 * Stores the given DispClientContext within the given guac_rdp_disp, such that
 * display updates can be properly sent. Until this is called, changes to the
 * display size will be deferred.
 *
 * @param guac_disp The display update module to associate with the connected
 *                  display update channel.
 * @param disp The DispClientContext associated by FreeRDP with the connected
 *             display update channel.
 */
void guac_rdp_disp_connect(guac_rdp_disp* guac_disp, DispClientContext* disp);

/**
 * Requests a display size update, which may then be sent immediately to the
 * RDP server. If an update was recently sent, this update may be delayed until
 * the RDP server has had time to settle. The width/height values provided may
 * be automatically altered to comply with the restrictions imposed by the
 * display update channel.
 *
 * @param disp
 *     The display update module which should maintain the requested size,
 *     sending the corresponding display update request when appropriate.
 *
 * @param settings
 *     The RDP client settings associated with the current or pending RDP
 *     session. These settings will be automatically adjusted to match the new
 *     screen size.
 *
 * @param rdp_inst
 *     The FreeRDP instance associated with the current or pending RDP session,
 *     if any. If no RDP session is active, this should be NULL.
 *
 * @param width
 *     The desired display width, in pixels. Due to the restrictions of the RDP
 *     display update channel, this will be contrained to the range of 200
 *     through 8192 inclusive, and rounded down to the nearest even number.
 *
 * @param height
 *     The desired display height, in pixels. Due to the restrictions of the
 *     RDP display update channel, this will be contrained to the range of 200
 *     through 8192 inclusive.
 */
void guac_rdp_disp_set_size(guac_rdp_disp* disp, guac_rdp_settings* settings,
        freerdp* rdp_inst, int width, int height);

/**
 * Sends an actual display update request to the RDP server based on previous
 * calls to guac_rdp_disp_set_size(). If an update was recently sent, the
 * update may be delayed until a future call to this function. If the RDP
 * session has not yet been established, the request will be delayed until the
 * session exists.
 *
 * @param disp
 *     The display update module which should track the update request.
 *
 * @param settings
 *     The RDP client settings associated with the current or pending RDP
 *     session. These settings will be automatically adjusted to match the new
 *     screen size.
 *
 * @param rdp_inst
 *     The FreeRDP instance associated with the current or pending RDP session,
 *     if any. If no RDP session is active, this should be NULL.
 */
void guac_rdp_disp_update_size(guac_rdp_disp* disp,
        guac_rdp_settings* settings, freerdp* rdp_inst);

/**
 * Signals the given display update module that the requested reconnect has
 * been performed.
 *
 * @param disp
 *     The display update module that should be signaled regarding the state
 *     of reconnection.
 */
void guac_rdp_disp_reconnect_complete(guac_rdp_disp* disp);

/**
 * Returns whether a full RDP reconnect is required for display update changes
 * to take effect.
 *
 * @return
 *     Non-zero if a reconnect is needed, zero otherwise.
 */
int guac_rdp_disp_reconnect_needed(guac_rdp_disp* disp);

#endif

