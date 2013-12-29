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


#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>

char* convert(const char* from_charset, const char* to_charset, const char* input) {
    size_t input_remaining;
    size_t output_remaining;
    size_t bytes_converted = 0;
    char* output;
    char* output_buffer;
    char* new_buffer;
    char* input_buffer;
    size_t output_length;
    iconv_t cd;
 
    cd = iconv_open(to_charset, from_charset);
    
    if(cd == (iconv_t) -1)
        /* Cannot convert due to invalid character set */
        return NULL;
 
    input_remaining = strlen(input);
    input_buffer = (char*) input;
 
    /* Start the output buffer the same size as the input buffer */
    output_length = input_remaining;
 
    /* Leave some space at the end for NULL terminator */
    if (!(output = (char*) malloc(output_length + 4))) {
        /* Cannot convert due to memory allocation error */
        iconv_close(cd);
        return NULL;
    }
 
    do {
        output_buffer = output + bytes_converted;
        output_remaining = output_length - bytes_converted;
 
        bytes_converted = iconv(cd, &input_buffer, 
                &input_remaining, &output_buffer, &output_remaining);

        if(bytes_converted == -1) {
            if(errno == E2BIG) {
                /* The output buffer is too small, so allocate more space */
                bytes_converted = output_buffer - output;
                output_length += input_remaining * 2 + 8;
         
                if (!(new_buffer = (char*) realloc(output, output_length + 4))) {
                    /* Cannot convert due to memory allocation error */
                    iconv_close(cd);
                    free(output);
                    return NULL;
                }
         
                output = new_buffer;
                output_buffer = output + bytes_converted;
            } 
            else if (errno == EILSEQ) {
                /* Invalid sequence detected, return what's been converted so far */
                break;
            } 
            else if (errno == EINVAL) {
                /* Incomplete sequence detected, can be ignored */
                break;
            }
        }
    } while (input_remaining);
 
    /* Flush the iconv conversion */
    iconv(cd, NULL, NULL, &output_buffer, &output_remaining);
    iconv_close(cd);
 
    /* Add the NULL terminator */
    memset(output_buffer, 0, 4);
 
    return output;
}

