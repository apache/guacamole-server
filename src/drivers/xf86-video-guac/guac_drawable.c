
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
#include "guac_drawable.h"
#include "guac_rect.h"
#include "list.h"

#include <xf86.h>

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <cairo/cairo.h>

void guac_drv_drawable_stub(guac_drv_drawable* drawable, int dx, int dy,
        int w, int h) {

    guac_drv_rect dst_rect;
    guac_drv_rect boundary_rect;

    int x, y;

    int r = rand() & 0xFF0000;
    int g = rand() & 0x00FF00;
    int b = rand() & 0x0000FF;

    uint32_t colorA = 0xFF000000 | r | g | b;
    uint32_t colorB =   0xFF000000
                      | ((r*7/8) & 0xFF0000)
                      | ((g*7/8) & 0x00FF00)
                      | ((b*7/8) & 0x0000FF);

    unsigned char* row;

    /* Get rects */
    guac_drv_rect_init(&dst_rect,      dx, dy, w, h);
    guac_drv_rect_init(&boundary_rect, 0,  0, drawable->pending.rect.width, drawable->pending.rect.height);

    /* Trim rectangle to boundary */
    guac_drv_rect_shrink(&dst_rect, &boundary_rect);

    row = (unsigned char*) drawable->operations
                       + dy*drawable->operations_stride
                       + dx*sizeof(guac_drv_drawable_operation);

    /* Write each stub pixel as a new SET operation */
    for (y=0; y<dst_rect.height; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<dst_rect.width; x++) {

            current->type = GUAC_DRV_DRAWABLE_SET;
            current->order = drawable->operations_pending;

            /* Choose color based on which checker we're in */
            if (((x >> 5) ^ (y >> 5)) & 0x1)
                current->color = colorA;
            else
                current->color = colorB;

            /* Next operation */
            current++;

        }

        row += drawable->operations_stride;

    }

    /* Drawable modified */
    guac_drv_rect_extend(&drawable->dirty, &dst_rect);

}


guac_drv_drawable* guac_drv_drawable_alloc(guac_drv_drawable_type type,
        guac_drv_drawable* parent, int x, int y, int z,
        int width, int height,
        int opacity, int online) {

    guac_drv_drawable* drawable = malloc(sizeof(guac_drv_drawable));

    /* Init basic descriptive values */
    drawable->type = type;
    drawable->realized = 0;
    drawable->operations_pending = 0;
    drawable->index = 0;
    guac_drv_rect_clear(&drawable->dirty);
    drawable->rows = height;
    drawable->image_stride =
        cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    drawable->operations_stride = drawable->image_stride / 4 *
        sizeof(guac_drv_drawable_operation);

    /* Init state */
    drawable->current.parent = parent;
    drawable->current.z = z;
    drawable->current.opacity = opacity;
    guac_drv_rect_init(&drawable->current.rect, x, y, width, height);
    drawable->pending = drawable->current;

    /* Init sync state */
    if (online)
        drawable->sync_state = GUAC_DRV_DRAWABLE_NEW;
    else
        drawable->sync_state = GUAC_DRV_DRAWABLE_OFFLINE;

    /* Init mutex */
    pthread_mutex_init(&(drawable->lock), NULL);

    /* Create surface */
    drawable->image_data = calloc(height, drawable->image_stride);
    drawable->surface =
        cairo_image_surface_create_for_data(drawable->image_data,
                CAIRO_FORMAT_ARGB32, width, height, drawable->image_stride);

    /* Create operations buffer */
    drawable->operations = calloc(height, drawable->operations_stride);

    return drawable;
}

void guac_drv_drawable_free(guac_drv_drawable* drawable) {
    pthread_mutex_destroy(&(drawable->lock));
    cairo_surface_destroy(drawable->surface);
    free(drawable->image_data);
    free(drawable);
}

void guac_drv_drawable_lock(guac_drv_drawable* drawable) {
    pthread_mutex_lock(&(drawable->lock));
}

void guac_drv_drawable_unlock(guac_drv_drawable* drawable) {
    pthread_mutex_unlock(&(drawable->lock));
}

