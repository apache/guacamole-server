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

#include "config.h"

#include "error.h"
#include "layer.h"
#include "object.h"
#include "palette.h"
#include "protocol.h"
#include "socket.h"
#include "stream.h"
#include "unicode.h"

#include <png.h>
#include <cairo/cairo.h>
#include <jpeglib.h>

#ifdef HAVE_PNGSTRUCT_H
#include <pngstruct.h>
#endif

#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* Output formatting functions */

ssize_t __guac_socket_write_length_string(guac_socket* socket, const char* str) {

    return
           guac_socket_write_int(socket, guac_utf8_strlen(str))
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_string(socket, str);

}

ssize_t __guac_socket_write_length_int(guac_socket* socket, int64_t i) {

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%"PRIi64, i);
    return __guac_socket_write_length_string(socket, buffer);

}

ssize_t __guac_socket_write_length_double(guac_socket* socket, double d) {

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%.16g", d);
    return __guac_socket_write_length_string(socket, buffer);

}

#ifndef HAVE_JPEG_MEM_DEST

/**
 * The number of bytes to allocate for the initial JPEG output buffer, if no
 * buffer (or an empty buffer) is provided.
 */
#define GUAC_JPEG_DEFAULT_BUFFER_SIZE 16384

/**
 * Extended version of the standard libjpeg jpeg_destination_mgr struct, which
 * provides access to the pointers to the output buffer and size. The values
 * of this structure will be initialized by jpeg_mem_dest().
 */
typedef struct guac_jpeg_destination_mgr {

    /**
     * Original jpeg_destination_mgr structure. This MUST be the first member
     * for guac_jpeg_destination_mgr to be usable as a jpeg_destination_mgr.
     */
    struct jpeg_destination_mgr parent;

    /**
     * The current output buffer.
     */
    unsigned char* buffer;

    /**
     * The size of the current output buffer, in bytes.
     */
    unsigned long size;

    /**
     * The originally-supplied (via jpeg_mem_dest()) pointer to output buffer
     * pointer.
     */
    unsigned char** outbuffer;

    /**
     * The originally-supplied (via jpeg_mem_dest()) pointer to the output
     * buffer size, in bytes.
     */
    unsigned long* outsize;

} guac_jpeg_destination_mgr;

/**
 * Initializes the destination structure of the given compression structure.
 *
 * @param cinfo
 *     The compression structure whose destination structure should be
 *     initialized.
 */
static void guac_jpeg_init_destination(j_compress_ptr cinfo) {

    guac_jpeg_destination_mgr* dest = (guac_jpeg_destination_mgr*) cinfo->dest;

    /* Init parent destination state */
    dest->parent.next_output_byte = dest->buffer;
    dest->parent.free_in_buffer   = dest->size;

}

/**
 * Re-allocates the output buffer associated with the given compression
 * structure, as the current output buffer is full.
 *
 * @param cinfo
 *     The compression structure whose output buffer should be re-allocated.
 * 
 * @return
 *     TRUE, always, indicating that space is now available. FALSE is returned
 *     only by applications that may need additional time to empty the buffer.
 */
static boolean guac_jpeg_empty_output_buffer(j_compress_ptr cinfo) {

    guac_jpeg_destination_mgr* dest = (guac_jpeg_destination_mgr*) cinfo->dest;
    unsigned long current_offset = dest->size;

    /* Double size of current buffer */
    dest->size *= 2;
    dest->buffer = realloc(dest->buffer, dest->size);

    /* Update destination offset */
    dest->parent.next_output_byte = dest->buffer + current_offset;
    dest->parent.free_in_buffer = dest->size - current_offset;

    return TRUE;

}

/**
 * Updates the given compression structure such that the originally-supplied
 * output buffer size describes the size of the output image, not the overall
 * size of the output buffer. This function is automatically called once JPEG
 * compression ends.
 *
 * @param cinfo
 *     The compression structure associated with the now-complete JPEG
 *     compression operation.
 */
