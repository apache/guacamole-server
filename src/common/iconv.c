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

#include "common/iconv.h"

#include <guacamole/unicode.h>

#include <endian.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * CP1252 lookup tables.
 *
 * __GUAC_CP1252_CODEPOINT maps CP1252 byte values in the 0x80-0x9F exception
 * range to Unicode codepoints.
 *
 * __GUAC_UNICODE_TO_CP1252 maps the Unicode codepoints representable in that
 * exception range back to their byte values. Because only 27 codepoints in the
 * range differ from their Unicode byte values, the reverse mapping is stored
 * as a sparse array sorted by codepoint, allowing the CP1252 byte to be found
 * with bsearch() in O(log n) time.
 */
const static int __GUAC_CP1252_CODEPOINT[32] = {
    0x20AC, /* 0x80 */
    0xFFFD, /* 0x81 */
    0x201A, /* 0x82 */
    0x0192, /* 0x83 */
    0x201E, /* 0x84 */
    0x2026, /* 0x85 */
    0x2020, /* 0x86 */
    0x2021, /* 0x87 */
    0x02C6, /* 0x88 */
    0x2030, /* 0x89 */
    0x0160, /* 0x8A */
    0x2039, /* 0x8B */
    0x0152, /* 0x8C */
    0xFFFD, /* 0x8D */
    0x017D, /* 0x8E */
    0xFFFD, /* 0x8F */
    0xFFFD, /* 0x90 */
    0x2018, /* 0x91 */
    0x2019, /* 0x92 */
    0x201C, /* 0x93 */
    0x201D, /* 0x94 */
    0x2022, /* 0x95 */
    0x2013, /* 0x96 */
    0x2014, /* 0x97 */
    0x02DC, /* 0x98 */
    0x2122, /* 0x99 */
    0x0161, /* 0x9A */
    0x203A, /* 0x9B */
    0x0153, /* 0x9C */
    0xFFFD, /* 0x9D */
    0x017E, /* 0x9E */
    0x0178, /* 0x9F */
};

typedef struct {
    int codepoint;
    unsigned char byte;
} guac_cp1252_reverse_t;

const static guac_cp1252_reverse_t __GUAC_UNICODE_TO_CP1252[27] = {
    { 0x0152, 0x8C }, /* 0x8C */
    { 0x0153, 0x9C }, /* 0x9C */
    { 0x0160, 0x8A }, /* 0x8A */
    { 0x0161, 0x9A }, /* 0x9A */
    { 0x0178, 0x9F }, /* 0x9F */
    { 0x017D, 0x8E }, /* 0x8E */
    { 0x017E, 0x9E }, /* 0x9E */
    { 0x0192, 0x83 }, /* 0x83 */
    { 0x02C6, 0x88 }, /* 0x88 */
    { 0x02DC, 0x98 }, /* 0x98 */
    { 0x2013, 0x96 }, /* 0x96 */
    { 0x2014, 0x97 }, /* 0x97 */
    { 0x2018, 0x91 }, /* 0x91 */
    { 0x2019, 0x92 }, /* 0x92 */
    { 0x201A, 0x82 }, /* 0x82 */
    { 0x201C, 0x93 }, /* 0x93 */
    { 0x201D, 0x94 }, /* 0x94 */
    { 0x201E, 0x84 }, /* 0x84 */
    { 0x2020, 0x86 }, /* 0x86 */
    { 0x2021, 0x87 }, /* 0x87 */
    { 0x2022, 0x95 }, /* 0x95 */
    { 0x2026, 0x85 }, /* 0x85 */
    { 0x2030, 0x89 }, /* 0x89 */
    { 0x2039, 0x8B }, /* 0x8B */
    { 0x203A, 0x9B }, /* 0x9B */
    { 0x20AC, 0x80 }, /* 0x80 */
    { 0x2122, 0x99 } /* 0x99 */
};

