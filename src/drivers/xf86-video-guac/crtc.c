
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
#include "crtc.h"
#include "display.h"
#include "drawable.h"
#include "screen.h"
#include "window.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86Crtc.h>

static Bool guac_drv_crtc_resize(ScrnInfoPtr screen_info,
        int width, int height) {

    /* Get guac_drv_screen */
    ScreenPtr screen = screen_info->pScreen;
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                                         GUAC_SCREEN_PRIVATE);

    /* Resize underlying pixmap of screen */
    PixmapPtr pixmap = screen->GetScreenPixmap(screen);
    if (pixmap != NULL)
        screen->ModifyPixmapHeader(pixmap, width, height, -1, -1,
                width * 4, guac_screen->framebuffer);

    /* Resize Guacamole display */
    guac_drv_display* display = guac_screen->display;
    guac_drv_display_resize(display, width, height);
    guac_drv_display_touch(display);

    /* Get root drawable */
    guac_drv_drawable* root =
        (guac_drv_drawable*) dixGetPrivate(&(screen->root->devPrivates),
                                         GUAC_WINDOW_PRIVATE);

    /* Resize root drawable to fit display */
    guac_drv_drawable_resize(root, width, height);

    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s %ix%i\n", __func__, width, height);

    return TRUE;

}

xf86CrtcConfigFuncsRec guac_drv_crtc_configfuncs = {
    .resize = guac_drv_crtc_resize
};

static void guac_drv_crtc_dpms(xf86CrtcPtr crtc, int mode) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
}

static Bool guac_drv_crtc_lock(xf86CrtcPtr crtc) {
    return TRUE;
}

static void guac_drv_crtc_unlock(xf86CrtcPtr crtc) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
}

static void guac_drv_crtc_set_cursor_colors(xf86CrtcPtr crtc, int bg, int fg) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
}

static void guac_drv_crtc_set_cursor_position(xf86CrtcPtr crtc, int x, int y) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
}

static void guac_drv_crtc_hide_cursor(xf86CrtcPtr crtc) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
}

static void guac_drv_crtc_show_cursor(xf86CrtcPtr crtc) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
}

static void guac_drv_crtc_load_cursor_argb(xf86CrtcPtr crtc, CARD32* image) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
}

static void guac_drv_crtc_destroy(xf86CrtcPtr crtc) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
}

static Bool guac_drv_crtc_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode,
        Rotation rotation, int x, int y) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
    return TRUE;
}

xf86CrtcFuncsRec guac_drv_crtc_funcs = {
    .dpms = guac_drv_crtc_dpms,
    .lock = guac_drv_crtc_lock,
    .unlock = guac_drv_crtc_unlock,
    .set_cursor_colors = guac_drv_crtc_set_cursor_colors,
    .set_cursor_position = guac_drv_crtc_set_cursor_position,
    .show_cursor = guac_drv_crtc_show_cursor,
    .hide_cursor = guac_drv_crtc_hide_cursor,
    .load_cursor_argb = guac_drv_crtc_load_cursor_argb,
    .destroy = guac_drv_crtc_destroy,
    .set_mode_major = guac_drv_crtc_set_mode_major
};

static void guac_drv_output_dpms(xf86OutputPtr output, int mode) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
}

static Bool guac_drv_output_mode_valid(xf86OutputPtr output,
        DisplayModePtr modes) {

    /* Accept all modes */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
    return MODE_OK;

}

static xf86OutputStatus guac_drv_output_detect(xf86OutputPtr output) {

    /* Output is always connected */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
    return XF86OutputStatusConnected;

}

static DisplayModePtr guac_drv_output_get_modes(xf86OutputPtr output) {

    DisplayModePtr mode = xnfcalloc(1, sizeof(DisplayModeRec));

    mode->status = MODE_OK;
    mode->HDisplay = 1024;
    mode->VDisplay = 768;

    xf86SetModeDefaultName(mode);
    xf86SetModeCrtc(mode, 0);

    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
    return mode;

}

static Bool guac_drv_output_set_property(xf86OutputPtr output, Atom property,
        RRPropertyValuePtr value) {
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
    return TRUE;
}

static Bool guac_drv_output_get_property(xf86OutputPtr output, Atom property) {
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
    return TRUE;
}

static void guac_drv_output_destroy(xf86OutputPtr output) {
    /* Do nothing */
    xf86Msg(X_INFO, "guac: NOTE: %s\n", __func__);
}

xf86OutputFuncsRec guac_drv_output_funcs = {
    .dpms = guac_drv_output_dpms,
    .mode_valid = guac_drv_output_mode_valid,
    .detect = guac_drv_output_detect,
    .get_modes = guac_drv_output_get_modes,
    .set_property = guac_drv_output_set_property,
    .get_property = guac_drv_output_get_property,
    .destroy = guac_drv_output_destroy
};

