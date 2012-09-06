
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

#ifndef _GUAC_POOL_H
#define _GUAC_POOL_H

/**
 * Provides functions and structures for maintaining dynamically allocated and freed
 * pools of integers.
 *
 * @file pool.h
 */

typedef struct guac_pool_int guac_pool_int;

typedef struct guac_pool {

    /**
     * The minimum number of integers which must have been returned by guac_pool_next_int
     * before previously-used and freed integers are allowed to be returned.
     */
    int min_size;

    /**
     * The next integer to be released (after no more integers remain in the pool.
     */
    int __next_value;

    /**
     * The first integer in the pool, if any.
     */
    guac_pool_int* __head;

    /**
     * The last integer in the pool, if any.
     */
    guac_pool_int* __tail;

} guac_pool;

/**
 * Represents a single layer within the Guacamole protocol.
 */
struct guac_pool_int {

    /**
     * The integer value of this pool entry.
     */
    int value;

    /**
     * The next available (unused) guac_pool_int in the list of
     * allocated but free'd ints.
     */
    __guac_pool_int* __next;

};

/**
 * Allocates a new guac_pool having the given minimum size.
 *
 * @param size The minimum number of integers which must have been returned by
 *             guac_pool_next_int before freed integers (previously used integers)
 *             are allowed to be returned.
 * @return A new, empty guac_pool, having the given minimum size.
 */
guac_pool* guac_pool_alloc(int size);

/**
 * Frees the given guac_pool.
 *
 * @param pool The guac_pool to free.
 */
void guac_pool_free(guac_pool* pool);

/**
 * Returns the next available integer from the given guac_pool. All integers returned are
 * non-negative, and are returned in sequences, starting from 0.
 *
 * @param pool The guac_pool to retrieve an integer from.
 * @return The next available integer, which may be either an integer not yet returned
 *         by a call to guac_pool_next_int, or an integer which was previosly returned,
 *         but has since been freed.
 */
int guac_pool_next_int(guac_pool* pool);

/**
 * Frees the given integer back into the given guac_pool. The integer given will be
 * available for future calls to guac_pool_next_int.
 *
 * @param pool The guac_pool to free the given integer into.
 * @param value The integer which should be readded to the given pool, such that it can
 *              be received by a future call to guac_pool_next_int.
 */
void guac_pool_free_int(guac_pool* pool, int value);

#endif