static void guac_jpeg_term_destination(j_compress_ptr cinfo) {

    guac_jpeg_destination_mgr* dest = (guac_jpeg_destination_mgr*) cinfo->dest;

    /* Commit current buffer and total image size to provided pointers */
    *dest->outbuffer = dest->buffer;
    *dest->outsize   = dest->size - dest->parent.free_in_buffer;

}

/**
 * Configures the given compression structure to use the given buffer for JPEG
 * output. If no buffer is supplied, a new buffer is allocated. This is a
 * limited, internal implementation of libjpeg's jpeg_mem_dest(), which is
 * unavailable in older versions of libjpeg.
 *
 * @param cinfo
 *     The libjpeg compression structure to configure.
 *
 * @param outbuffer
 *     Pointer to a pointer to the output buffer to write JPEG data to. If the
 *     output buffer pointer is NULL, a new output buffer will be allocated and
 *     assigned.
 *
 * @param outsize
 *     Pointer to the size of the output buffer, if an output buffer is
 *     supplied. If a new output buffer is allocated, its size will be stored
 *     here. When JPEG compression is complete, the total image size (NOT
 *     buffer size) will be stored here.
 */
static void jpeg_mem_dest(j_compress_ptr cinfo,
        unsigned char** outbuffer, unsigned long* outsize) {

    guac_jpeg_destination_mgr* dest;

    /* Allocate dest from pool if not already allocated */
    if (cinfo->dest == NULL)
        cinfo->dest = (struct jpeg_destination_mgr*)
            (cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                    sizeof(guac_jpeg_destination_mgr));

    /* Pull possibly-new destination struct from cinfo */
    dest = (guac_jpeg_destination_mgr*) cinfo->dest;

    /* Allocate output buffer if necessary */
    if (*outbuffer == NULL || *outsize == 0) {
        dest->buffer = malloc(GUAC_JPEG_DEFAULT_BUFFER_SIZE);
        dest->size   = GUAC_JPEG_DEFAULT_BUFFER_SIZE;
    }

    /* Otherwise, use provided buffer */
    else {
        dest->buffer = *outbuffer;
        dest->size   = *outsize;
    }

    /* Associate destination handlers */
    dest->parent.init_destination    = guac_jpeg_init_destination;
    dest->parent.empty_output_buffer = guac_jpeg_empty_output_buffer;
    dest->parent.term_destination    = guac_jpeg_term_destination;

    /* Store provided pointers */
    dest->outbuffer = outbuffer;
    dest->outsize   = outsize;

}
#endif

/**
 * Converts the image data on a Cairo surface to a JPEG image and sends it
 * as a base64-encoded transmission on the socket.
 * 
 * @param socket
 *     The guac_socket connection to use.
 * 
 * @param surface
 *     A cairo surface containing the image data to send. 
 * 
 * @param quality
 *     JPEG image quality.
 * 
 * @return
 *     Zero on success, non-zero on error.
 */
static int __guac_socket_write_length_jpeg(guac_socket* socket, 
        cairo_surface_t* surface, int quality) {

    /* Get image surface properties and data */
    cairo_format_t format = cairo_image_surface_get_format(surface);

    if (format != CAIRO_FORMAT_RGB24) {
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "Invalid Cairo image format. Unable to create JPEG.";
        return -1;
    }

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);

    /* Flush pending operations to surface */
    cairo_surface_flush(surface);

    /* Prepare JPEG bits */
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char *mem = NULL;
    unsigned long mem_size = 0;
    jpeg_mem_dest(&cinfo, &mem, &mem_size);

    cinfo.image_width = width; /* image width and height, in pixels */
    cinfo.image_height = height;
    cinfo.arith_code = TRUE;

#ifdef JCS_EXTENSIONS
    /* The Turbo JPEG extentions allows us to use the Cairo surface
     * (BRGx) as input without converting it */
    cinfo.input_components = 4;
    cinfo.in_color_space = JCS_EXT_BGRX;
