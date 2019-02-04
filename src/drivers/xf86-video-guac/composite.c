
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
#include "display.h"
#include "drawable.h"
#include "log.h"
#include "pixmap.h"
#include "screen.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

void guac_drv_composite(CARD8 op,
        PicturePtr src, PicturePtr mask, PicturePtr dst,
        INT16 src_x, INT16 src_y, INT16 mask_x, INT16 mask_y,
        INT16 dst_x, INT16 dst_y, CARD16 width, CARD16 height) {

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(dst->pDrawable);

    /* Draw to windows only */
    if (guac_drawable == NULL)
        return;

    /* Get guac_drv_screen */
    ScreenPtr screen = dst->pDrawable->pScreen;
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                                         GUAC_SCREEN_PRIVATE);

    /* Invoke underlying composite implementation */
    guac_screen->wrapped_composite(op, src, mask, dst,
            src_x, src_y, mask_x, mask_y, dst_x, dst_y,
            width, height);

    /* Copy region from framebuffer */
    guac_drv_drawable_copy_fb(dst->pDrawable, dst_x, dst_y,
        width, height, guac_drawable, dst_x, dst_y);

    /* Signal change */
    guac_drv_display_touch(guac_screen->display);

}