/*
 * MacRoman lookup tables.
 *
 * __GUAC_MACROMAN_CODEPOINT maps MacRoman byte values to Unicode codepoints,
 * allowing the codepoint to be determined by direct array access.
 *
 * __GUAC_UNICODE_TO_MACROMAN maps the Unicode codepoints representable in
 * MacRoman back to their byte values. Because the Unicode lookup space is much
 * larger than 256 bytes, the reverse mapping is stored as a sparse array
 * sorted by codepoint, allowing the MacRoman byte to be found with bsearch()
 * in O(log n) time.
 *
 * To regenerate both tables, run:
 *     python3 generate_macroman_table.py
 *
 * where generate_macroman_table.py contains:
 *     MACROMAN_HIGH_RANGE = range(0x80, 0x100)
 *
 *     # Forward table: MacRoman byte -> Unicode codepoint
 *     pairs = []
 *     print("const static int __GUAC_MACROMAN_CODEPOINT[128] = {")
 *     for mac_byte in MACROMAN_HIGH_RANGE:
 *         unicode_char = bytes([mac_byte]).decode('mac_roman')
 *         unicode_codepoint = ord(unicode_char)
 *         pairs.append((unicode_codepoint, mac_byte))
 *         trailing_comma = "," if mac_byte < 0xFF else ""
 *         print(f"    0x{unicode_codepoint:04X}{trailing_comma} "
 *               f"/ * 0x{mac_byte:02X} - {unicode_char} * /")
 *     print("};")
 *     print()
 *
 *     # Reverse table: Unicode codepoint -> MacRoman byte, sorted by codepoint
 *     print("const static struct {")
 *     print("    int codepoint;")
 *     print("    unsigned char byte;")
 *     print("} __GUAC_UNICODE_TO_MACROMAN[ARRAY_SIZE(__GUAC_MACROMAN_CODEPOINT)] = {")
 *     for unicode_codepoint, mac_byte in sorted(pairs):
 *         unicode_char = chr(unicode_codepoint)
 *         print(f"    {{ 0x{unicode_codepoint:04X}, 0x{mac_byte:02X} }}, "
 *               f"/ * {unicode_char} * /")
 *     print("};")
 */