#else
    /* Standard JPEG supports RGB as input so we will have to convert
     * the contents of the Cairo surface from (BRGx) to RGB */
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    /* Create a buffer for the write scan line which is where we will
     * put the converted pixels (BRGx -> RGB) */
    int write_stride = cinfo.image_width * cinfo.input_components;
    unsigned char *scanline_data = malloc(write_stride);
    memset(scanline_data, 0, write_stride);
#endif

    /* Initialize the JPEG compressor */
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1]; /* pointer to a single row */

    /* Write scanlines to be used in JPEG compression */
    while (cinfo.next_scanline < cinfo.image_height) {

        int row_offset = stride * cinfo.next_scanline;

#ifdef JCS_EXTENSIONS
        /* In Turbo JPEG we can use the raw BGRx scanline  */
        row_pointer[0] = &data[row_offset];
#else
        /* For standard JPEG libraries we have to convert the
         * scanline from 24 bit (4 byte) BGRx to 24 bit (3 byte) RGB */
        unsigned char *inptr = data + row_offset;
        unsigned char *outptr = scanline_data;

        for (int x = 0; x < width; ++x) {

            outptr[2] = *inptr++; /* B */
            outptr[1] = *inptr++; /* G */
            outptr[0] = *inptr++; /* R */
            inptr++; /* skip the upper byte (x/A) */
            outptr += 3;

        }

        row_pointer[0] = scanline_data;
#endif

        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

#ifndef JCS_EXTENSIONS
    free(scanline_data);
#endif

    /* Finalize compression */
    jpeg_finish_compress(&cinfo);

    int base64_length = (mem_size + 2) / 3 * 4;

    /* Write length and data */
    if (guac_socket_write_int(socket, base64_length)
            || guac_socket_write_string(socket, ".")
            || guac_socket_write_base64(socket, mem, mem_size)
            || guac_socket_flush_base64(socket)) {

        /* Something failed. Clean up and return error code. */
        jpeg_destroy_compress(&cinfo);
        free(mem);

        return -1;
    }

    /* Clean up */
    jpeg_destroy_compress(&cinfo);
    free(mem);

    return 0;

}

/* PNG output formatting */

typedef struct __guac_socket_write_png_data {

    guac_socket* socket;

    char* buffer;
    int buffer_size;
    int data_size;

} __guac_socket_write_png_data;

cairo_status_t __guac_socket_write_png_cairo(void* closure, const unsigned char* data, unsigned int length) {

    __guac_socket_write_png_data* png_data = (__guac_socket_write_png_data*) closure;

    /* Calculate next buffer size */
    int next_size = png_data->data_size + length;

    /* If need resizing, double buffer size until big enough */
    if (next_size > png_data->buffer_size) {

        char* new_buffer;

        do {
            png_data->buffer_size <<= 1;
        } while (next_size > png_data->buffer_size);

        /* Resize buffer */
        new_buffer = realloc(png_data->buffer, png_data->buffer_size);
        png_data->buffer = new_buffer;

    }

    /* Append data to buffer */
    memcpy(png_data->buffer + png_data->data_size, data, length);
    png_data->data_size += length;

    return CAIRO_STATUS_SUCCESS;

}

int __guac_socket_write_length_png_cairo(guac_socket* socket, cairo_surface_t* surface) {

    __guac_socket_write_png_data png_data;
    int base64_length;

    /* Write surface */

    png_data.socket = socket;
    png_data.buffer_size = 8192;
    png_data.buffer = malloc(png_data.buffer_size);
    png_data.data_size = 0;

    if (cairo_surface_write_to_png_stream(surface, __guac_socket_write_png_cairo, &png_data) != CAIRO_STATUS_SUCCESS) {
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "Cairo PNG backend failed";
        return -1;
    }

    base64_length = (png_data.data_size + 2) / 3 * 4;

    /* Write length and data */
    if (
           guac_socket_write_int(socket, base64_length)
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_base64(socket, png_data.buffer, png_data.data_size)
        || guac_socket_flush_base64(socket)) {
        free(png_data.buffer);
        return -1;
    }

    free(png_data.buffer);
    return 0;

}

