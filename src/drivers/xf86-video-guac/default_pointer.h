
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

#ifndef _GUAC_DEFAULT_POINTER_H
#define _GUAC__DEFAULT_POINTER_H

#include <cairo/cairo.h>
#include <guacamole/client.h>

/**
 * Width of the embedded mouse cursor graphic.
 */
extern const int guac_drv_default_pointer_width;

/**
 * Height of the embedded mouse cursor graphic.
 */
extern const int guac_drv_default_pointer_height;

/**
 * Number of bytes in each row of the embedded mouse cursor graphic.
 */
extern const int guac_drv_default_pointer_stride;

/**
 * The Cairo grapic format of the mouse cursor graphic.
 */
extern const cairo_format_t guac_drv_default_pointer_format;

/**
 * Embedded mouse cursor graphic.
 */
extern unsigned char guac_drv_default_pointer[];

/**
 * Set the cursor of the remote display to the embedded cursor graphic.
 *
 * @param client The guac_client to send the cursor to.
 */
void guac_drv_set_default_pointer(guac_client* client);

#endif
