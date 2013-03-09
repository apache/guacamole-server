
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-vnc.
 *
 * The Initial Developer of the Original Code is
 * James Muehlner.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>

char* convert(const char* from_charset, const char* to_charset, char* input) {
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
	input_buffer = input;
 
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
			    /* Cannot convert because an invalid sequence was discovered */
			    iconv_close(cd);
			    free(output);
			    return NULL;
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