static void __guac_drv_copy_rect(void* dst, int dst_stride,
                                 void* src, int src_stride,
                                 int row_size, int row_count) {

    int i;
    unsigned char* src_row = (unsigned char*) src;
    unsigned char* dst_row = (unsigned char*) dst;

    /* Copy data */
    for (i=0; i<row_count; i++) {
        memcpy(dst_row, src_row, row_size);
        src_row += src_stride;
        dst_row += dst_stride;
    }

}

void guac_drv_drawable_resize(guac_drv_drawable* drawable,
        int width, int height) {

    guac_drv_drawable_lock(drawable);

    /* Resize backing surface if necessary */
    if (width*4 > drawable->image_stride || height > drawable->rows) {

        int min_width, min_height;

        /* Create new surface */
        int new_image_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
        unsigned char* new_image = calloc(height, new_image_stride);

        /* Create new operations buffer */
        int new_operations_stride = new_image_stride / 4 * sizeof(guac_drv_drawable_operation);
        guac_drv_drawable_operation* new_operations = calloc(height, new_operations_stride);

        /* Calculate height of relevant image data */
        if (drawable->pending.rect.height < height)
            min_height = drawable->pending.rect.height;
        else
            min_height = height;

        /* Calculate width of relevant image data */
        if (drawable->pending.rect.width < width)
            min_width = drawable->pending.rect.width;
        else
            min_width = width;

        /* Copy data from old surface */
        __guac_drv_copy_rect(new_image, new_image_stride,
                             drawable->image_data, drawable->image_stride,
                             min_width*4, min_height);

         /* Copy data from old operations */
        __guac_drv_copy_rect(new_operations, new_operations_stride,
                             drawable->operations, drawable->operations_stride,
                             min_width*sizeof(guac_drv_drawable_operation),
                             min_height);
        
        /* Free old data */
        cairo_surface_destroy(drawable->surface);
        free(drawable->image_data);
        free(drawable->operations);

        /* Set new data */
        drawable->rows = height;

        drawable->operations_stride = new_operations_stride;
        drawable->operations = new_operations;

        drawable->image_stride = new_image_stride;
        drawable->image_data = new_image;
        drawable->surface = cairo_image_surface_create_for_data(new_image,
                    CAIRO_FORMAT_ARGB32, width, height, new_image_stride);

    }

    /* Set new dimensions */
    drawable->pending.rect.width = width;
    drawable->pending.rect.height = height;

    guac_drv_drawable_unlock(drawable);

}

/**
 * 32bpp-specific PutImage
 */
static void _guac_drv_drawable_put32(guac_drv_drawable* drawable,
        char* data, int stride, int dx, int dy, int w, int h,
        guac_drv_rect* dirty) {

    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + dy*drawable->operations_stride
                       + dx*sizeof(guac_drv_drawable_operation);

    uint32_t* pixel = (uint32_t*) data;

    /* Overall bounds */
    int max_x = 0;
    int max_y = 0;
    int min_x = w;
    int min_y = h;
 
    /* Copy each pixel as a new SET operation */
    for (y=0; y<h; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<w; x++) {

            int new_color = *pixel;
            int old_color = current->old_color;

            /* If color different, set as SET */
            if (new_color != old_color) {
                current->type = GUAC_DRV_DRAWABLE_SET;
                current->order = drawable->operations_pending;
                current->color = new_color;

                /* Update bounds */
                if (x > max_x) max_x = x;
                if (x < min_x) min_x = x;
                if (y > max_y) max_y = y;
                if (y < min_y) min_y = y;

            }

            /* Otherwise, no operation */
            else {
                current->type = GUAC_DRV_DRAWABLE_NOP;
                current->order = drawable->operations_pending;
                current->color = old_color;
            }

            /* Next pixel/operation */
            pixel++;
            current++;

        }

        row += drawable->operations_stride;

    }

    /* Save real dirty rect */
    if (max_x > min_x && max_y > min_y)
        guac_drv_rect_init(dirty,
                dx+min_x, dy+min_y,
                max_x - min_x + 1, max_y - min_y + 1);
    else
        guac_drv_rect_clear(dirty);

}

