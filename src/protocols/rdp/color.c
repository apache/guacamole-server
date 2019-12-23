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

#include "client.h"
#include "rdp.h"
#include "settings.h"

#include <freerdp/codec/color.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <winpr/wtypes.h>

/**
 * Returns the integer constant used by the FreeRDP API to represent the colors
 * used by a connection having the given bit depth. These constants each have
 * corresponding PIXEL_FORMAT_* macros defined within freerdp/codec/color.h.
 *
 * @param depth
 *     The color depth which should be translated into the integer constant
 *     defined by FreeRDP's corresponding PIXEL_FORMAT_* macro.
 *
 * @return
 *     The integer value of the PIXEL_FORMAT_* macro corresponding to the
 *     given color depth.
 */
static UINT32 guac_rdp_get_pixel_format(int depth) {

    switch (depth) {

        /* 32- and 24-bit RGB (8 bits per color component) */
        case 32:
        case 24:
            return PIXEL_FORMAT_RGB24;

        /* 16-bit palette (6-bit green, 5-bit red and blue) */
        case 16:
            return PIXEL_FORMAT_RGB16;

        /* 15-bit RGB (5 bits per color component) */
        case 15:
            return PIXEL_FORMAT_RGB15;

        /* 8-bit palette */
        case 8:
            return PIXEL_FORMAT_RGB8;

    }

    /* Unknown format */
    return PIXEL_FORMAT_RGB24;

}

UINT32 guac_rdp_convert_color(rdpContext* context, UINT32 color) {

    int depth = guac_rdp_get_depth(context->instance);
    rdpGdi* gdi = context->gdi;

    /* Convert given color to ARGB32 */
    return FreeRDPConvertColor(color, guac_rdp_get_pixel_format(depth),
            PIXEL_FORMAT_BGRA32, &gdi->palette);

}

