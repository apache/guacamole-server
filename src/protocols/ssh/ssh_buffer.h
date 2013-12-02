
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

