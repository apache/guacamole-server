
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
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
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

#ifndef _GUAC_SSH_BUFFER_H
#define _GUAC_SSH_BUFFER_H

#include <stdint.h>
#include <openssl/bn.h>

/**
 * Writes the given byte to the given buffer, advancing the buffer pointer by
 * one byte.
 */
void buffer_write_byte(char** buffer, uint8_t value);

/**
 * Writes the given integer to the given buffer, advancing the buffer pointer
 * four bytes.
 */
void buffer_write_uint32(char** buffer, uint32_t value);

/**
 * Writes the given string and its length to the given buffer, advancing the
 * buffer pointer by the size of the length (four bytes) and the size of the
 * string.
 */
void buffer_write_string(char** buffer, const char* string, int length);

/**
 * Writes the given BIGNUM the given buffer, advancing the buffer pointer by
 * the size of the length (four bytes) and the size of the BIGNUM.
 */
void buffer_write_bignum(char** buffer, BIGNUM* value);

/**
 * Writes the given data the given buffer, advancing the buffer pointer by the
 * given length.
 */
void buffer_write_data(char** buffer, const char* data, int length);

/**
 * Reads a single byte from the given buffer, advancing the buffer by one byte.
 */
uint8_t buffer_read_byte(char** buffer);

/**
 * Reads an integer from the given buffer, advancing the buffer by four bytes.
 */
uint32_t buffer_read_uint32(char** buffer);

/**
 * Reads a string and its length from the given buffer, advancing the buffer
 * by the size of the length (four bytes) and the size of the string, and
 * returning a pointer to the buffer. The length of the string is stored in
 * the given int.
 */
char* buffer_read_string(char** buffer, int* length);

#endif

