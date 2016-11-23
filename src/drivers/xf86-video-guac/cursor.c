
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
#include "cursor.h"
#include "display.h"
#include "screen.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86Cursor.h>
#include <cursorstr.h>

static void guac_drv_cursor_set_argb(guac_drv_cursor* guac_cursor,
        CursorPtr cursor) {

    xf86Msg(X_INFO, "guac: STUB: %s (%i, %i) %ix%i\n", __func__,
            guac_cursor->hotspot_x, guac_cursor->hotspot_y,
            guac_cursor->width, guac_cursor->height);

}

static void guac_drv_cursor_set_glyph(guac_drv_cursor* guac_cursor,
        CursorPtr cursor) {

    int x;
    int y;

    /* Calculate foreground color */
    uint32_t fg = 0xFF000000 | (cursor->foreRed << 16)
        | (cursor->foreGreen << 8) | (cursor->foreBlue);

    /* Calculate background color */
    uint32_t bg = 0xFF000000 | (cursor->backRed << 16)
        | (cursor->backGreen << 8) | (cursor->backBlue);

    /* Get source and destination image data */
    uint32_t* src_row = (uint32_t*) cursor->bits->source;
    uint32_t* mask_row = (uint32_t*) cursor->bits->mask;
    uint32_t* dst_row = guac_cursor->image;

    /* For each row of image data */
    for (y = 0; y < guac_cursor->height; y++) {

        /* Get first pixel/bit in current row */
        uint32_t* dst = dst_row;
        uint32_t src = *src_row;
        uint32_t mask = *mask_row;

        /* For each pixel within row */
        for (x = 0; x < guac_cursor->width; x++) {

            /* Draw pixel only if mask is set. */
            if (mask & 0x1) {

                /* Select foreground/background depending on source bit */
                *dst = (src & 0x1) ? fg : bg;

            }

            /* Next pixel */
            dst++;
            src >>= 1;
            mask >>= 1;

        }

        /* Next row */
        dst_row += GUAC_DRV_CURSOR_MAX_WIDTH;
        src_row++;
        mask_row++;

    }

}

static unsigned char* guac_drv_realize_cursor(xf86CursorInfoPtr cursor_info,
        CursorPtr cursor) {

    guac_drv_cursor* guac_cursor =
        (guac_drv_cursor*) calloc(1, sizeof(guac_drv_cursor));

    /* Assign dimensions */
    guac_cursor->width = cursor->bits->width;
    guac_cursor->height = cursor->bits->height;

    /* Assign hotspot */
    guac_cursor->hotspot_x = cursor->bits->xhot;
    guac_cursor->hotspot_y = cursor->bits->yhot;

    /* Use ARGB cursor image if available */
    if (cursor->bits->argb != NULL)
        guac_drv_cursor_set_argb(guac_cursor, cursor);

    /* Otherwise, use glyph cursor */
    else
        guac_drv_cursor_set_glyph(guac_cursor, cursor);

    return (unsigned char*) guac_cursor;

}

static void guac_drv_set_cursor_colors(ScrnInfoPtr screen, int bg, int fg) {
    /* Do nothing */
}

static void guac_drv_set_cursor_position(ScrnInfoPtr screen, int x, int y) {
    /* Do nothing */
}

static void guac_drv_load_cursor_image(ScrnInfoPtr screen_info,
        unsigned char* image) {

    guac_drv_cursor* guac_cursor = (guac_drv_cursor*) image;

    /* Get guac_drv_screen */
    ScreenPtr screen = screen_info->pScreen;
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                GUAC_SCREEN_PRIVATE);

    /* Set cursor of display */
    guac_common_cursor_set_argb(guac_screen->display->display->cursor,
            guac_cursor->hotspot_x, guac_cursor->hotspot_y,
            (unsigned char*) guac_cursor->image, guac_cursor->width,
            guac_cursor->height, GUAC_DRV_CURSOR_STRIDE);

    guac_drv_display_touch(guac_screen->display);

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
    cursor_info->MaxHeight = GUAC_DRV_CURSOR_MAX_WIDTH;
    cursor_info->MaxWidth = GUAC_DRV_CURSOR_MAX_HEIGHT;
    cursor_info->Flags =
          HARDWARE_CURSOR_ARGB
        | HARDWARE_CURSOR_UPDATE_UNHIDDEN
        | HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1;

    /* Set handlers */
    cursor_info->RealizeCursor = guac_drv_realize_cursor;
    cursor_info->SetCursorPosition = guac_drv_set_cursor_position;
    cursor_info->HideCursor = guac_drv_hide_cursor;
    cursor_info->ShowCursor = guac_drv_show_cursor;

    /* Glyph cursors (ARGB data is stored within the cursor data by our
     * implementation of RealizeCursor) */
    cursor_info->SetCursorColors = guac_drv_set_cursor_colors;
    cursor_info->UseHWCursor = guac_drv_use_hw_cursor;
    cursor_info->LoadCursorImage = guac_drv_load_cursor_image;

    return xf86InitCursor(screen, cursor_info);

}

