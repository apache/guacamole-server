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
#include "rdp_settings.h"

#include <freerdp/codec/color.h>
#include <freerdp/freerdp.h>
#include <winpr/wtypes.h>

UINT32 guac_rdp_convert_color(rdpContext* context, UINT32 color) {

    CLRCONV* clrconv = ((rdp_freerdp_context*) context)->clrconv;

    /* Convert given color to ARGB32 */
    return freerdp_color_convert_drawing_order_color_to_gdi_color(color,
            guac_rdp_get_depth(context->instance), clrconv);

}