const static int __GUAC_MACROMAN_CODEPOINT[128] = {
    0x00C4, /* 0x80 - Ä */
    0x00C5, /* 0x81 - Å */
    0x00C7, /* 0x82 - Ç */
    0x00C9, /* 0x83 - É */
    0x00D1, /* 0x84 - Ñ */
    0x00D6, /* 0x85 - Ö */
    0x00DC, /* 0x86 - Ü */
    0x00E1, /* 0x87 - á */
    0x00E0, /* 0x88 - à */
    0x00E2, /* 0x89 - â */
    0x00E4, /* 0x8A - ä */
    0x00E3, /* 0x8B - ã */
    0x00E5, /* 0x8C - å */
    0x00E7, /* 0x8D - ç */
    0x00E9, /* 0x8E - é */
    0x00E8, /* 0x8F - è */
    0x00EA, /* 0x90 - ê */
    0x00EB, /* 0x91 - ë */
    0x00ED, /* 0x92 - í */
    0x00EC, /* 0x93 - ì */
    0x00EE, /* 0x94 - î */
    0x00EF, /* 0x95 - ï */
    0x00F1, /* 0x96 - ñ */
    0x00F3, /* 0x97 - ó */
    0x00F2, /* 0x98 - ò */
    0x00F4, /* 0x99 - ô */
    0x00F6, /* 0x9A - ö */
    0x00F5, /* 0x9B - õ */
    0x00FA, /* 0x9C - ú */
    0x00F9, /* 0x9D - ù */
    0x00FB, /* 0x9E - û */
    0x00FC, /* 0x9F - ü */
    0x2020, /* 0xA0 - † */
    0x00B0, /* 0xA1 - ° */
    0x00A2, /* 0xA2 - ¢ */
    0x00A3, /* 0xA3 - £ */
    0x00A7, /* 0xA4 - § */
    0x2022, /* 0xA5 - • */
    0x00B6, /* 0xA6 - ¶ */
    0x00DF, /* 0xA7 - ß */
    0x00AE, /* 0xA8 - ® */
    0x00A9, /* 0xA9 - © */
    0x2122, /* 0xAA - ™ */
    0x00B4, /* 0xAB - ´ */
    0x00A8, /* 0xAC - ¨ */
    0x2260, /* 0xAD - ≠ */
    0x00C6, /* 0xAE - Æ */
    0x00D8, /* 0xAF - Ø */
    0x221E, /* 0xB0 - ∞ */
    0x00B1, /* 0xB1 - ± */
    0x2264, /* 0xB2 - ≤ */
    0x2265, /* 0xB3 - ≥ */
    0x00A5, /* 0xB4 - ¥ */
    0x00B5, /* 0xB5 - µ */
    0x2202, /* 0xB6 - ∂ */
    0x2211, /* 0xB7 - ∑ */
    0x220F, /* 0xB8 - ∏ */
    0x03C0, /* 0xB9 - π */
    0x222B, /* 0xBA - ∫ */
    0x00AA, /* 0xBB - ª */
    0x00BA, /* 0xBC - º */
    0x03A9, /* 0xBD - Ω */
    0x00E6, /* 0xBE - æ */
    0x00F8, /* 0xBF - ø */
    0x00BF, /* 0xC0 - ¿ */
    0x00A1, /* 0xC1 - ¡ */
    0x00AC, /* 0xC2 - ¬ */
    0x221A, /* 0xC3 - √ */
    0x0192, /* 0xC4 - ƒ */
    0x2248, /* 0xC5 - ≈ */
    0x2206, /* 0xC6 - ∆ */
    0x00AB, /* 0xC7 - « */
    0x00BB, /* 0xC8 - » */
    0x2026, /* 0xC9 - … */
    0x00A0, /* 0xCA -   */
    0x00C0, /* 0xCB - À */
    0x00C3, /* 0xCC - Ã */
    0x00D5, /* 0xCD - Õ */
    0x0152, /* 0xCE - Œ */
    0x0153, /* 0xCF - œ */
    0x2013, /* 0xD0 - – */
    0x2014, /* 0xD1 - — */
    0x201C, /* 0xD2 - “ */
    0x201D, /* 0xD3 - ” */
    0x2018, /* 0xD4 - ‘ */
    0x2019, /* 0xD5 - ’ */
    0x00F7, /* 0xD6 - ÷ */
    0x25CA, /* 0xD7 - ◊ */
    0x00FF, /* 0xD8 - ÿ */
    0x0178, /* 0xD9 - Ÿ */
    0x2044, /* 0xDA - ⁄ */
    0x20AC, /* 0xDB - € */
    0x2039, /* 0xDC - ‹ */
    0x203A, /* 0xDD - › */
    0xFB01, /* 0xDE - ﬁ */
    0xFB02, /* 0xDF - ﬂ */
    0x2021, /* 0xE0 - ‡ */
    0x00B7, /* 0xE1 - · */
    0x201A, /* 0xE2 - ‚ */
    0x201E, /* 0xE3 - „ */
    0x2030, /* 0xE4 - ‰ */
    0x00C2, /* 0xE5 - Â */
    0x00CA, /* 0xE6 - Ê */
    0x00C1, /* 0xE7 - Á */
    0x00CB, /* 0xE8 - Ë */
    0x00C8, /* 0xE9 - È */
    0x00CD, /* 0xEA - Í */
    0x00CE, /* 0xEB - Î */
    0x00CF, /* 0xEC - Ï */
    0x00CC, /* 0xED - Ì */
    0x00D3, /* 0xEE - Ó */
    0x00D4, /* 0xEF - Ô */
    0xF8FF, /* 0xF0 -  */
    0x00D2, /* 0xF1 - Ò */
    0x00DA, /* 0xF2 - Ú */
    0x00DB, /* 0xF3 - Û */
    0x00D9, /* 0xF4 - Ù */
    0x0131, /* 0xF5 - ı */
    0x02C6, /* 0xF6 - ˆ */
    0x02DC, /* 0xF7 - ˜ */
    0x00AF, /* 0xF8 - ¯ */
    0x02D8, /* 0xF9 - ˘ */
    0x02D9, /* 0xFA - ˙ */
    0x02DA, /* 0xFB - ˚ */
    0x00B8, /* 0xFC - ¸ */
    0x02DD, /* 0xFD - ˝ */
    0x02DB, /* 0xFE - ˛ */
    0x02C7 /* 0xFF - ˇ */
};

