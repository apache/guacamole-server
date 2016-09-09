
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

Bool guac_drv_init_cursor(ScreenPtr screen) {

    /* Get cursor info struct */
    xf86CursorInfoPtr cursor_info = xf86CreateCursorInfoRec();
    if (!cursor_info)
        return FALSE;

    /* Init cursor info */
    cursor_info->MaxHeight = 64;
    cursor_info->MaxWidth = 64;
    cursor_info->Flags = HARDWARE_CURSOR_ARGB;

    /* Set handlers */
    cursor_info->SetCursorColors = guac_drv_set_cursor_colors;
    cursor_info->SetCursorPosition = guac_drv_set_cursor_position;
    cursor_info->LoadCursorImage = guac_drv_load_cursor_image;
    cursor_info->HideCursor = guac_drv_hide_cursor;
    cursor_info->ShowCursor = guac_drv_show_cursor;
    cursor_info->UseHWCursor = guac_drv_use_hw_cursor;

    return xf86InitCursor(screen, cursor_info);

}

