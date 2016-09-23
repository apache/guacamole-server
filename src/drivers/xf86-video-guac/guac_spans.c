
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
#include "guac_spans.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

void guac_drv_fillspans(DrawablePtr drawable, GCPtr gc, int npoints,
        DDXPointPtr points, int* width, int sorted) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
    fbFillSpans(drawable, gc, npoints, points, width, sorted);
}

void guac_drv_setspans(DrawablePtr drawable, GCPtr gc, char* src,
        DDXPointPtr points, int* width, int nspans, int sorted) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s\n", __func__);
    fbSetSpans(drawable, gc, src, points, width, nspans, sorted);
}