typedef struct {
    int codepoint;
    unsigned char byte;
} guac_macroman_reverse_t;

const static guac_macroman_reverse_t __GUAC_UNICODE_TO_MACROMAN[ARRAY_SIZE(__GUAC_MACROMAN_CODEPOINT)] = {
    { 0x00A0, 0xCA }, /* 0xCA -   */
    { 0x00A1, 0xC1 }, /* 0xC1 - ¡ */
    { 0x00A2, 0xA2 }, /* 0xA2 - ¢ */
    { 0x00A3, 0xA3 }, /* 0xA3 - £ */
    { 0x00A5, 0xB4 }, /* 0xB4 - ¥ */
    { 0x00A7, 0xA4 }, /* 0xA4 - § */
    { 0x00A8, 0xAC }, /* 0xAC - ¨ */
    { 0x00A9, 0xA9 }, /* 0xA9 - © */
    { 0x00AA, 0xBB }, /* 0xBB - ª */
    { 0x00AB, 0xC7 }, /* 0xC7 - « */
    { 0x00AC, 0xC2 }, /* 0xC2 - ¬ */
    { 0x00AE, 0xA8 }, /* 0xA8 - ® */
    { 0x00AF, 0xF8 }, /* 0xF8 - ¯ */
    { 0x00B0, 0xA1 }, /* 0xA1 - ° */
    { 0x00B1, 0xB1 }, /* 0xB1 - ± */
    { 0x00B4, 0xAB }, /* 0xAB - ´ */
    { 0x00B5, 0xB5 }, /* 0xB5 - µ */
    { 0x00B6, 0xA6 }, /* 0xA6 - ¶ */
    { 0x00B7, 0xE1 }, /* 0xE1 - · */
    { 0x00B8, 0xFC }, /* 0xFC - ¸ */
    { 0x00BA, 0xBC }, /* 0xBC - º */
    { 0x00BB, 0xC8 }, /* 0xC8 - » */
    { 0x00BF, 0xC0 }, /* 0xC0 - ¿ */
    { 0x00C0, 0xCB }, /* 0xCB - À */
    { 0x00C1, 0xE7 }, /* 0xE7 - Á */
    { 0x00C2, 0xE5 }, /* 0xE5 - Â */
    { 0x00C3, 0xCC }, /* 0xCC - Ã */
    { 0x00C4, 0x80 }, /* 0x80 - Ä */
    { 0x00C5, 0x81 }, /* 0x81 - Å */
    { 0x00C6, 0xAE }, /* 0xAE - Æ */
    { 0x00C7, 0x82 }, /* 0x82 - Ç */
    { 0x00C8, 0xE9 }, /* 0xE9 - È */
    { 0x00C9, 0x83 }, /* 0x83 - É */
    { 0x00CA, 0xE6 }, /* 0xE6 - Ê */
    { 0x00CB, 0xE8 }, /* 0xE8 - Ë */
    { 0x00CC, 0xED }, /* 0xED - Ì */
    { 0x00CD, 0xEA }, /* 0xEA - Í */
    { 0x00CE, 0xEB }, /* 0xEB - Î */
    { 0x00CF, 0xEC }, /* 0xEC - Ï */
    { 0x00D1, 0x84 }, /* 0x84 - Ñ */
    { 0x00D2, 0xF1 }, /* 0xF1 - Ò */
    { 0x00D3, 0xEE }, /* 0xEE - Ó */
    { 0x00D4, 0xEF }, /* 0xEF - Ô */
    { 0x00D5, 0xCD }, /* 0xCD - Õ */
    { 0x00D6, 0x85 }, /* 0x85 - Ö */
    { 0x00D8, 0xAF }, /* 0xAF - Ø */
    { 0x00D9, 0xF4 }, /* 0xF4 - Ù */
    { 0x00DA, 0xF2 }, /* 0xF2 - Ú */
    { 0x00DB, 0xF3 }, /* 0xF3 - Û */
    { 0x00DC, 0x86 }, /* 0x86 - Ü */
    { 0x00DF, 0xA7 }, /* 0xA7 - ß */
    { 0x00E0, 0x88 }, /* 0x88 - à */
    { 0x00E1, 0x87 }, /* 0x87 - á */
    { 0x00E2, 0x89 }, /* 0x89 - â */
    { 0x00E3, 0x8B }, /* 0x8B - ã */
    { 0x00E4, 0x8A }, /* 0x8A - ä */
    { 0x00E5, 0x8C }, /* 0x8C - å */
    { 0x00E6, 0xBE }, /* 0xBE - æ */
    { 0x00E7, 0x8D }, /* 0x8D - ç */
    { 0x00E8, 0x8F }, /* 0x8F - è */
    { 0x00E9, 0x8E }, /* 0x8E - é */
    { 0x00EA, 0x90 }, /* 0x90 - ê */
    { 0x00EB, 0x91 }, /* 0x91 - ë */
    { 0x00EC, 0x93 }, /* 0x93 - ì */
    { 0x00ED, 0x92 }, /* 0x92 - í */
    { 0x00EE, 0x94 }, /* 0x94 - î */
    { 0x00EF, 0x95 }, /* 0x95 - ï */
    { 0x00F1, 0x96 }, /* 0x96 - ñ */
    { 0x00F2, 0x98 }, /* 0x98 - ò */
    { 0x00F3, 0x97 }, /* 0x97 - ó */
    { 0x00F4, 0x99 }, /* 0x99 - ô */
    { 0x00F5, 0x9B }, /* 0x9B - õ */
    { 0x00F6, 0x9A }, /* 0x9A - ö */
    { 0x00F7, 0xD6 }, /* 0xD6 - ÷ */
    { 0x00F8, 0xBF }, /* 0xBF - ø */
    { 0x00F9, 0x9D }, /* 0x9D - ù */
    { 0x00FA, 0x9C }, /* 0x9C - ú */
    { 0x00FB, 0x9E }, /* 0x9E - û */
    { 0x00FC, 0x9F }, /* 0x9F - ü */
    { 0x00FF, 0xD8 }, /* 0xD8 - ÿ */
    { 0x0131, 0xF5 }, /* 0xF5 - ı */
    { 0x0152, 0xCE }, /* 0xCE - Œ */
    { 0x0153, 0xCF }, /* 0xCF - œ */
    { 0x0178, 0xD9 }, /* 0xD9 - Ÿ */
    { 0x0192, 0xC4 }, /* 0xC4 - ƒ */
    { 0x02C6, 0xF6 }, /* 0xF6 - ˆ */
    { 0x02C7, 0xFF }, /* 0xFF - ˇ */
    { 0x02D8, 0xF9 }, /* 0xF9 - ˘ */
    { 0x02D9, 0xFA }, /* 0xFA - ˙ */
    { 0x02DA, 0xFB }, /* 0xFB - ˚ */
    { 0x02DB, 0xFE }, /* 0xFE - ˛ */
    { 0x02DC, 0xF7 }, /* 0xF7 - ˜ */
    { 0x02DD, 0xFD }, /* 0xFD - ˝ */
    { 0x03A9, 0xBD }, /* 0xBD - Ω */
    { 0x03C0, 0xB9 }, /* 0xB9 - π */
    { 0x2013, 0xD0 }, /* 0xD0 - – */
    { 0x2014, 0xD1 }, /* 0xD1 - — */
    { 0x2018, 0xD4 }, /* 0xD4 - ‘ */
    { 0x2019, 0xD5 }, /* 0xD5 - ’ */
    { 0x201A, 0xE2 }, /* 0xE2 - ‚ */
    { 0x201C, 0xD2 }, /* 0xD2 - “ */
    { 0x201D, 0xD3 }, /* 0xD3 - ” */
    { 0x201E, 0xE3 }, /* 0xE3 - „ */
    { 0x2020, 0xA0 }, /* 0xA0 - † */
    { 0x2021, 0xE0 }, /* 0xE0 - ‡ */
    { 0x2022, 0xA5 }, /* 0xA5 - • */
    { 0x2026, 0xC9 }, /* 0xC9 - … */
    { 0x2030, 0xE4 }, /* 0xE4 - ‰ */
    { 0x2039, 0xDC }, /* 0xDC - ‹ */
    { 0x203A, 0xDD }, /* 0xDD - › */
    { 0x2044, 0xDA }, /* 0xDA - ⁄ */
    { 0x20AC, 0xDB }, /* 0xDB - € */
    { 0x2122, 0xAA }, /* 0xAA - ™ */
    { 0x2202, 0xB6 }, /* 0xB6 - ∂ */
    { 0x2206, 0xC6 }, /* 0xC6 - ∆ */
    { 0x220F, 0xB8 }, /* 0xB8 - ∏ */
    { 0x2211, 0xB7 }, /* 0xB7 - ∑ */
    { 0x221A, 0xC3 }, /* 0xC3 - √ */
    { 0x221E, 0xB0 }, /* 0xB0 - ∞ */
    { 0x222B, 0xBA }, /* 0xBA - ∫ */
    { 0x2248, 0xC5 }, /* 0xC5 - ≈ */
    { 0x2260, 0xAD }, /* 0xAD - ≠ */
    { 0x2264, 0xB2 }, /* 0xB2 - ≤ */
    { 0x2265, 0xB3 }, /* 0xB3 - ≥ */
    { 0x25CA, 0xD7 }, /* 0xD7 - ◊ */
    { 0xF8FF, 0xF0 }, /* 0xF0 -  */
    { 0xFB01, 0xDE }, /* 0xDE - ﬁ */
    { 0xFB02, 0xDF }, /* 0xDF - ﬂ */
};

