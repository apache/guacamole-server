
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

#ifndef __GUAC_POLY_H
#define __GUAC_POLY_H

#include "config.h"

#include <xorg-server.h>
#include <xf86.h>

/**
 * Guacamole implementation of PolyPoint.
 */
void guac_drv_polypoint(DrawablePtr drawable, GCPtr gc, int mode, int npt,
        DDXPointPtr init);

/**
 * Guacamole implementation of PolyLine.
 */
void guac_drv_polyline(DrawablePtr drawable, GCPtr gc, int mode, int npt,
        DDXPointPtr init);

/**
 * Guacamole implementation of PolySegment.
 */
void guac_drv_polysegment(DrawablePtr drawable, GCPtr gc, int nseg,
        xSegment* segs);

/**
 * Guacamole implementation of PolyRectangle.
 */
void guac_drv_polyrectangle(DrawablePtr drawable, GCPtr gc, int nrects,
        xRectangle* rects);

/**
 * Guacamole implementation of PolyArc.
 */
void guac_drv_polyarc(DrawablePtr drawable, GCPtr gc, int narcs,
        xArc* arcs);

/**
 * Guacamole implementation of PolyPolygon.
 */
void guac_drv_fillpolygon(DrawablePtr drawable, GCPtr gc, int shape, int mode,
        int count, DDXPointPtr pts);

/**
 * Guacamole implementation of PolyFillRect.
 */
void guac_drv_polyfillrect(DrawablePtr drawable, GCPtr gc, int nrects,
        xRectangle* rects);

/**
 * Guacamole implementation of PolyFillArc.
 */
void guac_drv_polyfillarc(DrawablePtr drawable, GCPtr gc, int narcs,
        xArc* arcs);

#endif