/**
 * 24bpp-specific PutImage
 */
static void _guac_drv_drawable_put24(guac_drv_drawable* drawable,
        char* data, int stride, int dx, int dy, int w, int h,
        guac_drv_rect* dirty) {

    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + dy*drawable->operations_stride
                       + dx*sizeof(guac_drv_drawable_operation);

    uint32_t* pixel = (uint32_t*) data;

    /* Overall bounds */
    int max_x = 0;
    int max_y = 0;
    int min_x = w;
    int min_y = h;

    /* Copy each pixel as a new SET operation */
    for (y=0; y<h; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<w; x++) {

            int new_color = *pixel | 0xFF000000;
            int old_color = current->old_color;

            /* If color different, set as SET */
            if (new_color != old_color) {
                current->type = GUAC_DRV_DRAWABLE_SET;
                current->order = drawable->operations_pending;
                current->color = new_color;

                /* Update bounds */
                if (x > max_x) max_x = x;
                if (x < min_x) min_x = x;
                if (y > max_y) max_y = y;
                if (y < min_y) min_y = y;

            }

            /* Otherwise, no operation */
            else {
                current->type = GUAC_DRV_DRAWABLE_NOP;
                current->color = old_color;
            }

            /* Next pixel/operation */
            pixel++;
            current++;

        }

        row += drawable->operations_stride;

    }

    /* Save real dirty rect */
    if (max_x > min_x && max_y > min_y)
        guac_drv_rect_init(dirty,
                dx+min_x, dy+min_y,
                max_x - min_x + 1, max_y - min_y + 1);
    else
        guac_drv_rect_clear(dirty);

}

/**
 * Mark all pixels within a rectangle as SET.
 */
static void _guac_drv_drawable_mark_set(guac_drv_drawable* drawable,
        guac_drv_rect* dirty) {

    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + dirty->y*drawable->operations_stride
                       + dirty->x*sizeof(guac_drv_drawable_operation);

    /* Mark each operation as a new SET operation */
    for (y=0; y<dirty->height; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<dirty->width; x++) {
            current->type = GUAC_DRV_DRAWABLE_SET;
            current->order = drawable->operations_pending;
            current++;
        }

        row += drawable->operations_stride;

    }

}


void guac_drv_drawable_put(guac_drv_drawable* drawable,
        char* data, guac_drv_drawable_format format, int stride,
        int dx, int dy, int w, int h) {

    guac_drv_rect dirty;
    guac_drv_rect dst_rect;
    guac_drv_rect boundary_rect;

    guac_drv_drawable_lock(drawable);

    /* Get rects */
    guac_drv_rect_init(&dst_rect,      dx, dy, w, h);
    guac_drv_rect_init(&boundary_rect, 0,  0, drawable->pending.rect.width, drawable->pending.rect.height);

    /* Trim rectangle to boundary */
    guac_drv_rect_shrink(&dst_rect, &boundary_rect);

    /* Call appropriate format-specific implementation */
    switch (format) {

        /* 32bpp */
        case GUAC_DRV_DRAWABLE_ARGB_32:
            _guac_drv_drawable_put32(drawable, data, stride,
                    dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height,
                    &dirty);
            break;

        /* 24bpp */
        case GUAC_DRV_DRAWABLE_RGB_24:
            _guac_drv_drawable_put24(drawable, data, stride,
                    dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height,
                    &dirty);
            break;

        /* Use stub by default */
        default:
            guac_drv_drawable_stub(drawable,
                    dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height);
            guac_drv_rect_init(&dirty,
                    dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height);

    }

    /* Set entire rectangle */
    _guac_drv_drawable_mark_set(drawable, &dirty);

    /* Drawable modified */
    guac_drv_rect_extend(&drawable->dirty, &dirty);

    /* One more operation pending */
    drawable->operations_pending++;

    guac_drv_drawable_unlock(drawable);

}

