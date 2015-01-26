/*
 * Copyright (C) 2015 Glyptodon LLC
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

#include "config.h"
#include "scrollbar.h"

#include <guacamole/client.h>
#include <guacamole/layer.h>

#include <stdlib.h>

guac_terminal_scrollbar* guac_terminal_scrollbar_alloc(guac_client* client,
        guac_layer* parent, int parent_width, int parent_height) {
    /* STUB */
    return NULL;
}

void guac_terminal_scrollbar_free(guac_terminal_scrollbar* scrollbar) {
    /* STUB */
}

void guac_terminal_scrollbar_set_bounds(guac_terminal_scrollbar* scrollbar,
        int min, int max) {
    /* STUB */
}

void guac_terminal_scrollbar_set_value(guac_terminal_scrollbar* scrollbar,
        int value) {
    /* STUB */
}

void guac_terminal_scrollbar_parent_resized(guac_terminal_scrollbar* scrollbar,
        int parent_width, int parent_height) {
    /* STUB */
}

