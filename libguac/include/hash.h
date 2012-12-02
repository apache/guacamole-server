
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
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
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

#ifndef _GUAC_HASH_H
#define _GUAC_HASH_H

#include <cairo/cairo.h>

/**
 * Provides functions and structures for producing likely-to-be-unique hash
 * values for images.
 *
 * @file hash.h
 */

/**
 * Produces a 24-bit hash value from all pixels of the given surface. The
 * surface provided must be RGB or ARGB with each pixel stored in 32 bits.
 * The hashing algorithm used is a variant of the cyclic polynomial rolling
 * hash.
 *
 * @param surface The Cairo surface to hash.
 * @return An arbitrary 24-bit unsigned integer value intended to be well
 *         distributed across different images.
 */
unsigned int guac_hash_surface(cairo_surface_t* surface);

/**
 * Given two Cairo surfaces, returns zero if the data contained within each
 * is identical, and a positive or negative value if the value of the first
 * is found to be lexically greater or less than the second respectively.
 *
 * @param a The first Cairo surface to compare.
 * @param b The Cairo surface to compare the first surface against.
 * @return Zero if the data contained within each is identical, and a positive
 *         or negative value if the value of the first is found to be lexically
 *         greater or less than the second respectively.
 */
int guac_surface_cmp(cairo_surface_t* a, cairo_surface_t* b);

#endif