void guac_drv_drawable_copy(
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty) {

    /* If not same drawable, perform simple copy */
    if (src != dst) {
        xf86Msg(X_INFO, "guac: stub: copy (simple)\n");
    }

    /* Otherwise, perform move */
    else {

        guac_drv_rect dirty;

        int x,  y;
        int sx, sy;
        int ex, ey;

        int src_stride;
        int dst_stride;
        int direction;

        unsigned char* src_row;
        unsigned char* dst_row;

        guac_drv_drawable_lock(dst);

        /* If destination follows source, must copy backwards */
        if (dsty > srcy || (dsty == srcy && dstx > srcx)) {

            /* Position at end of each buffer */
            src_row = ((unsigned char*) src->operations)
                    + (srcy + h - 1)*src->operations_stride
                    + (srcx + w - 1)*sizeof(guac_drv_drawable_operation);

            dst_row = ((unsigned char*) dst->operations)
                    + (dsty + h - 1)*dst->operations_stride
                    + (dstx + w - 1)*sizeof(guac_drv_drawable_operation);

            /* Move backwards */
            src_stride = -src->operations_stride;
            dst_stride = -dst->operations_stride;

            ex = srcx - 1;
            ey = srcy - 1;
            sx = ex + w;
            sy = ey + h;

            direction = -1;

        }

        /* Otherwise, forwards */
        else {

            /* Position at beginning of each buffer */
            src_row = ((unsigned char*) src->operations)
                    + srcy*src->operations_stride
                    + srcx*sizeof(guac_drv_drawable_operation);

            dst_row = ((unsigned char*) dst->operations)
                    + dsty*dst->operations_stride
                    + dstx*sizeof(guac_drv_drawable_operation);

            /* Move forwards */
            src_stride = src->operations_stride;
            dst_stride = dst->operations_stride;

            sx = srcx;
            sy = srcy;
            ex = sx + w;
            ey = sy + h;

            direction = 1;

        }

        /* For each row */
        for (y=sy; y != ey; y += direction) {

            guac_drv_drawable_operation* src_op = (guac_drv_drawable_operation*) src_row;
            guac_drv_drawable_operation* dst_op = (guac_drv_drawable_operation*) dst_row;

            /* For each operation in row */
            for (x=sx; x != ex; x += direction) {

                /* If not NOP, just copy operation */
                if (src_op->type != GUAC_DRV_DRAWABLE_NOP) {
                    dst_op->type   = src_op->type;
                    dst_op->order  = dst->operations_pending;
                    dst_op->color  = src_op->color;
                    dst_op->source = src_op->source;
                    dst_op->x      = src_op->x;
                    dst_op->y      = src_op->y;
                }

                /* Otherwise, set copy */
                else {
                    dst_op->type   = GUAC_DRV_DRAWABLE_COPY;
                    dst_op->order  = dst->operations_pending;
                    dst_op->color  = src_op->color;
                    dst_op->source = src;
                    dst_op->x      = x;
                    dst_op->y      = y;
                }

                /* Next operation */
                src_op += direction;
                dst_op += direction;
            }

            /* Next row */
            src_row += src_stride;
            dst_row += dst_stride;

        }

        /* Mark dirty */
        guac_drv_rect_init(&dirty, dstx, dsty, w, h);
        guac_drv_rect_extend(&dst->dirty, &dirty);

        /* One more operation pending */
        dst->operations_pending++;

        guac_drv_drawable_unlock(dst);

    }

}

void guac_drv_drawable_shade(guac_drv_drawable* drawable, int opacity) {
    guac_drv_drawable_lock(drawable);
    drawable->pending.opacity = opacity;
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_move(guac_drv_drawable* drawable, int x, int y) {
    guac_drv_drawable_lock(drawable);
    drawable->pending.rect.x = x;
    drawable->pending.rect.y = y;
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_stack(guac_drv_drawable* drawable, int z) {
    guac_drv_drawable_lock(drawable);
    drawable->pending.z = z;
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_reparent(guac_drv_drawable* drawable,
        guac_drv_drawable* parent) {
    guac_drv_drawable_lock(drawable);
    drawable->pending.parent = parent;
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_destroy(guac_drv_drawable* drawable) {
    guac_drv_drawable_lock(drawable);
    drawable->sync_state = GUAC_DRV_DRAWABLE_DESTROYED;
    guac_drv_drawable_unlock(drawable);
}

