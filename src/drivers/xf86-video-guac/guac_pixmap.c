
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
#include "guac_pixmap.h"
#include "guac_window.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

static DevPrivateKeyRec __GUAC_PIXMAP_PRIVATE;

const DevPrivateKey GUAC_PIXMAP_PRIVATE = &__GUAC_PIXMAP_PRIVATE;

PixmapPtr guac_drv_get_pixmap(DrawablePtr drawable) {

    if (drawable->type != DRAWABLE_PIXMAP)
        return fbGetWindowPixmap(drawable);

    return (PixmapPtr) drawable;

}

guac_drv_drawable* guac_drv_get_drawable(DrawablePtr drawable) {

    if (drawable->type != DRAWABLE_PIXMAP) {

        WindowPtr window = (WindowPtr) drawable;
        return (guac_drv_drawable*)
            dixGetPrivate(&(window->devPrivates), GUAC_WINDOW_PRIVATE);

    }

    else {

        PixmapPtr pixmap = (PixmapPtr) drawable;
        return (guac_drv_drawable*)
            dixGetPrivate(&(pixmap->devPrivates), GUAC_PIXMAP_PRIVATE);

    }

}
