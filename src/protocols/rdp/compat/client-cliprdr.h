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


#ifndef __GUAC_CLIENT_CLIPRDR_COMPAT_H
#define __GUAC_CLIENT_CLIPRDR_COMPAT_H

#include "config.h"

#include <freerdp/plugins/cliprdr.h>

#define CliprdrChannel_Class        RDP_EVENT_CLASS_CLIPRDR
#define CliprdrChannel_FormatList   RDP_EVENT_TYPE_CB_FORMAT_LIST
#define CliprdrChannel_MonitorReady RDP_EVENT_TYPE_CB_MONITOR_READY
#define CliprdrChannel_DataRequest  RDP_EVENT_TYPE_CB_DATA_REQUEST
#define CliprdrChannel_DataResponse RDP_EVENT_TYPE_CB_DATA_RESPONSE

#endif

