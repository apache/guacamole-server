
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
 * The Original Code is libguac-client-rdp.
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

#include <freerdp/input.h>

#include "rdp_keymap.h"

const guac_rdp_keymap guac_rdp_keysym_scancode[256][256] = {
    {                                        /* 0x00?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0000 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0001 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0002 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0003 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0004 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0005 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0006 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0007 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0008 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0009 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x000a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x000b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x000c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x000d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x000e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x000f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0010 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0011 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0012 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0013 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0014 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0015 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0016 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0017 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0018 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0019 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x001a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x001b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x001c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x001d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x001e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x001f */
        { .scancode = 0x39, .flags = 0x00 }, /* 0x0020 (space) */
        { .scancode = 0x02, .flags = 0x00 }, /* 0x0021 (exclam) */
        { .scancode = 0x28, .flags = 0x00 }, /* 0x0022 (quotedbl) */
        { .scancode = 0x04, .flags = 0x00 }, /* 0x0023 (numbersign) */
        { .scancode = 0x05, .flags = 0x00 }, /* 0x0024 (dollar) */
        { .scancode = 0x06, .flags = 0x00 }, /* 0x0025 (percent) */
        { .scancode = 0x08, .flags = 0x00 }, /* 0x0026 (ampersand) */
        { .scancode = 0x28, .flags = 0x00 }, /* 0x0027 (quoteright) */
        { .scancode = 0x0A, .flags = 0x00 }, /* 0x0028 (parenleft) */
        { .scancode = 0x0B, .flags = 0x00 }, /* 0x0029 (parenright) */
        { .scancode = 0x09, .flags = 0x00 }, /* 0x002a (asterisk) */
        { .scancode = 0x0D, .flags = 0x00 }, /* 0x002b (plus) */
        { .scancode = 0x33, .flags = 0x00 }, /* 0x002c (comma) */
        { .scancode = 0x0C, .flags = 0x00 }, /* 0x002d (minus) */
        { .scancode = 0x34, .flags = 0x00 }, /* 0x002e (period) */
        { .scancode = 0x35, .flags = 0x00 }, /* 0x002f (slash) */
        { .scancode = 0x0B, .flags = 0x00 }, /* 0x0030 (0) */
        { .scancode = 0x02, .flags = 0x00 }, /* 0x0031 (1) */
        { .scancode = 0x03, .flags = 0x00 }, /* 0x0032 (2) */
        { .scancode = 0x04, .flags = 0x00 }, /* 0x0033 (3) */
        { .scancode = 0x05, .flags = 0x00 }, /* 0x0034 (4) */
        { .scancode = 0x06, .flags = 0x00 }, /* 0x0035 (5) */
        { .scancode = 0x07, .flags = 0x00 }, /* 0x0036 (6) */
        { .scancode = 0x08, .flags = 0x00 }, /* 0x0037 (7) */
        { .scancode = 0x09, .flags = 0x00 }, /* 0x0038 (8) */
        { .scancode = 0x0A, .flags = 0x00 }, /* 0x0039 (9) */
        { .scancode = 0x27, .flags = 0x00 }, /* 0x003a (colon) */
        { .scancode = 0x27, .flags = 0x00 }, /* 0x003b (semicolon) */
        { .scancode = 0x33, .flags = 0x00 }, /* 0x003c (less) */
        { .scancode = 0x0D, .flags = 0x00 }, /* 0x003d (equal) */
        { .scancode = 0x34, .flags = 0x00 }, /* 0x003e (greater) */
        { .scancode = 0x35, .flags = 0x00 }, /* 0x003f (question) */
        { .scancode = 0x03, .flags = 0x00 }, /* 0x0040 (at) */
        { .scancode = 0x1E, .flags = 0x00 }, /* 0x0041 (A) */
        { .scancode = 0x30, .flags = 0x00 }, /* 0x0042 (B) */
        { .scancode = 0x2E, .flags = 0x00 }, /* 0x0043 (C) */
        { .scancode = 0x20, .flags = 0x00 }, /* 0x0044 (D) */
        { .scancode = 0x12, .flags = 0x00 }, /* 0x0045 (E) */
        { .scancode = 0x21, .flags = 0x00 }, /* 0x0046 (F) */
        { .scancode = 0x22, .flags = 0x00 }, /* 0x0047 (G) */
        { .scancode = 0x23, .flags = 0x00 }, /* 0x0048 (H) */
        { .scancode = 0x17, .flags = 0x00 }, /* 0x0049 (I) */
        { .scancode = 0x24, .flags = 0x00 }, /* 0x004a (J) */
        { .scancode = 0x25, .flags = 0x00 }, /* 0x004b (K) */
        { .scancode = 0x26, .flags = 0x00 }, /* 0x004c (L) */
        { .scancode = 0x32, .flags = 0x00 }, /* 0x004d (M) */
        { .scancode = 0x31, .flags = 0x00 }, /* 0x004e (N) */
        { .scancode = 0x18, .flags = 0x00 }, /* 0x004f (O) */
        { .scancode = 0x19, .flags = 0x00 }, /* 0x0050 (P) */
        { .scancode = 0x10, .flags = 0x00 }, /* 0x0051 (Q) */
        { .scancode = 0x13, .flags = 0x00 }, /* 0x0052 (R) */
        { .scancode = 0x1F, .flags = 0x00 }, /* 0x0053 (S) */
        { .scancode = 0x14, .flags = 0x00 }, /* 0x0054 (T) */
        { .scancode = 0x16, .flags = 0x00 }, /* 0x0055 (U) */
        { .scancode = 0x2F, .flags = 0x00 }, /* 0x0056 (V) */
        { .scancode = 0x11, .flags = 0x00 }, /* 0x0057 (W) */
        { .scancode = 0x2D, .flags = 0x00 }, /* 0x0058 (X) */
        { .scancode = 0x15, .flags = 0x00 }, /* 0x0059 (Y) */
        { .scancode = 0x2C, .flags = 0x00 }, /* 0x005a (Z) */
        { .scancode = 0x1A, .flags = 0x00 }, /* 0x005b (bracketleft) */
        { .scancode = 0x2B, .flags = 0x00 }, /* 0x005c (backslash) */
        { .scancode = 0x1B, .flags = 0x00 }, /* 0x005d (bracketright) */
        { .scancode = 0x29, .flags = 0x00 }, /* 0x005e (asciicircum) */
        { .scancode = 0x0C, .flags = 0x00 }, /* 0x005f (underscore) */
        { .scancode = 0x29, .flags = 0x00 }, /* 0x0060 (quoteleft) */
        { .scancode = 0x1E, .flags = 0x00 }, /* 0x0061 (a) */
        { .scancode = 0x30, .flags = 0x00 }, /* 0x0062 (b) */
        { .scancode = 0x2E, .flags = 0x00 }, /* 0x0063 (c) */
        { .scancode = 0x20, .flags = 0x00 }, /* 0x0064 (d) */
        { .scancode = 0x12, .flags = 0x00 }, /* 0x0065 (e) */
        { .scancode = 0x21, .flags = 0x00 }, /* 0x0066 (f) */
        { .scancode = 0x22, .flags = 0x00 }, /* 0x0067 (g) */
        { .scancode = 0x23, .flags = 0x00 }, /* 0x0068 (h) */
        { .scancode = 0x17, .flags = 0x00 }, /* 0x0069 (i) */
        { .scancode = 0x24, .flags = 0x00 }, /* 0x006a (j) */
        { .scancode = 0x25, .flags = 0x00 }, /* 0x006b (k) */
        { .scancode = 0x26, .flags = 0x00 }, /* 0x006c (l) */
        { .scancode = 0x32, .flags = 0x00 }, /* 0x006d (m) */
        { .scancode = 0x31, .flags = 0x00 }, /* 0x006e (n) */
        { .scancode = 0x18, .flags = 0x00 }, /* 0x006f (o) */
        { .scancode = 0x19, .flags = 0x00 }, /* 0x0070 (p) */
        { .scancode = 0x10, .flags = 0x00 }, /* 0x0071 (q) */
        { .scancode = 0x13, .flags = 0x00 }, /* 0x0072 (r) */
        { .scancode = 0x1F, .flags = 0x00 }, /* 0x0073 (s) */
        { .scancode = 0x14, .flags = 0x00 }, /* 0x0074 (t) */
        { .scancode = 0x16, .flags = 0x00 }, /* 0x0075 (u) */
        { .scancode = 0x2F, .flags = 0x00 }, /* 0x0076 (v) */
        { .scancode = 0x11, .flags = 0x00 }, /* 0x0077 (w) */
        { .scancode = 0x2D, .flags = 0x00 }, /* 0x0078 (x) */
        { .scancode = 0x15, .flags = 0x00 }, /* 0x0079 (y) */
        { .scancode = 0x2C, .flags = 0x00 }, /* 0x007a (z) */
        { .scancode = 0x1A, .flags = 0x00 }, /* 0x007b (braceleft) */
        { .scancode = 0x2B, .flags = 0x00 }, /* 0x007c (bar) */
        { .scancode = 0x1B, .flags = 0x00 }, /* 0x007d (braceright) */
        { .scancode = 0x29, .flags = 0x00 }, /* 0x007e (asciitilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x007f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0080 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0081 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0082 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0083 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0084 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0085 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0086 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0087 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0088 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0089 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x008a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x008b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x008c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x008d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x008e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x008f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0090 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0091 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0092 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0093 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0094 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0095 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0096 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0097 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0098 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0099 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x009a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x009b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x009c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x009d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x009e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x009f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a0 (nobreakspace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a1 (exclamdown) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a2 (cent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a3 (sterling) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a4 (currency) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a5 (yen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a6 (brokenbar) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a7 (section) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a8 (diaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00a9 (copyright) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00aa (ordfeminine) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ab (guillemotleft) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ac (notsign) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ad (hyphen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ae (registered) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00af (macron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b0 (degree) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b1 (plusminus) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b2 (twosuperior) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b3 (threesuperior) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b4 (acute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b5 (mu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b6 (paragraph) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b7 (periodcentered) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b8 (cedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00b9 (onesuperior) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ba (masculine) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00bb (guillemotright) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00bc (onequarter) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00bd (onehalf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00be (threequarters) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00bf (questiondown) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c0 (Agrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c1 (Aacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c2 (Acircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c3 (Atilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c4 (Adiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c5 (Aring) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c6 (AE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c7 (Ccedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c8 (Egrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00c9 (Eacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ca (Ecircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00cb (Ediaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00cc (Igrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00cd (Iacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ce (Icircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00cf (Idiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d0 (Eth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d1 (Ntilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d2 (Ograve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d3 (Oacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d4 (Ocircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d5 (Otilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d6 (Odiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d7 (multiply) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d8 (Ooblique) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00d9 (Ugrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00da (Uacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00db (Ucircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00dc (Udiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00dd (Yacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00de (Thorn) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00df (ssharp) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e0 (agrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e1 (aacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e2 (acircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e3 (atilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e4 (adiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e5 (aring) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e6 (ae) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e7 (ccedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e8 (egrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00e9 (eacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ea (ecircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00eb (ediaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ec (igrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ed (iacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ee (icircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ef (idiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f0 (eth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f1 (ntilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f2 (ograve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f3 (oacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f4 (ocircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f5 (otilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f6 (odiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f7 (division) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f8 (ooblique) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00f9 (ugrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00fa (uacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00fb (ucircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00fc (udiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00fd (yacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00fe (thorn) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x00ff (ydiaeresis) */
    },
    {                                        /* 0x01?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0100 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0101 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0102 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0103 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0104 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0105 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0106 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0107 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0108 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0109 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x010a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x010b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x010c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x010d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x010e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x010f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0110 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0111 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0112 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0113 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0114 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0115 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0116 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0117 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0118 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0119 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x011a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x011b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x011c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x011d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x011e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x011f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0120 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0121 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0122 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0123 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0124 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0125 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0126 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0127 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0128 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0129 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x012a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x012b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x012c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x012d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x012e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x012f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0130 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0131 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0132 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0133 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0134 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0135 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0136 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0137 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0138 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0139 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x013a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x013b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x013c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x013d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x013e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x013f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0140 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0141 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0142 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0143 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0144 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0145 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0146 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0147 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0148 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0149 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x014a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x014b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x014c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x014d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x014e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x014f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0150 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0151 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0152 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0153 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0154 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0155 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0156 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0157 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0158 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0159 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x015a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x015b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x015c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x015d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x015e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x015f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0160 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0161 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0162 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0163 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0164 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0165 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0166 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0167 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0168 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0169 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x016a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x016b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x016c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x016d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x016e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x016f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0170 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0171 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0172 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0173 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0174 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0175 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0176 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0177 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0178 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0179 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x017a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x017b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x017c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x017d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x017e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x017f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0180 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0181 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0182 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0183 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0184 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0185 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0186 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0187 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0188 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0189 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x018a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x018b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x018c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x018d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x018e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x018f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0190 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0191 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0192 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0193 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0194 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0195 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0196 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0197 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0198 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0199 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x019a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x019b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x019c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x019d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x019e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x019f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a1 (Aogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a2 (breve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a3 (Lstroke) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a5 (Lcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a6 (Sacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01a9 (Scaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01aa (Scedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ab (Tcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ac (Zacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ae (Zcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01af (Zabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b1 (aogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b2 (ogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b3 (lstroke) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b5 (lcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b6 (sacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b7 (caron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01b9 (scaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ba (scedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01bb (tcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01bc (zacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01bd (doubleacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01be (zcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01bf (zabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c0 (Racute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c3 (Abreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c5 (Lacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c6 (Cacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c8 (Ccaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01c9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ca (Eogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01cb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01cc (Ecaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01cd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01cf (Dcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d0 (Dstroke) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d1 (Nacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d2 (Ncaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d5 (Odoubleacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d8 (Rcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01d9 (Uring) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01da */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01db (Udoubleacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01dd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01de (Tcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01df */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e0 (racute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e3 (abreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e5 (lacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e6 (cacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e8 (ccaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01e9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ea (eogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01eb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ec (ecaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ef (dcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f0 (dstroke) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f1 (nacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f2 (ncaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f5 (odoubleacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f8 (rcaron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01f9 (uring) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01fb (udoubleacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01fd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01fe (tcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x01ff (abovedot) */
    },
    {                                        /* 0x02?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0200 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0201 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0202 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0203 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0204 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0205 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0206 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0207 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0208 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0209 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x020a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x020b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x020c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x020d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x020e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x020f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0210 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0211 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0212 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0213 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0214 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0215 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0216 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0217 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0218 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0219 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x021a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x021b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x021c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x021d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x021e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x021f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0220 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0221 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0222 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0223 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0224 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0225 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0226 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0227 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0228 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0229 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x022a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x022b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x022c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x022d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x022e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x022f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0230 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0231 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0232 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0233 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0234 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0235 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0236 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0237 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0238 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0239 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x023a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x023b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x023c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x023d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x023e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x023f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0240 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0241 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0242 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0243 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0244 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0245 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0246 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0247 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0248 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0249 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x024a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x024b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x024c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x024d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x024e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x024f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0250 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0251 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0252 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0253 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0254 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0255 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0256 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0257 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0258 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0259 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x025a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x025b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x025c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x025d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x025e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x025f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0260 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0261 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0262 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0263 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0264 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0265 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0266 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0267 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0268 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0269 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x026a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x026b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x026c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x026d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x026e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x026f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0270 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0271 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0272 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0273 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0274 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0275 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0276 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0277 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0278 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0279 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x027a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x027b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x027c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x027d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x027e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x027f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0280 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0281 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0282 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0283 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0284 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0285 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0286 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0287 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0288 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0289 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x028a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x028b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x028c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x028d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x028e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x028f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0290 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0291 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0292 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0293 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0294 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0295 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0296 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0297 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0298 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0299 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x029a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x029b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x029c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x029d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x029e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x029f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a1 (Hstroke) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a6 (Hcircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02a9 (Iabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02aa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ab (Gbreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ac (Jcircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02af */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b1 (hstroke) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b6 (hcircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02b9 (idotless) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02bb (gbreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02bc (jcircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02bd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02be */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02bf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c5 (Cabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c6 (Ccircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02c9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02cb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02cc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02cd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02cf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d5 (Gabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d8 (Gcircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02d9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02da */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02db */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02dd (Ubreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02de (Scircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02df */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e5 (cabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e6 (ccircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02e9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02eb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ec */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f5 (gabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f8 (gcircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02f9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02fd (ubreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02fe (scircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x02ff */
    },
    {                                        /* 0x03?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0300 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0301 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0302 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0303 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0304 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0305 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0306 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0307 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0308 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0309 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x030a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x030b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x030c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x030d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x030e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x030f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0310 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0311 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0312 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0313 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0314 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0315 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0316 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0317 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0318 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0319 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x031a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x031b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x031c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x031d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x031e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x031f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0320 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0321 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0322 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0323 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0324 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0325 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0326 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0327 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0328 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0329 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x032a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x032b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x032c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x032d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x032e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x032f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0330 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0331 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0332 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0333 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0334 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0335 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0336 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0337 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0338 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0339 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x033a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x033b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x033c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x033d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x033e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x033f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0340 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0341 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0342 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0343 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0344 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0345 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0346 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0347 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0348 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0349 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x034a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x034b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x034c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x034d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x034e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x034f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0350 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0351 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0352 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0353 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0354 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0355 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0356 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0357 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0358 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0359 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x035a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x035b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x035c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x035d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x035e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x035f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0360 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0361 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0362 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0363 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0364 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0365 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0366 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0367 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0368 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0369 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x036a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x036b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x036c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x036d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x036e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x036f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0370 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0371 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0372 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0373 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0374 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0375 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0376 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0377 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0378 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0379 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x037a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x037b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x037c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x037d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x037e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x037f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0380 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0381 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0382 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0383 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0384 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0385 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0386 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0387 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0388 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0389 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x038a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x038b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x038c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x038d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x038e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x038f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0390 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0391 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0392 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0393 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0394 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0395 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0396 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0397 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0398 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0399 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x039a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x039b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x039c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x039d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x039e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x039f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a2 (kappa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a3 (Rcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a5 (Itilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a6 (Lcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03a9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03aa (Emacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ab (Gcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ac (Tslash) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03af */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b3 (rcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b5 (itilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b6 (lcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03b9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ba (emacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03bb (gcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03bc (tslash) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03bd (ENG) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03be */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03bf (eng) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c0 (Amacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c7 (Iogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03c9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03cb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03cc (Eabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03cd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03cf (Imacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d1 (Ncedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d2 (Omacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d3 (Kcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03d9 (Uogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03da */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03db */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03dd (Utilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03de (Umacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03df */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e0 (amacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e7 (iogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03e9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03eb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ec (eabovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ef (imacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f1 (ncedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f2 (omacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f3 (kcedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03f9 (uogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03fd (utilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03fe (umacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x03ff */
    },
    {                                        /* 0x04?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0400 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0401 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0402 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0403 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0404 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0405 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0406 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0407 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0408 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0409 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x040a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x040b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x040c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x040d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x040e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x040f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0410 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0411 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0412 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0413 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0414 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0415 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0416 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0417 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0418 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0419 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x041a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x041b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x041c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x041d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x041e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x041f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0420 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0421 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0422 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0423 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0424 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0425 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0426 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0427 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0428 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0429 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x042a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x042b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x042c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x042d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x042e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x042f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0430 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0431 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0432 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0433 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0434 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0435 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0436 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0437 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0438 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0439 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x043a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x043b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x043c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x043d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x043e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x043f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0440 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0441 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0442 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0443 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0444 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0445 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0446 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0447 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0448 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0449 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x044a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x044b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x044c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x044d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x044e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x044f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0450 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0451 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0452 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0453 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0454 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0455 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0456 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0457 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0458 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0459 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x045a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x045b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x045c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x045d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x045e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x045f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0460 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0461 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0462 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0463 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0464 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0465 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0466 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0467 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0468 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0469 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x046a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x046b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x046c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x046d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x046e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x046f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0470 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0471 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0472 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0473 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0474 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0475 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0476 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0477 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0478 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0479 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x047a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x047b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x047c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x047d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x047e (overline) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x047f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0480 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0481 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0482 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0483 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0484 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0485 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0486 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0487 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0488 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0489 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x048a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x048b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x048c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x048d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x048e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x048f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0490 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0491 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0492 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0493 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0494 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0495 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0496 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0497 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0498 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0499 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x049a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x049b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x049c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x049d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x049e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x049f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a1 (kana_fullstop) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a2 (kana_openingbracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a3 (kana_closingbracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a4 (kana_comma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a5 (kana_middledot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a6 (kana_WO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a7 (kana_a) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a8 (kana_i) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04a9 (kana_u) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04aa (kana_e) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ab (kana_o) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ac (kana_ya) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ad (kana_yu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ae (kana_yo) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04af (kana_tu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b0 (prolongedsound) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b1 (kana_A) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b2 (kana_I) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b3 (kana_U) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b4 (kana_E) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b5 (kana_O) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b6 (kana_KA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b7 (kana_KI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b8 (kana_KU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04b9 (kana_KE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ba (kana_KO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04bb (kana_SA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04bc (kana_SHI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04bd (kana_SU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04be (kana_SE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04bf (kana_SO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c0 (kana_TA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c1 (kana_TI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c2 (kana_TU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c3 (kana_TE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c4 (kana_TO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c5 (kana_NA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c6 (kana_NI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c7 (kana_NU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c8 (kana_NE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04c9 (kana_NO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ca (kana_HA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04cb (kana_HI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04cc (kana_HU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04cd (kana_HE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ce (kana_HO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04cf (kana_MA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d0 (kana_MI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d1 (kana_MU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d2 (kana_ME) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d3 (kana_MO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d4 (kana_YA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d5 (kana_YU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d6 (kana_YO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d7 (kana_RA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d8 (kana_RI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04d9 (kana_RU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04da (kana_RE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04db (kana_RO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04dc (kana_WA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04dd (kana_N) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04de (voicedsound) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04df (semivoicedsound) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04e9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04eb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ec */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04f9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04fd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04fe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x04ff */
    },
    {                                        /* 0x05?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0500 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0501 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0502 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0503 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0504 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0505 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0506 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0507 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0508 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0509 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x050a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x050b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x050c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x050d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x050e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x050f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0510 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0511 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0512 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0513 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0514 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0515 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0516 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0517 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0518 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0519 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x051a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x051b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x051c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x051d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x051e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x051f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0520 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0521 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0522 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0523 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0524 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0525 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0526 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0527 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0528 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0529 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x052a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x052b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x052c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x052d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x052e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x052f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0530 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0531 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0532 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0533 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0534 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0535 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0536 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0537 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0538 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0539 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x053a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x053b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x053c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x053d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x053e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x053f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0540 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0541 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0542 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0543 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0544 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0545 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0546 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0547 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0548 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0549 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x054a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x054b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x054c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x054d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x054e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x054f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0550 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0551 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0552 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0553 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0554 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0555 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0556 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0557 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0558 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0559 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x055a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x055b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x055c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x055d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x055e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x055f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0560 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0561 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0562 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0563 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0564 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0565 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0566 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0567 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0568 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0569 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x056a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x056b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x056c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x056d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x056e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x056f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0570 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0571 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0572 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0573 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0574 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0575 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0576 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0577 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0578 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0579 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x057a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x057b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x057c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x057d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x057e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x057f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0580 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0581 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0582 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0583 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0584 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0585 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0586 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0587 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0588 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0589 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x058a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x058b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x058c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x058d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x058e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x058f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0590 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0591 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0592 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0593 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0594 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0595 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0596 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0597 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0598 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0599 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x059a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x059b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x059c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x059d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x059e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x059f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05a9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05aa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ac (Arabic_comma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05af */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05b9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05bb (Arabic_semicolon) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05bc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05bd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05be */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05bf (Arabic_question_mark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c1 (Arabic_hamza) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c2 (Arabic_maddaonalef) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c3 (Arabic_hamzaonalef) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c4 (Arabic_hamzaonwaw) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c5 (Arabic_hamzaunderalef) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c6 (Arabic_hamzaonyeh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c7 (Arabic_alef) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c8 (Arabic_beh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05c9 (Arabic_tehmarbuta) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ca (Arabic_teh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05cb (Arabic_theh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05cc (Arabic_jeem) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05cd (Arabic_hah) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ce (Arabic_khah) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05cf (Arabic_dal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d0 (Arabic_thal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d1 (Arabic_ra) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d2 (Arabic_zain) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d3 (Arabic_seen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d4 (Arabic_sheen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d5 (Arabic_sad) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d6 (Arabic_dad) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d7 (Arabic_tah) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d8 (Arabic_zah) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05d9 (Arabic_ain) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05da (Arabic_ghain) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05db */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05dd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05de */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05df */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e0 (Arabic_tatweel) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e1 (Arabic_feh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e2 (Arabic_qaf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e3 (Arabic_kaf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e4 (Arabic_lam) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e5 (Arabic_meem) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e6 (Arabic_noon) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e7 (Arabic_heh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e8 (Arabic_waw) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05e9 (Arabic_alefmaksura) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ea (Arabic_yeh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05eb (Arabic_fathatan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ec (Arabic_dammatan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ed (Arabic_kasratan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ee (Arabic_fatha) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ef (Arabic_damma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f0 (Arabic_kasra) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f1 (Arabic_shadda) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f2 (Arabic_sukun) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05f9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05fd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05fe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x05ff */
    },
    {                                        /* 0x06?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0600 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0601 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0602 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0603 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0604 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0605 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0606 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0607 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0608 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0609 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x060a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x060b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x060c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x060d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x060e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x060f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0610 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0611 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0612 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0613 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0614 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0615 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0616 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0617 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0618 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0619 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x061a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x061b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x061c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x061d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x061e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x061f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0620 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0621 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0622 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0623 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0624 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0625 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0626 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0627 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0628 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0629 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x062a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x062b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x062c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x062d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x062e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x062f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0630 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0631 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0632 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0633 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0634 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0635 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0636 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0637 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0638 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0639 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x063a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x063b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x063c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x063d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x063e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x063f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0640 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0641 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0642 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0643 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0644 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0645 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0646 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0647 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0648 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0649 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x064a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x064b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x064c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x064d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x064e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x064f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0650 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0651 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0652 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0653 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0654 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0655 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0656 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0657 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0658 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0659 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x065a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x065b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x065c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x065d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x065e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x065f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0660 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0661 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0662 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0663 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0664 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0665 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0666 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0667 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0668 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0669 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x066a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x066b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x066c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x066d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x066e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x066f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0670 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0671 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0672 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0673 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0674 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0675 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0676 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0677 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0678 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0679 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x067a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x067b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x067c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x067d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x067e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x067f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0680 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0681 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0682 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0683 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0684 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0685 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0686 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0687 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0688 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0689 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x068a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x068b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x068c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x068d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x068e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x068f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0690 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0691 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0692 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0693 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0694 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0695 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0696 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0697 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0698 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0699 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x069a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x069b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x069c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x069d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x069e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x069f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a1 (Serbian_dje) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a2 (Macedonia_gje) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a3 (Cyrillic_io) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a4 (Ukranian_je) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a5 (Macedonia_dse) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a6 (Ukranian_i) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a7 (Ukranian_yi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a8 (Serbian_je) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06a9 (Serbian_lje) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06aa (Serbian_nje) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ab (Serbian_tshe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ac (Macedonia_kje) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ad (Ukrainian_ghe_with_upturn) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ae (Byelorussian_shortu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06af (Serbian_dze) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b0 (numerosign) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b1 (Serbian_DJE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b2 (Macedonia_GJE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b3 (Cyrillic_IO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b4 (Ukranian_JE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b5 (Macedonia_DSE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b6 (Ukranian_I) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b7 (Ukranian_YI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b8 (Serbian_JE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06b9 (Serbian_LJE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ba (Serbian_NJE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06bb (Serbian_TSHE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06bc (Macedonia_KJE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06bd (Ukrainian_GHE_WITH_UPTURN) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06be (Byelorussian_SHORTU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06bf (Serbian_DZE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c0 (Cyrillic_yu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c1 (Cyrillic_a) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c2 (Cyrillic_be) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c3 (Cyrillic_tse) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c4 (Cyrillic_de) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c5 (Cyrillic_ie) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c6 (Cyrillic_ef) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c7 (Cyrillic_ghe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c8 (Cyrillic_ha) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06c9 (Cyrillic_i) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ca (Cyrillic_shorti) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06cb (Cyrillic_ka) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06cc (Cyrillic_el) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06cd (Cyrillic_em) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ce (Cyrillic_en) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06cf (Cyrillic_o) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d0 (Cyrillic_pe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d1 (Cyrillic_ya) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d2 (Cyrillic_er) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d3 (Cyrillic_es) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d4 (Cyrillic_te) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d5 (Cyrillic_u) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d6 (Cyrillic_zhe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d7 (Cyrillic_ve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d8 (Cyrillic_softsign) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06d9 (Cyrillic_yeru) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06da (Cyrillic_ze) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06db (Cyrillic_sha) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06dc (Cyrillic_e) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06dd (Cyrillic_shcha) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06de (Cyrillic_che) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06df (Cyrillic_hardsign) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e0 (Cyrillic_YU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e1 (Cyrillic_A) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e2 (Cyrillic_BE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e3 (Cyrillic_TSE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e4 (Cyrillic_DE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e5 (Cyrillic_IE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e6 (Cyrillic_EF) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e7 (Cyrillic_GHE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e8 (Cyrillic_HA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06e9 (Cyrillic_I) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ea (Cyrillic_SHORTI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06eb (Cyrillic_KA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ec (Cyrillic_EL) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ed (Cyrillic_EM) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ee (Cyrillic_EN) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ef (Cyrillic_O) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f0 (Cyrillic_PE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f1 (Cyrillic_YA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f2 (Cyrillic_ER) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f3 (Cyrillic_ES) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f4 (Cyrillic_TE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f5 (Cyrillic_U) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f6 (Cyrillic_ZHE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f7 (Cyrillic_VE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f8 (Cyrillic_SOFTSIGN) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06f9 (Cyrillic_YERU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06fa (Cyrillic_ZE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06fb (Cyrillic_SHA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06fc (Cyrillic_E) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06fd (Cyrillic_SHCHA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06fe (Cyrillic_CHE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x06ff (Cyrillic_HARDSIGN) */
    },
    {                                        /* 0x07?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0700 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0701 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0702 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0703 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0704 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0705 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0706 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0707 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0708 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0709 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x070a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x070b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x070c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x070d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x070e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x070f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0710 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0711 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0712 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0713 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0714 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0715 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0716 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0717 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0718 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0719 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x071a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x071b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x071c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x071d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x071e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x071f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0720 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0721 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0722 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0723 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0724 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0725 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0726 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0727 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0728 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0729 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x072a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x072b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x072c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x072d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x072e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x072f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0730 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0731 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0732 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0733 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0734 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0735 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0736 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0737 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0738 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0739 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x073a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x073b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x073c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x073d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x073e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x073f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0740 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0741 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0742 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0743 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0744 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0745 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0746 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0747 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0748 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0749 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x074a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x074b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x074c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x074d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x074e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x074f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0750 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0751 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0752 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0753 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0754 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0755 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0756 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0757 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0758 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0759 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x075a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x075b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x075c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x075d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x075e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x075f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0760 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0761 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0762 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0763 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0764 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0765 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0766 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0767 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0768 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0769 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x076a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x076b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x076c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x076d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x076e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x076f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0770 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0771 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0772 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0773 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0774 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0775 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0776 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0777 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0778 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0779 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x077a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x077b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x077c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x077d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x077e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x077f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0780 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0781 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0782 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0783 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0784 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0785 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0786 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0787 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0788 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0789 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x078a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x078b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x078c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x078d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x078e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x078f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0790 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0791 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0792 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0793 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0794 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0795 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0796 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0797 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0798 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0799 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x079a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x079b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x079c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x079d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x079e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x079f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a1 (Greek_ALPHAaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a2 (Greek_EPSILONaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a3 (Greek_ETAaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a4 (Greek_IOTAaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a5 (Greek_IOTAdiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a7 (Greek_OMICRONaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a8 (Greek_UPSILONaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07a9 (Greek_UPSILONdieresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07aa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ab (Greek_OMEGAaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ac */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ae (Greek_accentdieresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07af (Greek_horizbar) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b1 (Greek_alphaaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b2 (Greek_epsilonaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b3 (Greek_etaaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b4 (Greek_iotaaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b5 (Greek_iotadieresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b6 (Greek_iotaaccentdieresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b7 (Greek_omicronaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b8 (Greek_upsilonaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07b9 (Greek_upsilondieresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ba (Greek_upsilonaccentdieresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07bb (Greek_omegaaccent) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07bc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07bd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07be */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07bf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c1 (Greek_ALPHA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c2 (Greek_BETA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c3 (Greek_GAMMA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c4 (Greek_DELTA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c5 (Greek_EPSILON) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c6 (Greek_ZETA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c7 (Greek_ETA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c8 (Greek_THETA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07c9 (Greek_IOTA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ca (Greek_KAPPA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07cb (Greek_LAMBDA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07cc (Greek_MU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07cd (Greek_NU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ce (Greek_XI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07cf (Greek_OMICRON) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d0 (Greek_PI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d1 (Greek_RHO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d2 (Greek_SIGMA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d4 (Greek_TAU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d5 (Greek_UPSILON) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d6 (Greek_PHI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d7 (Greek_CHI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d8 (Greek_PSI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07d9 (Greek_OMEGA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07da */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07db */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07dd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07de */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07df */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e1 (Greek_alpha) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e2 (Greek_beta) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e3 (Greek_gamma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e4 (Greek_delta) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e5 (Greek_epsilon) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e6 (Greek_zeta) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e7 (Greek_eta) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e8 (Greek_theta) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07e9 (Greek_iota) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ea (Greek_kappa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07eb (Greek_lambda) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ec (Greek_mu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ed (Greek_nu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ee (Greek_xi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ef (Greek_omicron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f0 (Greek_pi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f1 (Greek_rho) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f2 (Greek_sigma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f3 (Greek_finalsmallsigma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f4 (Greek_tau) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f5 (Greek_upsilon) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f6 (Greek_phi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f7 (Greek_chi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f8 (Greek_psi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07f9 (Greek_omega) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07fd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07fe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x07ff */
    },
    {                                        /* 0x08?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0800 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0801 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0802 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0803 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0804 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0805 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0806 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0807 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0808 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0809 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x080a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x080b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x080c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x080d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x080e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x080f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0810 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0811 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0812 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0813 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0814 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0815 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0816 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0817 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0818 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0819 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x081a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x081b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x081c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x081d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x081e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x081f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0820 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0821 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0822 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0823 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0824 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0825 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0826 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0827 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0828 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0829 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x082a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x082b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x082c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x082d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x082e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x082f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0830 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0831 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0832 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0833 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0834 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0835 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0836 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0837 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0838 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0839 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x083a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x083b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x083c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x083d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x083e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x083f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0840 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0841 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0842 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0843 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0844 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0845 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0846 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0847 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0848 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0849 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x084a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x084b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x084c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x084d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x084e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x084f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0850 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0851 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0852 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0853 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0854 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0855 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0856 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0857 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0858 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0859 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x085a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x085b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x085c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x085d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x085e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x085f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0860 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0861 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0862 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0863 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0864 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0865 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0866 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0867 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0868 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0869 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x086a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x086b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x086c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x086d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x086e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x086f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0870 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0871 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0872 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0873 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0874 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0875 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0876 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0877 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0878 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0879 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x087a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x087b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x087c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x087d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x087e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x087f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0880 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0881 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0882 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0883 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0884 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0885 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0886 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0887 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0888 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0889 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x088a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x088b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x088c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x088d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x088e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x088f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0890 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0891 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0892 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0893 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0894 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0895 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0896 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0897 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0898 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0899 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x089a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x089b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x089c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x089d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x089e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x089f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a1 (leftradical) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a2 (topleftradical) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a3 (horizconnector) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a4 (topintegral) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a5 (botintegral) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a6 (vertconnector) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a7 (topleftsqbracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a8 (botleftsqbracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08a9 (toprightsqbracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08aa (botrightsqbracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ab (topleftparens) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ac (botleftparens) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ad (toprightparens) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ae (botrightparens) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08af (leftmiddlecurlybrace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b0 (rightmiddlecurlybrace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b1 (topleftsummation) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b2 (botleftsummation) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b3 (topvertsummationconnector) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b4 (botvertsummationconnector) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b5 (toprightsummation) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b6 (botrightsummation) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b7 (rightmiddlesummation) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08b9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08bb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08bc (lessthanequal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08bd (notequal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08be (greaterthanequal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08bf (integral) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c0 (therefore) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c1 (variation) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c2 (infinity) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c5 (nabla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c8 (approximate) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08c9 (similarequal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08cb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08cc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08cd (ifonlyif) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ce (implies) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08cf (identical) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d6 (radical) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08d9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08da (includedin) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08db (includes) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08dc (intersection) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08dd (union) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08de (logicaland) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08df (logicalor) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08e9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08eb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ec */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ef (partialderivative) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f6 (function) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08f9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08fb (leftarrow) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08fc (uparrow) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08fd (rightarrow) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08fe (downarrow) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x08ff */
    },
    {                                        /* 0x09?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0900 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0901 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0902 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0903 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0904 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0905 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0906 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0907 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0908 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0909 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x090a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x090b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x090c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x090d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x090e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x090f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0910 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0911 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0912 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0913 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0914 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0915 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0916 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0917 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0918 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0919 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x091a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x091b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x091c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x091d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x091e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x091f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0920 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0921 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0922 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0923 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0924 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0925 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0926 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0927 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0928 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0929 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x092a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x092b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x092c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x092d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x092e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x092f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0930 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0931 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0932 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0933 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0934 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0935 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0936 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0937 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0938 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0939 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x093a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x093b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x093c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x093d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x093e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x093f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0940 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0941 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0942 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0943 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0944 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0945 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0946 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0947 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0948 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0949 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x094a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x094b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x094c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x094d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x094e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x094f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0950 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0951 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0952 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0953 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0954 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0955 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0956 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0957 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0958 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0959 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x095a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x095b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x095c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x095d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x095e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x095f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0960 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0961 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0962 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0963 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0964 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0965 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0966 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0967 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0968 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0969 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x096a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x096b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x096c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x096d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x096e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x096f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0970 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0971 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0972 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0973 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0974 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0975 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0976 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0977 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0978 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0979 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x097a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x097b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x097c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x097d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x097e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x097f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0980 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0981 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0982 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0983 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0984 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0985 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0986 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0987 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0988 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0989 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x098a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x098b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x098c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x098d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x098e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x098f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0990 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0991 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0992 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0993 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0994 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0995 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0996 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0997 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0998 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0999 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x099a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x099b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x099c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x099d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x099e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x099f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09a9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09aa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ac */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09af */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09b9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09bb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09bc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09bd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09be */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09bf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09c9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09cb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09cc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09cd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09cf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09d9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09da */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09db */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09dd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09de */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09df (blank) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e0 (soliddiamond) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e1 (checkerboard) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e2 (ht) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e3 (ff) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e4 (cr) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e5 (lf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e8 (nl) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09e9 (vt) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ea (lowrightcorner) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09eb (uprightcorner) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ec (upleftcorner) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ed (lowleftcorner) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ee (crossinglines) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ef (horizlinescan1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f0 (horizlinescan3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f1 (horizlinescan5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f2 (horizlinescan7) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f3 (horizlinescan9) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f4 (leftt) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f5 (rightt) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f6 (bott) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f7 (topt) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f8 (vertbar) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09f9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09fd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09fe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x09ff */
    },
    {                                        /* 0x0a?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a01 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a02 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a03 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a04 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a05 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a06 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a07 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a08 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a09 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a0a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a0b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a0c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a0d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a0e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a0f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a10 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a11 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a12 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a13 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a14 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a15 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a16 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a17 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a18 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a19 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a1a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a1b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a1c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a1d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a1e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a20 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a21 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a22 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a23 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a24 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a25 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a26 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a27 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a28 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a29 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a2a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a2b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a2c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a2d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a2e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a2f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a30 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a31 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a32 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a33 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a34 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a35 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a36 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a37 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a38 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a39 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a3a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a3b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a3c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a3d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a3e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a3f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a50 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a51 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a52 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a53 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a54 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a55 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a56 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a57 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a58 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a59 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a5a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a5b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a5c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a5d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a5e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a5f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a60 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a61 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a62 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a63 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a64 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a65 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a66 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a67 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a68 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a69 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a6a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a6b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a6c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a6d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a6e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a6f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a70 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a71 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a72 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a73 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a74 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a75 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a76 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a77 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a78 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a79 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a7a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a7e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a7f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a80 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a81 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a82 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a83 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a84 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a85 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a86 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a87 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a88 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a89 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a8a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a8b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a8d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a91 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a92 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a93 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a94 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a95 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a96 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a97 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a98 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a99 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a9a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a9b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a9c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a9d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a9e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0a9f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa1 (emspace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa2 (enspace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa3 (em3space) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa4 (em4space) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa5 (digitspace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa6 (punctspace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa7 (thinspace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa8 (hairspace) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aa9 (emdash) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aaa (endash) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aac (signifblank) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aae (ellipsis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aaf (doubbaselinedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab0 (onethird) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab1 (twothirds) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab2 (onefifth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab3 (twofifths) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab4 (threefifths) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab5 (fourfifths) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab6 (onesixth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab7 (fivesixths) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab8 (careof) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ab9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0abb (figdash) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0abc (leftanglebracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0abd (decimalpoint) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0abe (rightanglebracket) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0abf (marker) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac3 (oneeighth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac4 (threeeighths) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac5 (fiveeighths) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac6 (seveneighths) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ac9 (trademark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aca (signaturemark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0acb (trademarkincircle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0acc (leftopentriangle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0acd (rightopentriangle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ace (emopencircle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0acf (emopenrectangle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad0 (leftsinglequotemark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad1 (rightsinglequotemark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad2 (leftdoublequotemark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad3 (rightdoublequotemark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad4 (prescription) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad6 (minutes) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad7 (seconds) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ad9 (latincross) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ada (hexagram) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0adb (filledrectbullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0adc (filledlefttribullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0add (filledrighttribullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ade (emfilledcircle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0adf (emfilledrect) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae0 (enopencircbullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae1 (enopensquarebullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae2 (openrectbullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae3 (opentribulletup) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae4 (opentribulletdown) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae5 (openstar) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae6 (enfilledcircbullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae7 (enfilledsqbullet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae8 (filledtribulletup) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ae9 (filledtribulletdown) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aea (leftpointer) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aeb (rightpointer) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aec (club) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aed (diamond) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aee (heart) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af0 (maltesecross) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af1 (dagger) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af2 (doubledagger) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af3 (checkmark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af4 (ballotcross) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af5 (musicalsharp) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af6 (musicalflat) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af7 (malesymbol) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af8 (femalesymbol) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0af9 (telephone) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0afa (telephonerecorder) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0afb (phonographcopyright) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0afc (caret) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0afd (singlelowquotemark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0afe (doublelowquotemark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0aff (cursor) */
    },
    {                                        /* 0x0b?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b01 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b02 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b03 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b04 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b05 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b06 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b07 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b08 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b09 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b0a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b0b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b0c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b0d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b0e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b0f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b10 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b11 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b12 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b13 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b14 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b15 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b16 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b17 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b18 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b19 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b1a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b1b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b1c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b1d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b1e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b20 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b21 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b22 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b23 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b24 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b25 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b26 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b27 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b28 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b29 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b2a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b2b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b2c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b2d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b2e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b2f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b30 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b31 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b32 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b33 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b34 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b35 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b36 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b37 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b38 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b39 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b3a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b3b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b3c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b3d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b3e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b3f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b50 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b51 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b52 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b53 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b54 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b55 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b56 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b57 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b58 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b59 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b5a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b5b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b5c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b5d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b5e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b5f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b60 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b61 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b62 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b63 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b64 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b65 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b66 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b67 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b68 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b69 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b6a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b6b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b6c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b6d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b6e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b6f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b70 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b71 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b72 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b73 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b74 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b75 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b76 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b77 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b78 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b79 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b7a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b7e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b7f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b80 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b81 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b82 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b83 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b84 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b85 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b86 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b87 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b88 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b89 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b8a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b8b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b8d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b91 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b92 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b93 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b94 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b95 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b96 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b97 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b98 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b99 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b9a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b9b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b9c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b9d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b9e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0b9f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba3 (leftcaret) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba6 (rightcaret) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba8 (downcaret) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ba9 (upcaret) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0baa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bac */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0baf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bb9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bbb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bbc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bbd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bbe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bbf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc0 (overbar) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc2 (downtack) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc3 (upshoe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc4 (downstile) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc6 (underbar) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bc9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bca (jot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bcb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bcc (quad) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bcd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bce (uptack) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bcf (circle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd3 (upstile) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd6 (downshoe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd8 (rightshoe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bd9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bda (leftshoe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bdb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bdc (lefttack) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bdd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bde */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bdf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0be9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0beb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bec */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bf9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bfa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bfb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bfc (righttack) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bfd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bfe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0bff */
    },
    {                                        /* 0x0c?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c01 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c02 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c03 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c04 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c05 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c06 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c07 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c08 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c09 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c0a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c0b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c0c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c0d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c0e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c0f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c10 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c11 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c12 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c13 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c14 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c15 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c16 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c17 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c18 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c19 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c1a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c1b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c1c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c1d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c1e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c20 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c21 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c22 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c23 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c24 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c25 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c26 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c27 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c28 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c29 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c2a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c2b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c2c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c2d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c2e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c2f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c30 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c31 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c32 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c33 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c34 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c35 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c36 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c37 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c38 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c39 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c3a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c3b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c3c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c3d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c3e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c3f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c50 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c51 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c52 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c53 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c54 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c55 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c56 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c57 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c58 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c59 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c5a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c5b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c5c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c5d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c5e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c5f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c60 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c61 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c62 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c63 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c64 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c65 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c66 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c67 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c68 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c69 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c6a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c6b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c6c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c6d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c6e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c6f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c70 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c71 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c72 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c73 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c74 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c75 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c76 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c77 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c78 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c79 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c7a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c7e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c7f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c80 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c81 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c82 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c83 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c84 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c85 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c86 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c87 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c88 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c89 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c8a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c8b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c8d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c91 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c92 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c93 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c94 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c95 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c96 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c97 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c98 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c99 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c9a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c9b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c9c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c9d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c9e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0c9f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ca9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0caa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cac */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0caf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cb9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cbb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cbc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cbd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cbe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cbf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cc9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ccb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ccc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ccd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ccf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cd9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cda */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cdb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cdc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cdd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cde */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cdf (hebrew_doublelowline) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce0 (hebrew_aleph) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce1 (hebrew_beth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce2 (hebrew_gimmel) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce3 (hebrew_daleth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce4 (hebrew_he) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce5 (hebrew_waw) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce6 (hebrew_zayin) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce7 (hebrew_het) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce8 (hebrew_teth) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ce9 (hebrew_yod) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cea (hebrew_finalkaph) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ceb (hebrew_kaph) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cec (hebrew_lamed) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ced (hebrew_finalmem) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cee (hebrew_mem) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cef (hebrew_finalnun) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf0 (hebrew_nun) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf1 (hebrew_samekh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf2 (hebrew_ayin) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf3 (hebrew_finalpe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf4 (hebrew_pe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf5 (hebrew_finalzadi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf6 (hebrew_zadi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf7 (hebrew_kuf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf8 (hebrew_resh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cf9 (hebrew_shin) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cfa (hebrew_taf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cfb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cfc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cfd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cfe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0cff */
    },
    {                                        /* 0x0d?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d01 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d02 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d03 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d04 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d05 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d06 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d07 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d08 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d09 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d0a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d0b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d0c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d0d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d0e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d0f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d10 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d11 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d12 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d13 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d14 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d15 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d16 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d17 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d18 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d19 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d1a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d1b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d1c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d1d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d1e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d20 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d21 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d22 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d23 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d24 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d25 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d26 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d27 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d28 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d29 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d2a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d2b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d2c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d2d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d2e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d2f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d30 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d31 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d32 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d33 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d34 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d35 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d36 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d37 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d38 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d39 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d3a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d3b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d3c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d3d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d3e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d3f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d50 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d51 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d52 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d53 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d54 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d55 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d56 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d57 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d58 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d59 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d5a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d5b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d5c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d5d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d5e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d5f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d60 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d61 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d62 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d63 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d64 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d65 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d66 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d67 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d68 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d69 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d6a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d6b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d6c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d6d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d6e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d6f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d70 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d71 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d72 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d73 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d74 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d75 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d76 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d77 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d78 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d79 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d7a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d7e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d7f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d80 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d81 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d82 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d83 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d84 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d85 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d86 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d87 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d88 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d89 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d8a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d8b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d8d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d91 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d92 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d93 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d94 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d95 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d96 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d97 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d98 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d99 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d9a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d9b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d9c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d9d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d9e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0d9f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da1 (Thai_kokai) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da2 (Thai_khokhai) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da3 (Thai_khokhuat) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da4 (Thai_khokhwai) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da5 (Thai_khokhon) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da6 (Thai_khorakhang) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da7 (Thai_ngongu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da8 (Thai_chochan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0da9 (Thai_choching) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0daa (Thai_chochang) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dab (Thai_soso) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dac (Thai_chochoe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dad (Thai_yoying) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dae (Thai_dochada) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0daf (Thai_topatak) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db0 (Thai_thothan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db1 (Thai_thonangmontho) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db2 (Thai_thophuthao) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db3 (Thai_nonen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db4 (Thai_dodek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db5 (Thai_totao) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db6 (Thai_thothung) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db7 (Thai_thothahan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db8 (Thai_thothong) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0db9 (Thai_nonu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dba (Thai_bobaimai) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dbb (Thai_popla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dbc (Thai_phophung) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dbd (Thai_fofa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dbe (Thai_phophan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dbf (Thai_fofan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc0 (Thai_phosamphao) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc1 (Thai_moma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc2 (Thai_yoyak) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc3 (Thai_rorua) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc4 (Thai_ru) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc5 (Thai_loling) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc6 (Thai_lu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc7 (Thai_wowaen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc8 (Thai_sosala) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dc9 (Thai_sorusi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dca (Thai_sosua) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dcb (Thai_hohip) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dcc (Thai_lochula) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dcd (Thai_oang) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dce (Thai_honokhuk) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dcf (Thai_paiyannoi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd0 (Thai_saraa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd1 (Thai_maihanakat) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd2 (Thai_saraaa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd3 (Thai_saraam) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd4 (Thai_sarai) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd5 (Thai_saraii) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd6 (Thai_saraue) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd7 (Thai_sarauee) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd8 (Thai_sarau) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dd9 (Thai_sarauu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dda (Thai_phinthu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ddb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ddc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ddd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dde (Thai_maihanakat_maitho) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ddf (Thai_baht) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de0 (Thai_sarae) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de1 (Thai_saraae) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de2 (Thai_sarao) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de3 (Thai_saraaimaimuan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de4 (Thai_saraaimaimalai) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de5 (Thai_lakkhangyao) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de6 (Thai_maiyamok) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de7 (Thai_maitaikhu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de8 (Thai_maiek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0de9 (Thai_maitho) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dea (Thai_maitri) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0deb (Thai_maichattawa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dec (Thai_thanthakhat) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ded (Thai_nikhahit) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0def */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df0 (Thai_leksun) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df1 (Thai_leknung) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df2 (Thai_leksong) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df3 (Thai_leksam) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df4 (Thai_leksi) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df5 (Thai_lekha) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df6 (Thai_lekhok) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df7 (Thai_lekchet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df8 (Thai_lekpaet) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0df9 (Thai_lekkao) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dfa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dfb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dfc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dfd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dfe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0dff */
    },
    {                                        /* 0x0e?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e01 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e02 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e03 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e04 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e05 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e06 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e07 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e08 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e09 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e0a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e0b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e0c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e0d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e0e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e0f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e10 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e11 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e12 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e13 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e14 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e15 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e16 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e17 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e18 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e19 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e1a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e1b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e1c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e1d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e1e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e20 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e21 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e22 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e23 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e24 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e25 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e26 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e27 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e28 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e29 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e2a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e2b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e2c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e2d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e2e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e2f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e30 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e31 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e32 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e33 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e34 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e35 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e36 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e37 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e38 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e39 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e3a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e3b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e3c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e3d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e3e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e3f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e50 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e51 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e52 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e53 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e54 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e55 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e56 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e57 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e58 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e59 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e5a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e5b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e5c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e5d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e5e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e5f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e60 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e61 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e62 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e63 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e64 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e65 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e66 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e67 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e68 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e69 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e6a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e6b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e6c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e6d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e6e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e6f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e70 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e71 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e72 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e73 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e74 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e75 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e76 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e77 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e78 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e79 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e7a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e7e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e7f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e80 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e81 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e82 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e83 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e84 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e85 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e86 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e87 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e88 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e89 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e8a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e8b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e8d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e91 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e92 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e93 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e94 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e95 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e96 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e97 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e98 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e99 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e9a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e9b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e9c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e9d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e9e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0e9f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea1 (Hangul_Kiyeog) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea2 (Hangul_SsangKiyeog) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea3 (Hangul_KiyeogSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea4 (Hangul_Nieun) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea5 (Hangul_NieunJieuj) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea6 (Hangul_NieunHieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea7 (Hangul_Dikeud) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea8 (Hangul_SsangDikeud) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ea9 (Hangul_Rieul) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eaa (Hangul_RieulKiyeog) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eab (Hangul_RieulMieum) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eac (Hangul_RieulPieub) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ead (Hangul_RieulSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eae (Hangul_RieulTieut) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eaf (Hangul_RieulPhieuf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb0 (Hangul_RieulHieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb1 (Hangul_Mieum) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb2 (Hangul_Pieub) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb3 (Hangul_SsangPieub) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb4 (Hangul_PieubSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb5 (Hangul_Sios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb6 (Hangul_SsangSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb7 (Hangul_Ieung) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb8 (Hangul_Jieuj) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eb9 (Hangul_SsangJieuj) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eba (Hangul_Cieuc) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ebb (Hangul_Khieuq) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ebc (Hangul_Tieut) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ebd (Hangul_Phieuf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ebe (Hangul_Hieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ebf (Hangul_A) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec0 (Hangul_AE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec1 (Hangul_YA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec2 (Hangul_YAE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec3 (Hangul_EO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec4 (Hangul_E) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec5 (Hangul_YEO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec6 (Hangul_YE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec7 (Hangul_O) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec8 (Hangul_WA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ec9 (Hangul_WAE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eca (Hangul_OE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ecb (Hangul_YO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ecc (Hangul_U) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ecd (Hangul_WEO) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ece (Hangul_WE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ecf (Hangul_WI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed0 (Hangul_YU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed1 (Hangul_EU) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed2 (Hangul_YI) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed3 (Hangul_I) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed4 (Hangul_J_Kiyeog) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed5 (Hangul_J_SsangKiyeog) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed6 (Hangul_J_KiyeogSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed7 (Hangul_J_Nieun) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed8 (Hangul_J_NieunJieuj) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ed9 (Hangul_J_NieunHieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eda (Hangul_J_Dikeud) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0edb (Hangul_J_Rieul) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0edc (Hangul_J_RieulKiyeog) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0edd (Hangul_J_RieulMieum) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ede (Hangul_J_RieulPieub) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0edf (Hangul_J_RieulSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee0 (Hangul_J_RieulTieut) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee1 (Hangul_J_RieulPhieuf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee2 (Hangul_J_RieulHieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee3 (Hangul_J_Mieum) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee4 (Hangul_J_Pieub) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee5 (Hangul_J_PieubSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee6 (Hangul_J_Sios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee7 (Hangul_J_SsangSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee8 (Hangul_J_Ieung) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ee9 (Hangul_J_Jieuj) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eea (Hangul_J_Cieuc) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eeb (Hangul_J_Khieuq) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eec (Hangul_J_Tieut) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eed (Hangul_J_Phieuf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eee (Hangul_J_Hieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eef (Hangul_RieulYeorinHieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef0 (Hangul_SunkyeongeumMieum) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef1 (Hangul_SunkyeongeumPieub) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef2 (Hangul_PanSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef3 (Hangul_KkogjiDalrinIeung) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef4 (Hangul_SunkyeongeumPhieuf) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef5 (Hangul_YeorinHieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef6 (Hangul_AraeA) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef7 (Hangul_AraeAE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef8 (Hangul_J_PanSios) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0ef9 (Hangul_J_KkogjiDalrinIeung) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0efa (Hangul_J_YeorinHieuh) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0efb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0efc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0efd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0efe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x0eff (Korean_Won) */
    },
	{{0}}, /* 0x0f?? */
	{{0}}, /* 0x10?? */
	{{0}}, /* 0x11?? */
	{{0}}, /* 0x12?? */
    {                                        /* 0x13?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1300 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1301 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1302 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1303 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1304 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1305 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1306 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1307 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1308 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1309 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x130a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x130b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x130c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x130d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x130e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x130f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1310 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1311 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1312 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1313 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1314 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1315 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1316 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1317 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1318 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1319 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x131a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x131b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x131c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x131d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x131e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x131f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1320 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1321 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1322 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1323 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1324 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1325 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1326 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1327 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1328 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1329 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x132a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x132b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x132c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x132d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x132e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x132f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1330 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1331 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1332 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1333 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1334 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1335 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1336 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1337 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1338 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1339 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x133a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x133b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x133c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x133d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x133e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x133f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1340 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1341 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1342 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1343 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1344 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1345 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1346 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1347 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1348 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1349 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x134a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x134b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x134c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x134d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x134e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x134f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1350 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1351 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1352 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1353 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1354 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1355 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1356 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1357 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1358 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1359 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x135a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x135b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x135c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x135d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x135e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x135f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1360 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1361 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1362 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1363 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1364 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1365 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1366 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1367 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1368 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1369 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x136a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x136b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x136c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x136d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x136e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x136f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1370 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1371 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1372 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1373 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1374 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1375 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1376 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1377 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1378 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1379 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x137a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x137b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x137c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x137d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x137e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x137f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1380 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1381 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1382 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1383 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1384 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1385 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1386 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1387 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1388 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1389 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x138a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x138b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x138c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x138d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x138e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x138f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1390 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1391 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1392 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1393 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1394 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1395 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1396 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1397 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1398 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x1399 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x139a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x139b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x139c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x139d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x139e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x139f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13a9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13aa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ac */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13af */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13b9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13bb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13bc (OE) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13bd (oe) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13be (Ydiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13bf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13c9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13cb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13cc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13cd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13cf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13d9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13da */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13db */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13dd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13de */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13df */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13e9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13eb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ec */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13f9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13fd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13fe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x13ff */
    },
	{{0}}, /* 0x14?? */
	{{0}}, /* 0x15?? */
	{{0}}, /* 0x16?? */
	{{0}}, /* 0x17?? */
	{{0}}, /* 0x18?? */
	{{0}}, /* 0x19?? */
	{{0}}, /* 0x1a?? */
	{{0}}, /* 0x1b?? */
	{{0}}, /* 0x1c?? */
	{{0}}, /* 0x1d?? */
	{{0}}, /* 0x1e?? */
	{{0}}, /* 0x1f?? */
    {                                        /* 0x20?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2000 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2001 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2002 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2003 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2004 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2005 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2006 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2007 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2008 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2009 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x200a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x200b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x200c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x200d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x200e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x200f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2010 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2011 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2012 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2013 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2014 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2015 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2016 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2017 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2018 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2019 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x201a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x201b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x201c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x201d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x201e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x201f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2020 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2021 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2022 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2023 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2024 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2025 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2026 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2027 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2028 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2029 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x202a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x202b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x202c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x202d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x202e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x202f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2030 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2031 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2032 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2033 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2034 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2035 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2036 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2037 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2038 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2039 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x203a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x203b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x203c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x203d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x203e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x203f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2040 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2041 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2042 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2043 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2044 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2045 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2046 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2047 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2048 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2049 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x204a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x204b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x204c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x204d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x204e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x204f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2050 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2051 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2052 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2053 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2054 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2055 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2056 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2057 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2058 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2059 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x205a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x205b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x205c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x205d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x205e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x205f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2060 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2061 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2062 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2063 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2064 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2065 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2066 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2067 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2068 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2069 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x206a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x206b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x206c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x206d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x206e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x206f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2070 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2071 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2072 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2073 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2074 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2075 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2076 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2077 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2078 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2079 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x207a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x207b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x207c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x207d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x207e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x207f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2080 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2081 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2082 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2083 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2084 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2085 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2086 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2087 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2088 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2089 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x208a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x208b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x208c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x208d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x208e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x208f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2090 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2091 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2092 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2093 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2094 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2095 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2096 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2097 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2098 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x2099 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x209a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x209b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x209c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x209d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x209e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x209f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20a9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20aa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ac (EuroSign) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20af */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20b9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20bb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20bc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20bd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20be */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20bf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20c9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20cb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20cc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20cd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20cf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20d9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20da */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20db */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20dc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20dd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20de */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20df */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20e9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20eb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ec */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ed */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20f9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20fa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20fb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20fc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20fd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20fe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0x20ff */
    },
	{{0}}, /* 0x21?? */
	{{0}}, /* 0x22?? */
	{{0}}, /* 0x23?? */
	{{0}}, /* 0x24?? */
	{{0}}, /* 0x25?? */
	{{0}}, /* 0x26?? */
	{{0}}, /* 0x27?? */
	{{0}}, /* 0x28?? */
	{{0}}, /* 0x29?? */
	{{0}}, /* 0x2a?? */
	{{0}}, /* 0x2b?? */
	{{0}}, /* 0x2c?? */
	{{0}}, /* 0x2d?? */
	{{0}}, /* 0x2e?? */
	{{0}}, /* 0x2f?? */
	{{0}}, /* 0x30?? */
	{{0}}, /* 0x31?? */
	{{0}}, /* 0x32?? */
	{{0}}, /* 0x33?? */
	{{0}}, /* 0x34?? */
	{{0}}, /* 0x35?? */
	{{0}}, /* 0x36?? */
	{{0}}, /* 0x37?? */
	{{0}}, /* 0x38?? */
	{{0}}, /* 0x39?? */
	{{0}}, /* 0x3a?? */
	{{0}}, /* 0x3b?? */
	{{0}}, /* 0x3c?? */
	{{0}}, /* 0x3d?? */
	{{0}}, /* 0x3e?? */
	{{0}}, /* 0x3f?? */
	{{0}}, /* 0x40?? */
	{{0}}, /* 0x41?? */
	{{0}}, /* 0x42?? */
	{{0}}, /* 0x43?? */
	{{0}}, /* 0x44?? */
	{{0}}, /* 0x45?? */
	{{0}}, /* 0x46?? */
	{{0}}, /* 0x47?? */
	{{0}}, /* 0x48?? */
	{{0}}, /* 0x49?? */
	{{0}}, /* 0x4a?? */
	{{0}}, /* 0x4b?? */
	{{0}}, /* 0x4c?? */
	{{0}}, /* 0x4d?? */
	{{0}}, /* 0x4e?? */
	{{0}}, /* 0x4f?? */
	{{0}}, /* 0x50?? */
	{{0}}, /* 0x51?? */
	{{0}}, /* 0x52?? */
	{{0}}, /* 0x53?? */
	{{0}}, /* 0x54?? */
	{{0}}, /* 0x55?? */
	{{0}}, /* 0x56?? */
	{{0}}, /* 0x57?? */
	{{0}}, /* 0x58?? */
	{{0}}, /* 0x59?? */
	{{0}}, /* 0x5a?? */
	{{0}}, /* 0x5b?? */
	{{0}}, /* 0x5c?? */
	{{0}}, /* 0x5d?? */
	{{0}}, /* 0x5e?? */
	{{0}}, /* 0x5f?? */
	{{0}}, /* 0x60?? */
	{{0}}, /* 0x61?? */
	{{0}}, /* 0x62?? */
	{{0}}, /* 0x63?? */
	{{0}}, /* 0x64?? */
	{{0}}, /* 0x65?? */
	{{0}}, /* 0x66?? */
	{{0}}, /* 0x67?? */
	{{0}}, /* 0x68?? */
	{{0}}, /* 0x69?? */
	{{0}}, /* 0x6a?? */
	{{0}}, /* 0x6b?? */
	{{0}}, /* 0x6c?? */
	{{0}}, /* 0x6d?? */
	{{0}}, /* 0x6e?? */
	{{0}}, /* 0x6f?? */
	{{0}}, /* 0x70?? */
	{{0}}, /* 0x71?? */
	{{0}}, /* 0x72?? */
	{{0}}, /* 0x73?? */
	{{0}}, /* 0x74?? */
	{{0}}, /* 0x75?? */
	{{0}}, /* 0x76?? */
	{{0}}, /* 0x77?? */
	{{0}}, /* 0x78?? */
	{{0}}, /* 0x79?? */
	{{0}}, /* 0x7a?? */
	{{0}}, /* 0x7b?? */
	{{0}}, /* 0x7c?? */
	{{0}}, /* 0x7d?? */
	{{0}}, /* 0x7e?? */
	{{0}}, /* 0x7f?? */
	{{0}}, /* 0x80?? */
	{{0}}, /* 0x81?? */
	{{0}}, /* 0x82?? */
	{{0}}, /* 0x83?? */
	{{0}}, /* 0x84?? */
	{{0}}, /* 0x85?? */
	{{0}}, /* 0x86?? */
	{{0}}, /* 0x87?? */
	{{0}}, /* 0x88?? */
	{{0}}, /* 0x89?? */
	{{0}}, /* 0x8a?? */
	{{0}}, /* 0x8b?? */
	{{0}}, /* 0x8c?? */
	{{0}}, /* 0x8d?? */
	{{0}}, /* 0x8e?? */
	{{0}}, /* 0x8f?? */
	{{0}}, /* 0x90?? */
	{{0}}, /* 0x91?? */
	{{0}}, /* 0x92?? */
	{{0}}, /* 0x93?? */
	{{0}}, /* 0x94?? */
	{{0}}, /* 0x95?? */
	{{0}}, /* 0x96?? */
	{{0}}, /* 0x97?? */
	{{0}}, /* 0x98?? */
	{{0}}, /* 0x99?? */
	{{0}}, /* 0x9a?? */
	{{0}}, /* 0x9b?? */
	{{0}}, /* 0x9c?? */
	{{0}}, /* 0x9d?? */
	{{0}}, /* 0x9e?? */
	{{0}}, /* 0x9f?? */
	{{0}}, /* 0xa0?? */
	{{0}}, /* 0xa1?? */
	{{0}}, /* 0xa2?? */
	{{0}}, /* 0xa3?? */
	{{0}}, /* 0xa4?? */
	{{0}}, /* 0xa5?? */
	{{0}}, /* 0xa6?? */
	{{0}}, /* 0xa7?? */
	{{0}}, /* 0xa8?? */
	{{0}}, /* 0xa9?? */
	{{0}}, /* 0xaa?? */
	{{0}}, /* 0xab?? */
	{{0}}, /* 0xac?? */
	{{0}}, /* 0xad?? */
	{{0}}, /* 0xae?? */
	{{0}}, /* 0xaf?? */
	{{0}}, /* 0xb0?? */
	{{0}}, /* 0xb1?? */
	{{0}}, /* 0xb2?? */
	{{0}}, /* 0xb3?? */
	{{0}}, /* 0xb4?? */
	{{0}}, /* 0xb5?? */
	{{0}}, /* 0xb6?? */
	{{0}}, /* 0xb7?? */
	{{0}}, /* 0xb8?? */
	{{0}}, /* 0xb9?? */
	{{0}}, /* 0xba?? */
	{{0}}, /* 0xbb?? */
	{{0}}, /* 0xbc?? */
	{{0}}, /* 0xbd?? */
	{{0}}, /* 0xbe?? */
	{{0}}, /* 0xbf?? */
	{{0}}, /* 0xc0?? */
	{{0}}, /* 0xc1?? */
	{{0}}, /* 0xc2?? */
	{{0}}, /* 0xc3?? */
	{{0}}, /* 0xc4?? */
	{{0}}, /* 0xc5?? */
	{{0}}, /* 0xc6?? */
	{{0}}, /* 0xc7?? */
	{{0}}, /* 0xc8?? */
	{{0}}, /* 0xc9?? */
	{{0}}, /* 0xca?? */
	{{0}}, /* 0xcb?? */
	{{0}}, /* 0xcc?? */
	{{0}}, /* 0xcd?? */
	{{0}}, /* 0xce?? */
	{{0}}, /* 0xcf?? */
	{{0}}, /* 0xd0?? */
	{{0}}, /* 0xd1?? */
	{{0}}, /* 0xd2?? */
	{{0}}, /* 0xd3?? */
	{{0}}, /* 0xd4?? */
	{{0}}, /* 0xd5?? */
	{{0}}, /* 0xd6?? */
	{{0}}, /* 0xd7?? */
	{{0}}, /* 0xd8?? */
	{{0}}, /* 0xd9?? */
	{{0}}, /* 0xda?? */
	{{0}}, /* 0xdb?? */
	{{0}}, /* 0xdc?? */
	{{0}}, /* 0xdd?? */
	{{0}}, /* 0xde?? */
	{{0}}, /* 0xdf?? */
	{{0}}, /* 0xe0?? */
	{{0}}, /* 0xe1?? */
	{{0}}, /* 0xe2?? */
	{{0}}, /* 0xe3?? */
	{{0}}, /* 0xe4?? */
	{{0}}, /* 0xe5?? */
	{{0}}, /* 0xe6?? */
	{{0}}, /* 0xe7?? */
	{{0}}, /* 0xe8?? */
	{{0}}, /* 0xe9?? */
	{{0}}, /* 0xea?? */
	{{0}}, /* 0xeb?? */
	{{0}}, /* 0xec?? */
	{{0}}, /* 0xed?? */
	{{0}}, /* 0xee?? */
	{{0}}, /* 0xef?? */
	{{0}}, /* 0xf0?? */
	{{0}}, /* 0xf1?? */
	{{0}}, /* 0xf2?? */
	{{0}}, /* 0xf3?? */
	{{0}}, /* 0xf4?? */
	{{0}}, /* 0xf5?? */
	{{0}}, /* 0xf6?? */
	{{0}}, /* 0xf7?? */
	{{0}}, /* 0xf8?? */
	{{0}}, /* 0xf9?? */
	{{0}}, /* 0xfa?? */
	{{0}}, /* 0xfb?? */
	{{0}}, /* 0xfc?? */
    {                                        /* 0xfd?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd01 (3270_Duplicate) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd02 (3270_FieldMark) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd03 (3270_Right2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd04 (3270_Left2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd05 (3270_BackTab) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd06 (3270_EraseEOF) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd07 (3270_EraseInput) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd08 (3270_Reset) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd09 (3270_Quit) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd0a (3270_PA1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd0b (3270_PA2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd0c (3270_PA3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd0d (3270_Test) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd0e (3270_Attn) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd0f (3270_CursorBlink) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd10 (3270_AltCursor) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd11 (3270_KeyClick) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd12 (3270_Jump) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd13 (3270_Ident) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd14 (3270_Rule) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd15 (3270_Copy) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd16 (3270_Play) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd17 (3270_Setup) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd18 (3270_Record) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd19 (3270_ChangeScreen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd1a (3270_DeleteWord) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd1b (3270_ExSelect) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd1c (3270_CursorSelect) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd1d (3270_PrintScreen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd1e (3270_Enter) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd20 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd21 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd22 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd23 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd24 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd25 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd26 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd27 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd28 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd29 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd2a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd2b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd2c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd2d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd2e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd2f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd30 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd31 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd32 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd33 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd34 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd35 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd36 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd37 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd38 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd39 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd3a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd3b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd3c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd3d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd3e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd3f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd50 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd51 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd52 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd53 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd54 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd55 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd56 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd57 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd58 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd59 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd5a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd5b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd5c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd5d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd5e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd5f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd60 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd61 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd62 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd63 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd64 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd65 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd66 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd67 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd68 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd69 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd6a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd6b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd6c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd6d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd6e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd6f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd70 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd71 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd72 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd73 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd74 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd75 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd76 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd77 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd78 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd79 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd7a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd7e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd7f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd80 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd81 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd82 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd83 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd84 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd85 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd86 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd87 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd88 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd89 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd8a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd8b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd8d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd91 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd92 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd93 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd94 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd95 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd96 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd97 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd98 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd99 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd9a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd9b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd9c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd9d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd9e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfd9f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfda9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdaa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdac */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdad */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdaf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdb9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdbb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdbc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdbd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdbe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdbf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdc9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdcb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdcc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdcd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdce */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdcf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdd9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdda */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfddb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfddc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfddd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdde */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfddf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfde9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdea */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdeb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdec */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfded */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdee */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdf9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdfa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdfb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdfc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdfd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdfe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfdff */
    },
    {                                        /* 0xfe?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe01 (ISO_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe02 (ISO_Level2_Latch) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe03 (ISO_Level3_Shift) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe04 (ISO_Level3_Latch) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe05 (ISO_Level3_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe06 (ISO_Group_Latch) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe07 (ISO_Group_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe08 (ISO_Next_Group) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe09 (ISO_Next_Group_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe0a (ISO_Prev_Group) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe0b (ISO_Prev_Group_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe0c (ISO_First_Group) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe0d (ISO_First_Group_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe0e (ISO_Last_Group) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe0f (ISO_Last_Group_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe10 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe11 (ISO_Level5_Shift) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe12 (ISO_Level5_Latch) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe13 (ISO_Level5_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe14 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe15 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe16 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe17 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe18 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe19 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe1a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe1b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe1c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe1d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe1e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe20 (ISO_Left_Tab) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe21 (ISO_Move_Line_Up) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe22 (ISO_Move_Line_Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe23 (ISO_Partial_Line_Up) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe24 (ISO_Partial_Line_Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe25 (ISO_Partial_Space_Left) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe26 (ISO_Partial_Space_Right) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe27 (ISO_Set_Margin_Left) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe28 (ISO_Set_Margin_Right) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe29 (ISO_Release_Margin_Left) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe2a (ISO_Release_Margin_Right) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe2b (ISO_Release_Both_Margins) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe2c (ISO_Fast_Cursor_Left) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe2d (ISO_Fast_Cursor_Right) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe2e (ISO_Fast_Cursor_Up) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe2f (ISO_Fast_Cursor_Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe30 (ISO_Continuous_Underline) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe31 (ISO_Discontinuous_Underline) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe32 (ISO_Emphasize) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe33 (ISO_Center_Object) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe34 (ISO_Enter) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe35 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe36 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe37 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe38 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe39 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe3a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe3b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe3c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe3d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe3e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe3f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe50 (dead_grave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe51 (dead_acute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe52 (dead_circumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe53 (dead_perispomeni) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe54 (dead_macron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe55 (dead_breve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe56 (dead_abovedot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe57 (dead_diaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe58 (dead_abovering) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe59 (dead_doubleacute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe5a (dead_caron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe5b (dead_cedilla) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe5c (dead_ogonek) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe5d (dead_iota) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe5e (dead_voiced_sound) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe5f (dead_semivoiced_sound) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe60 (dead_belowdot) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe61 (dead_hook) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe62 (dead_horn) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe63 (dead_stroke) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe64 (dead_psili) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe65 (dead_dasia) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe66 (dead_doublegrave) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe67 (dead_belowring) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe68 (dead_belowmacron) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe69 (dead_belowcircumflex) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe6a (dead_belowtilde) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe6b (dead_belowbreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe6c (dead_belowdiaeresis) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe6d (dead_invertedbreve) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe6e (dead_belowcomma) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe6f (dead_currency) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe70 (AccessX_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe71 (AccessX_Feedback_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe72 (RepeatKeys_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe73 (SlowKeys_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe74 (BounceKeys_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe75 (StickyKeys_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe76 (MouseKeys_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe77 (MouseKeys_Accel_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe78 (Overlay1_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe79 (Overlay2_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe7a (AudibleBell_Enable) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe7e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe7f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe80 (dead_a) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe81 (dead_A) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe82 (dead_e) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe83 (dead_E) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe84 (dead_i) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe85 (dead_I) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe86 (dead_o) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe87 (dead_O) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe88 (dead_u) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe89 (dead_U) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe8a (dead_small_schwa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe8b (dead_capital_schwa) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe8d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe91 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe92 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe93 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe94 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe95 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe96 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe97 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe98 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe99 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe9a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe9b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe9c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe9d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe9e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfe9f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfea9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeaa */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeab */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeac */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfead */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeae */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeaf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeb9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfebb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfebc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfebd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfebe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfebf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfec9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeca */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfecb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfecc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfecd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfece */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfecf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed0 (First_Virtual_Screen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed1 (Prev_Virtual_Screen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed2 (Next_Virtual_Screen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed4 (Last_Virtual_Screen) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed5 (Terminate_Server) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfed9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeda */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfedb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfedc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfedd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfede */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfedf */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee0 (Pointer_Left) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee1 (Pointer_Right) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee2 (Pointer_Up) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee3 (Pointer_Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee4 (Pointer_UpLeft) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee5 (Pointer_UpRight) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee6 (Pointer_DownLeft) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee7 (Pointer_DownRight) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee8 (Pointer_Button_Dflt) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfee9 (Pointer_Button1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeea (Pointer_Button2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeeb (Pointer_Button3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeec (Pointer_Button4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeed (Pointer_Button5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeee (Pointer_DblClick_Dflt) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeef (Pointer_DblClick1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef0 (Pointer_DblClick2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef1 (Pointer_DblClick3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef2 (Pointer_DblClick4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef3 (Pointer_DblClick5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef4 (Pointer_Drag_Dflt) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef5 (Pointer_Drag1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef6 (Pointer_Drag2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef7 (Pointer_Drag3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef8 (Pointer_Drag4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfef9 (Pointer_EnableKeys) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfefa (Pointer_Accelerate) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfefb (Pointer_DfltBtnNext) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfefc (Pointer_DfltBtnPrev) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfefd (Pointer_Drag5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfefe */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfeff */
    },
    {                                        /* 0xff?? */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff00 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff01 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff02 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff03 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff04 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff05 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff06 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff07 */
        { .scancode = 0x0E, .flags = 0x00 }, /* 0xff08 (BackSpace) */
        { .scancode = 0x0F, .flags = 0x00 }, /* 0xff09 (Tab) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff0a (Linefeed) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff0b (Clear) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff0c */
        { .scancode = 0x1C, .flags = 0x00 }, /* 0xff0d (Return) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff0e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff0f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff10 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff11 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff12 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff13 (Pause) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff14 (Scroll_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff15 (Sys_Req) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff16 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff17 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff18 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff19 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff1a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff1b (Escape) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff1c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff1d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff1e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff1f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff20 (Multi_key) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff21 (Kanji) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff22 (Muhenkan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff23 (Henkan) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff24 (Romaji) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff25 (Hiragana) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff26 (Katakana) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff27 (Hiragana_Katakana) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff28 (Zenkaku) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff29 (Hankaku) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff2a (Zenkaku_Hankaku) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff2b (Touroku) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff2c (Massyo) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff2d (Kana_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff2e (Kana_Shift) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff2f (Eisu_Shift) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff30 (Eisu_toggle) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff31 (Hangul) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff32 (Hangul_Start) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff33 (Hangul_End) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff34 (Hangul_Hanja) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff35 (Hangul_Jamo) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff36 (Hangul_Romaja) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff37 (Hangul_Codeinput) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff38 (Hangul_Jeonja) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff39 (Hangul_Banja) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff3a (Hangul_PreHanja) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff3b (Hangul_PostHanja) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff3c (Hangul_SingleCandidate) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff3d (Hangul_MultipleCandidate) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff3e (Hangul_PreviousCandidate) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff3f (Hangul_Special) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff40 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff41 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff42 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff43 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff44 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff45 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff46 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff47 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff48 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff49 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff4a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff4b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff4c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff4d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff4e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff4f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff50 (Home) */
        { .scancode = 0x4B, .flags = KBD_FLAGS_EXTENDED }, /* 0xff51 (Left) */
        { .scancode = 0x48, .flags = KBD_FLAGS_EXTENDED }, /* 0xff52 (Up) */
        { .scancode = 0x4D, .flags = KBD_FLAGS_EXTENDED }, /* 0xff53 (Right) */
        { .scancode = 0x50, .flags = KBD_FLAGS_EXTENDED }, /* 0xff54 (Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff55 (Page_Up) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff56 (Page_Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff57 (End) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff58 (Begin) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff59 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff5a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff5b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff5c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff5d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff5e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff5f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff60 (Select) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff61 (Print) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff62 (Execute) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff63 (Insert) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff64 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff65 (Undo) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff66 (Redo) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff67 (Menu) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff68 (Find) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff69 (Cancel) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff6a (Help) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff6b (Break) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff6c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff6d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff6e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff6f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff70 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff71 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff72 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff73 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff74 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff75 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff76 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff77 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff78 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff79 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff7a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff7b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff7c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff7d */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff7e (Hangul_switch) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff7f (Num_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff80 (KP_Space) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff81 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff82 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff83 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff84 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff85 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff86 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff87 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff88 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff89 (KP_Tab) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff8a */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff8b */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff8c */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff8d (KP_Enter) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff8e */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff8f */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff90 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff91 (KP_F1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff92 (KP_F2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff93 (KP_F3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff94 (KP_F4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff95 (KP_Home) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff96 (KP_Left) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff97 (KP_Up) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff98 (KP_Right) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff99 (KP_Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff9a (KP_Page_Up) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff9b (KP_Page_Down) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff9c (KP_End) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff9d (KP_Begin) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff9e (KP_Insert) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xff9f (KP_Delete) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa1 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa2 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa3 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa4 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa5 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa6 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa7 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa8 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffa9 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffaa (KP_Multiply) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffab (KP_Add) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffac (KP_Separator) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffad (KP_Subtract) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffae (KP_Decimal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffaf (KP_Divide) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb0 (KP_0) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb1 (KP_1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb2 (KP_2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb3 (KP_3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb4 (KP_4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb5 (KP_5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb6 (KP_6) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb7 (KP_7) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb8 (KP_8) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffb9 (KP_9) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffba */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffbb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffbc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffbd (KP_Equal) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffbe (F1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffbf (F2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc0 (F3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc1 (F4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc2 (F5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc3 (F6) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc4 (F7) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc5 (F8) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc6 (F9) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc7 (F10) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc8 (L1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffc9 (L2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffca (L3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffcb (L4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffcc (L5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffcd (L6) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffce (L7) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffcf (L8) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd0 (L9) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd1 (L10) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd2 (R1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd3 (R2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd4 (R3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd5 (R4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd6 (R5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd7 (R6) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd8 (R7) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffd9 (R8) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffda (R9) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffdb (R10) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffdc (R11) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffdd (R12) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffde (R13) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffdf (R14) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffe0 (R15) */
        { .scancode = 0x2A, .flags = 0x00 }, /* 0xffe1 (Shift_L) */
        { .scancode = 0x36, .flags = 0x00 }, /* 0xffe2 (Shift_R) */
        { .scancode = 0x1D, .flags = 0x00 }, /* 0xffe3 (Control_L) */
        { .scancode = 0x1D, .flags = 0x00 }, /* 0xffe4 (Control_R) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffe5 (Caps_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffe6 (Shift_Lock) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffe7 (Meta_L) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffe8 (Meta_R) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffe9 (Alt_L) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffea (Alt_R) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffeb (Super_L) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffec (Super_R) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffed (Hyper_L) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffee (Hyper_R) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xffef */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff0 */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff1 (braille_dot_1) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff2 (braille_dot_2) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff3 (braille_dot_3) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff4 (braille_dot_4) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff5 (braille_dot_5) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff6 (braille_dot_6) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff7 (braille_dot_7) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff8 (braille_dot_8) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfff9 (braille_dot_9) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfffa (braille_dot_10) */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfffb */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfffc */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfffd */
        { .scancode = 0x00, .flags = 0x00 }, /* 0xfffe */
        { .scancode = 0x53, .flags = KBD_FLAGS_EXTENDED }, /* 0xffff (Delete) */
    },
};