int guac_iconv(guac_iconv_read* reader, const char** input, int in_remaining,
               guac_iconv_write* writer, char** output, int out_remaining) {

    while (in_remaining > 0 && out_remaining > 0) {

        int value;
        const char* read_start;
        char* write_start;

        /* Read character */
        read_start = *input;
        value = reader(input, in_remaining);
        in_remaining -= *input - read_start;

        /* Write character */
        write_start = *output;
        writer(output, out_remaining, value);
        out_remaining -= *output - write_start;

        /* Stop if null terminator reached */
        if (value == 0)
            return 1;

    }

    /* Null terminator not reached */
    return 0;

}

int GUAC_READ_UTF8(const char** input, int remaining) {

    int value;

    *input += guac_utf8_read(*input, remaining, &value);
    return value;

}

int GUAC_READ_UTF16(const char** input, int remaining) {

    int value;

    /* Bail if not enough data */
    if (remaining < 2)
        return 0;

    /* Read two bytes as integer */
    value = le16toh(*((uint16_t*) *input));
    *input += 2;

    return value;

}

int GUAC_READ_CP1252(const char** input, int remaining) {

    int value = *((unsigned char*) *input);

    /* Replace value with exception if not identical to ISO-8859-1 */
    if (value >= 0x80 && value <= 0x9F)
        value = __GUAC_CP1252_CODEPOINT[value - 0x80];

    (*input)++;
    return value;

}

