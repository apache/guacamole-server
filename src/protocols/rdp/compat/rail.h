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


#ifndef __GUAC_RAIL_COMPAT_H
#define __GUAC_RAIL_COMPAT_H

#include "config.h"

#include <freerdp/rail.h>

#define RailChannel_Class                  RDP_EVENT_CLASS_RAIL
#define RailChannel_ClientSystemParam      RDP_EVENT_TYPE_RAIL_CLIENT_SET_SYSPARAMS
#define RailChannel_GetSystemParam         RDP_EVENT_TYPE_RAIL_CHANNEL_GET_SYSPARAMS
#define RailChannel_ServerExecuteResult    RDP_EVENT_TYPE_RAIL_CHANNEL_EXEC_RESULTS
#define RailChannel_ServerSystemParam      RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_SYSPARAM
#define RailChannel_ServerMinMaxInfo       RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_MINMAXINFO
#define RailChannel_ServerLocalMoveSize    RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_LOCALMOVESIZE
#define RailChannel_ServerGetAppIdResponse RDP_EVENT_TYPE_RAIL_CHANNEL_APPID_RESP
#define RailChannel_ServerLanguageBarInfo  RDP_EVENT_TYPE_RAIL_CHANNEL_LANGBARINFO

#endif

