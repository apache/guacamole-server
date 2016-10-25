
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

#include <xorg-server.h>
#include <xf86.h>
#include <xf86Cursor.h>
#include <cursorstr.h>

static void guac_drv_set_cursor_colors(ScrnInfoPtr screen, int bg, int fg) {
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
}

static void guac_drv_set_cursor_position(ScrnInfoPtr screen, int x, int y) {
    /*xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);*/
}

static void guac_drv_load_cursor_image(ScrnInfoPtr screen,
        unsigned char* image) {
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
}

static void guac_drv_hide_cursor(ScrnInfoPtr screen) {
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
}

static void guac_drv_show_cursor(ScrnInfoPtr screen) {
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
}

static Bool guac_drv_use_hw_cursor(ScreenPtr screen, CursorPtr cursor) {
    return TRUE;
}

static Bool guac_drv_use_hw_cursor_argb(ScreenPtr screen, CursorPtr cursor) {
    return TRUE;
}

static void guac_drv_load_cursor_argb(ScrnInfoPtr screen_info,
        CursorPtr cursor) {
    xf86Msg(X_INFO, "guac: STUB: %s: (%i, %i) %ix%i\n", __func__,
            cursor->bits->xhot,
            cursor->bits->yhot,
            cursor->bits->width,
            cursor->bits->height);
}

Bool guac_drv_init_cursor(ScreenPtr screen) {

    /* Get cursor info struct */
    xf86CursorInfoPtr cursor_info = xf86CreateCursorInfoRec();
    if (!cursor_info)
        return FALSE;

    /* Init cursor info */
    cursor_info->MaxHeight = 64;
    cursor_info->MaxWidth = 64;
    cursor_info->Flags =
          HARDWARE_CURSOR_ARGB
        | HARDWARE_CURSOR_UPDATE_UNHIDDEN;

    /* Set handlers */
    cursor_info->SetCursorPosition = guac_drv_set_cursor_position;
    cursor_info->HideCursor = guac_drv_hide_cursor;
    cursor_info->ShowCursor = guac_drv_show_cursor;

    /* Legacy cursors */
    cursor_info->SetCursorColors = guac_drv_set_cursor_colors;
    cursor_info->UseHWCursor = guac_drv_use_hw_cursor;
    cursor_info->LoadCursorImage = guac_drv_load_cursor_image;

    /* Full ARGB cursors */
    cursor_info->UseHWCursorARGB = guac_drv_use_hw_cursor_argb;
    cursor_info->LoadCursorARGB = guac_drv_load_cursor_argb;

    return xf86InitCursor(screen, cursor_info);

}