int GUAC_READ_ISO8859_1(const char** input, int remaining) {

    int value = *((unsigned char*) *input);

    (*input)++;
    return value;

}

/**
 * Invokes the given reader function, automatically normalizing newline
 * sequences as Unix-style newline characters ('\n').  All other characters are
 * read verbatim.
 *
 * @param reader
 *     The reader to use to read the given character.
 *
 * @param input
 *     Pointer to the location within the input buffer that the next character
 *     should be read from.
 *
 * @param remaining
 *     The number of bytes remaining in the input buffer.
 *
 * @return
 *     The codepoint that was read, or zero if the end of the input string has
 *     been reached.
 */
static int guac_iconv_read_normalized(guac_iconv_read* reader,
        const char** input, int remaining) {

    /* Read requested character */
    const char* input_start = *input;
    int value = reader(input, remaining);

    /* Automatically translate CRLF pairs to simple newlines */
    if (value == '\r') {

        /* Peek ahead by one character, adjusting remaining bytes relative to
         * last read */
        int peek_remaining = remaining - (*input - input_start);
        const char* peek_input = *input;
        int peek_value = reader(&peek_input, peek_remaining);

        /* Consider read value to be a newline if we have encountered a "\r\n"
         * (CRLF) pair */
        if (peek_value == '\n') {
            value = '\n';
            *input = peek_input;
        }

    }

    return value;

}

