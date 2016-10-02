
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
#include "copy.h"
#include "gc.h"
#include "image.h"
#include "image_text.h"
#include "pixmap.h"
#include "poly.h"
#include "poly_text.h"
#include "spans.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <gcstruct.h>

static DevPrivateKeyRec __GUAC_GC_PRIVATE;

const DevPrivateKey GUAC_GC_PRIVATE = &__GUAC_GC_PRIVATE;

GCOps guac_drv_gcops = {
    guac_drv_fillspans,
    guac_drv_setspans,
    guac_drv_putimage,
    guac_drv_copyarea,
    guac_drv_copyplane,
    guac_drv_polypoint,
    guac_drv_polyline,
    guac_drv_polysegment,
    guac_drv_polyrectangle,
    guac_drv_polyarc,
    guac_drv_fillpolygon,
    guac_drv_polyfillrect,
    guac_drv_polyfillarc,
    guac_drv_polytext8,
    guac_drv_polytext16,
    guac_drv_imagetext8,
    guac_drv_imagetext16,
    guac_drv_imageglyphblt,
    guac_drv_polyglyphblt,
    guac_drv_pushpixels
};

