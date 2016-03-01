/*
 * Copyright (C) 2014 Glyptodon LLC
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

#ifndef GUAC_RDP_RESOLUTION_H
#define GUAC_RDP_RESOLUTION_H

#include <guacamole/user.h>

/**
 * Returns whether the given resolution is reasonable for the given user,
 * based on arbitrary criteria for reasonability.
 *
 * @param user The guac_user to test the given resolution against.
 * @param resolution The resolution to test, in DPI.
 * @return Non-zero if the resolution is reasonable, zero otherwise.
 */
int guac_rdp_resolution_reasonable(guac_user* user, int resolution);

/**
 * Returns a reasonable resolution for the remote display, given the size and
 * resolution of a guac_user.
 *
 * @param user The guac_user whose size and resolution shall be used to
 *             determine an appropriate remote display resolution.
 * @return A reasonable resolution for the remote display, in DPI.
 */
int guac_rdp_suggest_resolution(guac_user* user);

#endif