int GUAC_READ_UTF8_NORMALIZED(const char** input, int remaining) {
    return guac_iconv_read_normalized(GUAC_READ_UTF8, input, remaining);
}

int GUAC_READ_UTF16_NORMALIZED(const char** input, int remaining) {
    return guac_iconv_read_normalized(GUAC_READ_UTF16, input, remaining);
}

int GUAC_READ_CP1252_NORMALIZED(const char** input, int remaining) {
    return guac_iconv_read_normalized(GUAC_READ_CP1252, input, remaining);
}

int GUAC_READ_ISO8859_1_NORMALIZED(const char** input, int remaining) {
    return guac_iconv_read_normalized(GUAC_READ_ISO8859_1, input, remaining);
}

int GUAC_READ_MACROMAN(const char** input, int remaining) {

    /* MacRoman is a single-byte encoding: each character is one byte */
    int value = (unsigned char) **input;
    (*input)++;

    /* Bytes 0x00-0x7F are identical to ASCII/Unicode; bytes 0x80-0xFF are
     * remapped via the lookup table. ARRAY_SIZE guards against the table
     * being resized without updating the encoding range. */
    if (value >= 0x80 && (size_t)(value - 0x80) < ARRAY_SIZE(__GUAC_MACROMAN_CODEPOINT))
        value = __GUAC_MACROMAN_CODEPOINT[value - 0x80];

    return value;

}

int GUAC_READ_MACROMAN_NORMALIZED(const char** input, int remaining) {

    const char* input_start = *input;
    int value = GUAC_READ_MACROMAN(input, remaining);

    /* Translate both bare CR (\r) and CRLF (\r\n) to Unix newline (\n).
     * Classic Mac OS uses bare CR as its line separator, so bare CR must be
     * normalized in addition to the CRLF pairs handled by other encodings. */
    if (value == '\r') {

        int peek_remaining = remaining - (*input - input_start);
        const char* peek_input = *input;
        int peek_value = GUAC_READ_MACROMAN(&peek_input, peek_remaining);

        /* Consume the following LF if this is a CRLF pair */
        if (peek_value == '\n')
            *input = peek_input;

        value = '\n';
    }

    return value;

}

void GUAC_WRITE_UTF8(char** output, int remaining, int value) {
    *output += guac_utf8_write(value, *output, remaining);
}

void GUAC_WRITE_UTF16(char** output, int remaining, int value) {

    /* Bail if not enough data */
    if (remaining < 2)
        return;

    /* Write two bytes as integer */
    *((uint16_t*) *output) = htole16(value);
    *output += 2;

}

/**
 * Compares a Unicode codepoint against a CP1252 reverse-lookup table entry.
 *
 * @param key
 *     Pointer to the Unicode codepoint being searched for.
 *
 * @param elem
 *     Pointer to the CP1252 reverse-lookup table entry being compared.
 *
 * @return
 *     A negative value if the requested codepoint sorts before the table
 *     entry, a positive value if it sorts after the table entry, or zero if
 *     the codepoints are equal.
 */
static int guac_cp1252_reverse_cmp(const void* key, const void* elem) {
    int value = *(const int*) key;
    const guac_cp1252_reverse_t* entry = elem;
    return (value > entry->codepoint) - (value < entry->codepoint);
}

void GUAC_WRITE_CP1252(char** output, int remaining, int value) {

    /* CP1252 matches Unicode directly for 0x00-0x7F and 0xA0-0xFF. */
    if ((value >= 0x00 && value < 0x80) || (value >= 0xA0 && value <= 0xFF)) {
        *((unsigned char*) *output) = (unsigned char) value;
        (*output)++;
        return;
    }

    /* The 0x80-0x9F range is a sparse remapping, so the reverse mapping is
     * stored as a codepoint-sorted table and searched with bsearch(). Values
     * outside the direct-write ranges also land here: representable exception
     * codepoints map to 0x80-0x9F, while everything else falls back to '?'. */
    const guac_cp1252_reverse_t* match = bsearch(
            &value,                                 /* key */
            __GUAC_UNICODE_TO_CP1252,              /* base */
            ARRAY_SIZE(__GUAC_UNICODE_TO_CP1252),  /* nmemb */
            sizeof(*__GUAC_UNICODE_TO_CP1252),     /* size */
            guac_cp1252_reverse_cmp                /* compar */
    );

    /* Write the matched byte, or '?' if the codepoint is not representable. */
    *((unsigned char*) *output) = (match != NULL) ? match->byte : '?';
    (*output)++;
}