void __guac_socket_write_png(png_structp png,
        png_bytep data, png_size_t length) {

    /* Get png buffer structure */
    __guac_socket_write_png_data* png_data;
#ifdef HAVE_PNG_GET_IO_PTR
    png_data = (__guac_socket_write_png_data*) png_get_io_ptr(png);
#else
    png_data = (__guac_socket_write_png_data*) png->io_ptr;
#endif

    /* Calculate next buffer size */
    int next_size = png_data->data_size + length;

    /* If need resizing, double buffer size until big enough */
    if (next_size > png_data->buffer_size) {

        char* new_buffer;

        do {
            png_data->buffer_size <<= 1;
        } while (next_size > png_data->buffer_size);

        /* Resize buffer */
        new_buffer = realloc(png_data->buffer, png_data->buffer_size);
        png_data->buffer = new_buffer;

    }

    /* Append data to buffer */
    memcpy(png_data->buffer + png_data->data_size, data, length);
    png_data->data_size += length;

}

void __guac_socket_flush_png(png_structp png) {
    /* Dummy function */
}

int __guac_socket_write_length_png(guac_socket* socket, cairo_surface_t* surface) {

    png_structp png;
    png_infop png_info;
    png_byte** png_rows;
    int bpp;

    int x, y;

    __guac_socket_write_png_data png_data;
    int base64_length;

    /* Get image surface properties and data */
    cairo_format_t format = cairo_image_surface_get_format(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);

    /* If not RGB24, use Cairo PNG writer */
    if (format != CAIRO_FORMAT_RGB24 || data == NULL)
        return __guac_socket_write_length_png_cairo(socket, surface);

    /* Flush pending operations to surface */
    cairo_surface_flush(surface);

    /* Attempt to build palette */
    guac_palette* palette = guac_palette_alloc(surface);

    /* If not possible, resort to Cairo PNG writer */
    if (palette == NULL)
        return __guac_socket_write_length_png_cairo(socket, surface);

    /* Calculate BPP from palette size */
    if      (palette->size <= 2)  bpp = 1;
    else if (palette->size <= 4)  bpp = 2;
    else if (palette->size <= 16) bpp = 4;
    else                          bpp = 8;

    /* Set up PNG writer */
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "libpng failed to create write structure";
        return -1;
    }

    png_info = png_create_info_struct(png);
    if (!png_info) {
        png_destroy_write_struct(&png, NULL);
        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "libpng failed to create info structure";
        return -1;
    }

    /* Set error handler */
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &png_info);
        guac_error = GUAC_STATUS_IO_ERROR;
        guac_error_message = "libpng output error";
        return -1;
    }

    /* Set up buffer structure */
    png_data.socket = socket;
    png_data.buffer_size = 8192;
    png_data.buffer = malloc(png_data.buffer_size);
    png_data.data_size = 0;

    /* Set up writer */
    png_set_write_fn(png, &png_data,
            __guac_socket_write_png,
            __guac_socket_flush_png);

    /* Copy data from surface into PNG data */
    png_rows = (png_byte**) malloc(sizeof(png_byte*) * height);
    for (y=0; y<height; y++) {

        /* Allocate new PNG row */
        png_byte* row = (png_byte*) malloc(sizeof(png_byte) * width);
        png_rows[y] = row;

        /* Copy data from surface into current row */
        for (x=0; x<width; x++) {

            /* Get pixel color */
            int color = ((uint32_t*) data)[x] & 0xFFFFFF;

            /* Set index in row */
            row[x] = guac_palette_find(palette, color);

        }

        /* Advance to next data row */
        data += stride;

    }

    /* Write image info */
    png_set_IHDR(
        png,
        png_info,
        width,
        height,
        bpp,
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    /* Write palette */
    png_set_PLTE(png, png_info, palette->colors, palette->size);

    /* Write image */
    png_set_rows(png, png_info, png_rows);
    png_write_png(png, png_info, PNG_TRANSFORM_PACKING, NULL);

    /* Finish write */
    png_destroy_write_struct(&png, &png_info);

    /* Free palette */
    guac_palette_free(palette);

    /* Free PNG data */
    for (y=0; y<height; y++)
        free(png_rows[y]);
    free(png_rows);

    base64_length = (png_data.data_size + 2) / 3 * 4;

    /* Write length and data */
    if (
           guac_socket_write_int(socket, base64_length)
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_base64(socket, png_data.buffer, png_data.data_size)
        || guac_socket_flush_base64(socket)) {
        free(png_data.buffer);
        return -1;
    }

    free(png_data.buffer);
    return 0;

}

