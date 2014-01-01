/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef __GUAC_RDPDR_FS_H
#define __GUAC_RDPDR_FS_H

/**
 * Functions and macros specific to filesystem handling and initialization
 * independent of RDP.  The functions here may deal with the RDPDR device
 * directly, but their semantics must not deal with RDP protocol messaging.
 * Functions here represent a virtual Windows-style filesystem on top of UNIX
 * system calls and structures, using the guac_rdpdr_device structure as a home
 * for common data.
 *
 * @file rdpdr_fs.h 
 */

#include "config.h"

#include "rdpdr_service.h"

#include <guacamole/pool.h>

/**
 * Registers a new filesystem device within the RDPDR plugin. This must be done
 * before RDPDR connection finishes.
 */
void guac_rdpdr_register_fs(guac_rdpdrPlugin* rdpdr);

#endif