void GUAC_WRITE_ISO8859_1(char** output, int remaining, int value) {

    /* Translate to question mark if out of range */
    if (value > 0xFF)
        value = '?';

    *((unsigned char*) *output) = (unsigned char) value;
    (*output)++;
}

/**
 * Invokes the given writer function, automatically writing newline characters
 * ('\n') as CRLF ("\r\n"). All other characters are written verbatim.
 *
 * @param writer
 *     The writer to use to write the given character.
 *
 * @param output
 *     Pointer to the location within the output buffer that the next character
 *     should be written.
 *
 * @param remaining
 *     The number of bytes remaining in the output buffer.
 *
 * @param value
 *     The codepoint of the character to write.
 */
static void guac_iconv_write_crlf(guac_iconv_write* writer, char** output,
        int remaining, int value) {

    if (value != '\n') {
        writer(output, remaining, value);
        return;
    }

    char* output_start = *output;
    writer(output, remaining, '\r');

    remaining -= *output - output_start;
    if (remaining > 0)
        writer(output, remaining, '\n');

}

void GUAC_WRITE_UTF8_CRLF(char** output, int remaining, int value) {
    guac_iconv_write_crlf(GUAC_WRITE_UTF8, output, remaining, value);
}

void GUAC_WRITE_UTF16_CRLF(char** output, int remaining, int value) {
    guac_iconv_write_crlf(GUAC_WRITE_UTF16, output, remaining, value);
}

void GUAC_WRITE_CP1252_CRLF(char** output, int remaining, int value) {
    guac_iconv_write_crlf(GUAC_WRITE_CP1252, output, remaining, value);
}

void GUAC_WRITE_ISO8859_1_CRLF(char** output, int remaining, int value) {
    guac_iconv_write_crlf(GUAC_WRITE_ISO8859_1, output, remaining, value);
}

/**
 * Compares a Unicode codepoint against a MacRoman reverse-lookup table entry.
 *
 * @param key
 *     Pointer to the Unicode codepoint being searched for.
 *
 * @param elem
 *     Pointer to the MacRoman reverse-lookup table entry being compared.
 *
 * @return
 *     A negative value if the requested codepoint sorts before the table
 *     entry, a positive value if it sorts after the table entry, or zero if
 *     the codepoints are equal.
 */
static int guac_macroman_reverse_cmp(const void* key, const void* elem) {
    int value = *(const int*) key;
    const guac_macroman_reverse_t* entry = elem;
    return (value > entry->codepoint) - (value < entry->codepoint);
}

void GUAC_WRITE_MACROMAN(char** output, int remaining, int value) {

    /* MacRoman is a single-byte encoding: each character is one byte */

    /* ASCII range is identical to Unicode; write directly */
    if (value >= 0x00 && value < 0x80) {
        *((unsigned char*) *output) = (unsigned char) value;
        (*output)++;
        return;
    }

    /* The reverse mapping is stored as a sparse array of representable Unicode
     * codepoints sorted by codepoint, allowing bsearch() to find the MacRoman
     * byte in O(log n) time. */
    const guac_macroman_reverse_t* match = bsearch(
            &value,                                   /* key */
            __GUAC_UNICODE_TO_MACROMAN,               /* base */
            ARRAY_SIZE(__GUAC_UNICODE_TO_MACROMAN),   /* nmemb */
            sizeof(*__GUAC_UNICODE_TO_MACROMAN),      /* size */
            guac_macroman_reverse_cmp                 /* compar */
    );

    /* Write the matched byte, or '?' if the codepoint is not representable. */
    *((unsigned char*) *output) = (match != NULL) ? match->byte : '?';
    (*output)++;

}

void GUAC_WRITE_MACROMAN_CRLF(char** output, int remaining, int value) {
    guac_iconv_write_crlf(GUAC_WRITE_MACROMAN, output, remaining, value);
}