/* Protocol functions */

int guac_protocol_send_ack(guac_socket* socket, guac_stream* stream,
        const char* error, guac_protocol_status status) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.ack,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, error)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, status)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

static int __guac_protocol_send_args(guac_socket* socket, const char** args) {

    int i;

    if (guac_socket_write_string(socket, "4.args")) return -1;

    for (i=0; args[i] != NULL; i++) {

        if (guac_socket_write_string(socket, ","))
            return -1;

        if (__guac_socket_write_length_string(socket, args[i]))
            return -1;

    }

    return guac_socket_write_string(socket, ";");

}

int guac_protocol_send_args(guac_socket* socket, const char** args) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = __guac_protocol_send_args(socket, args);
    guac_socket_instruction_end(socket);

    return ret_val;

}

int guac_protocol_send_arc(guac_socket* socket, const guac_layer* layer,
        int x, int y, int radius, double startAngle, double endAngle,
        int negative) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.arc,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, radius)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, startAngle)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, endAngle)
        || guac_socket_write_string(socket, ",")
        || guac_socket_write_string(socket, negative ? "1.1" : "1.0")
        || guac_socket_write_string(socket, ";");
    guac_socket_instruction_end(socket);

    return ret_val;

}

int guac_protocol_send_audio(guac_socket* socket, const guac_stream* stream,
        int channel, const char* mimetype, double duration) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = 
           guac_socket_write_string(socket, "5.audio,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, channel)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, mimetype)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, duration)
        || guac_socket_write_string(socket, ";");
    guac_socket_instruction_end(socket);

    return ret_val;

}

int guac_protocol_send_blob(guac_socket* socket, const guac_stream* stream,
        void* data, int count) {

    int base64_length = (count + 2) / 3 * 4;

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.blob,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || guac_socket_write_int(socket, base64_length)
        || guac_socket_write_string(socket, ".")
        || guac_socket_write_base64(socket, data, count)
        || guac_socket_flush_base64(socket)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_body(guac_socket* socket, const guac_object* object,
        const guac_stream* stream, const char* mimetype, const char* name) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.body,")
        || __guac_socket_write_length_int(socket, object->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, mimetype)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_cfill(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        int r, int g, int b, int a) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.cfill,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, r)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, g)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, a)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_close(guac_socket* socket, const guac_layer* layer) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.close,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

static int __guac_protocol_send_connect(guac_socket* socket, const char** args) {

    int i;

    if (guac_socket_write_string(socket, "7.connect")) return -1;

    for (i=0; args[i] != NULL; i++) {

        if (guac_socket_write_string(socket, ","))
            return -1;

        if (__guac_socket_write_length_string(socket, args[i]))
            return -1;

    }

    return guac_socket_write_string(socket, ";");

}

int guac_protocol_send_connect(guac_socket* socket, const char** args) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = __guac_protocol_send_connect(socket, args);
    guac_socket_instruction_end(socket);

    return ret_val;

}

