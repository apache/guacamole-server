
#include <stdint.h>
#include <string.h>

#include <openssl/bn.h>

void buffer_write_byte(char** buffer, uint8_t value) {

    uint8_t* data = (uint8_t*) *buffer;
    *data = value;

    (*buffer)++;

}

void buffer_write_uint32(char** buffer, uint32_t value) {

    uint8_t* data = (uint8_t*) *buffer;

    data[0] = (value & 0xFF000000) >> 24;
    data[1] = (value & 0x00FF0000) >> 16;
    data[2] = (value & 0x0000FF00) >> 8;
    data[3] =  value & 0x000000FF;

    *buffer += 4;

}

void buffer_write_data(char** buffer, const char* data, int length) {
    memcpy(*buffer, data, length);
    *buffer += length;
}

void buffer_write_bignum(char** buffer, BIGNUM* value) {

    unsigned char* bn_buffer;
    int length;

    /* If zero, just write zero length */
    if (BN_is_zero(value)) {
        buffer_write_uint32(buffer, 0);
        return;
    }

    /* Allocate output buffer, add padding byte */
    length = BN_num_bytes(value);
    bn_buffer = malloc(length);

    /* Convert BIGNUM */
    BN_bn2bin(value, bn_buffer);

    /* If first byte has high bit set, write padding byte */
    if (bn_buffer[0] & 0x80) {
        buffer_write_uint32(buffer, length+1);
        buffer_write_byte(buffer, 0);
    }
    else
        buffer_write_uint32(buffer, length);

    /* Write data */
    memcpy(*buffer, bn_buffer, length);
    *buffer += length;

    free(bn_buffer);

}

void buffer_write_string(char** buffer, const char* string, int length) {
    buffer_write_uint32(buffer, length);
    buffer_write_data(buffer, string, length);
}

uint8_t buffer_read_byte(char** buffer) {

    uint8_t* data = (uint8_t*) *buffer;
    uint8_t value = *data;

    (*buffer)++;

    return value;

}

uint32_t buffer_read_uint32(char** buffer) {

    uint8_t* data = (uint8_t*) *buffer;
    uint32_t value =
          (data[0] << 24)
        | (data[1] << 16)
        | (data[2] <<  8)
        |  data[3];

    *buffer += 4;

    return value;

}

char* buffer_read_string(char** buffer, int* length) {

    char* value;

    *length = buffer_read_uint32(buffer);
    value = *buffer;

    *buffer += *length;

    return value;

}