int guac_protocol_send_clip(guac_socket* socket, const guac_layer* layer) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.clip,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_clipboard(guac_socket* socket, const guac_stream* stream,
        const char* mimetype) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "9.clipboard,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, mimetype)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_copy(guac_socket* socket,
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_composite_mode mode, const guac_layer* dstl, int dstx, int dsty) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.copy,")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcy)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dsty)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_cstroke(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        guac_line_cap_style cap, guac_line_join_style join, int thickness,
        int r, int g, int b, int a) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "7.cstroke,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cap)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, join)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, thickness)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, r)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, g)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, a)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_cursor(guac_socket* socket, int x, int y,
        const guac_layer* srcl, int srcx, int srcy, int w, int h) {
    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "6.cursor,")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcy)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_curve(guac_socket* socket, const guac_layer* layer,
        int cp1x, int cp1y, int cp2x, int cp2y, int x, int y) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.curve,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp1x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp1y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp2x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cp2y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_disconnect(guac_socket* socket) {
    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = guac_socket_write_string(socket, "10.disconnect;");
    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_dispose(guac_socket* socket, const guac_layer* layer) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "7.dispose,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_distort(guac_socket* socket, const guac_layer* layer,
        double a, double b, double c,
        double d, double e, double f) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = 
           guac_socket_write_string(socket, "7.distort,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, a)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, c)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, d)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, e)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, f)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_end(guac_socket* socket, const guac_stream* stream) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.end,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_error(guac_socket* socket, const char* error,
        guac_protocol_status status) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.error,")
        || __guac_socket_write_length_string(socket, error)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, status)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int vguac_protocol_send_log(guac_socket* socket, const char* format,
        va_list args) {

    int ret_val;

    /* Copy log message into buffer */
    char message[4096];
    vsnprintf(message, sizeof(message), format, args);

    /* Log to instruction */
    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.log,")
        || __guac_socket_write_length_string(socket, message)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_log(guac_socket* socket, const char* format, ...) {

    int ret_val;

    va_list args;
    va_start(args, format);
    ret_val = vguac_protocol_send_log(socket, format, args);
    va_end(args);

    return ret_val;

}

int guac_protocol_send_file(guac_socket* socket, const guac_stream* stream,
        const char* mimetype, const char* name) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.file,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, mimetype)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_filesystem(guac_socket* socket,
        const guac_object* object, const char* name) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "10.filesystem,")
        || __guac_socket_write_length_int(socket, object->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_identity(guac_socket* socket, const guac_layer* layer) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "8.identity,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_lfill(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        const guac_layer* srcl) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.lfill,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_line(guac_socket* socket, const guac_layer* layer,
        int x, int y) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.line,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_lstroke(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        guac_line_cap_style cap, guac_line_join_style join, int thickness,
        const guac_layer* srcl) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "7.lstroke,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, cap)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, join)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, thickness)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_move(guac_socket* socket, const guac_layer* layer,
        const guac_layer* parent, int x, int y, int z) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.move,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, parent->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, z)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_name(guac_socket* socket, const char* name) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.name,")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_nest(guac_socket* socket, int index,
        const char* data) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.nest,")
        || __guac_socket_write_length_int(socket, index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, data)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_nop(guac_socket* socket) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = guac_socket_write_string(socket, "3.nop;");
    guac_socket_instruction_end(socket);

    return ret_val;

}

int guac_protocol_send_pipe(guac_socket* socket, const guac_stream* stream,
        const char* mimetype, const char* name) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.pipe,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, mimetype)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_jpeg(guac_socket* socket, guac_composite_mode mode,
        const guac_layer* layer, int x, int y, cairo_surface_t* surface,
        int quality) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.jpeg,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_jpeg(socket, surface, quality)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_png(guac_socket* socket, guac_composite_mode mode,
        const guac_layer* layer, int x, int y, cairo_surface_t* surface) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.png,")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_png(socket, surface)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_img(guac_socket* socket, const guac_stream* stream,
        guac_composite_mode mode, const guac_layer* layer,
        const char* mimetype, int x, int y) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.img,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, mode)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, mimetype)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_pop(guac_socket* socket, const guac_layer* layer) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.pop,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_push(guac_socket* socket, const guac_layer* layer) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.push,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_ready(guac_socket* socket, const char* id) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.ready,")
        || __guac_socket_write_length_string(socket, id)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_rect(guac_socket* socket,
        const guac_layer* layer, int x, int y, int width, int height) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.rect,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, width)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, height)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_reset(guac_socket* socket, const guac_layer* layer) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.reset,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_set(guac_socket* socket, const guac_layer* layer,
        const char* name, const char* value) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "3.set,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, name)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, value)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_select(guac_socket* socket, const char* protocol) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "6.select,")
        || __guac_socket_write_length_string(socket, protocol)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_shade(guac_socket* socket, const guac_layer* layer,
        int a) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.shade,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, a)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_size(guac_socket* socket, const guac_layer* layer,
        int w, int h) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "4.size,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_start(guac_socket* socket, const guac_layer* layer,
        int x, int y) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "5.start,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, x)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, y)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_sync(guac_socket* socket, guac_timestamp timestamp) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = 
           guac_socket_write_string(socket, "4.sync,")
        || __guac_socket_write_length_int(socket, timestamp)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_transfer(guac_socket* socket,
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_transfer_function fn, const guac_layer* dstl, int dstx, int dsty) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "8.transfer,")
        || __guac_socket_write_length_int(socket, srcl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, srcy)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, w)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, h)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, fn)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstl->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dstx)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, dsty)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_transform(guac_socket* socket, const guac_layer* layer,
        double a, double b, double c,
        double d, double e, double f) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = 
           guac_socket_write_string(socket, "9.transform,")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, a)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, b)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, c)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, d)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, e)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, f)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_undefine(guac_socket* socket,
        const guac_object* object) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val =
           guac_socket_write_string(socket, "8.undefine,")
        || __guac_socket_write_length_int(socket, object->index)
        || guac_socket_write_string(socket, ";");

    guac_socket_instruction_end(socket);
    return ret_val;

}

int guac_protocol_send_video(guac_socket* socket, const guac_stream* stream,
        const guac_layer* layer, const char* mimetype, double duration) {

    int ret_val;

    guac_socket_instruction_begin(socket);
    ret_val = 
           guac_socket_write_string(socket, "5.video,")
        || __guac_socket_write_length_int(socket, stream->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_int(socket, layer->index)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_string(socket, mimetype)
        || guac_socket_write_string(socket, ",")
        || __guac_socket_write_length_double(socket, duration)
        || guac_socket_write_string(socket, ";");
    guac_socket_instruction_end(socket);

    return ret_val;

}

/**
 * Returns the value of a single base64 character.
 */
static int __guac_base64_value(char c) {

    if (c >= 'A' && c <= 'Z')
        return c - 'A';

    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;

    if (c >= '0' && c <= '9')
        return c - '0' + 52;

    if (c == '+')
        return 62;

    if (c == '/')
        return 63;

    return 0;

}

int guac_protocol_decode_base64(char* base64) {

    char* input = base64;
    char* output = base64;

    int length = 0;
    int bits_read = 0;
    int value = 0;
    char current;

    /* For all characters in string */
    while ((current = *(input++)) != 0) {

        /* If we've reached padding, then we're done */
        if (current == '=')
            break;

        /* Otherwise, shift on the latest 6 bits */
        value = (value << 6) | __guac_base64_value(current);
        bits_read += 6;

        /* If we have at least one byte, write out the latest whole byte */
        if (bits_read >= 8) {
            *(output++) = (value >> (bits_read % 8)) & 0xFF;
            bits_read -= 8;
            length++;
        }

    }

    /* Return number of bytes written */
    return length;

}

