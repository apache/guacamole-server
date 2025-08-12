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

#include "terminal/selection-point.h"

#include <stdbool.h>

bool guac_terminal_selection_point_is_after(
    const guac_terminal_selection_point *a,
    const guac_terminal_selection_point *b) {

    if (a->row == b->row) {
        if (a->column == b->column) {
            if (a->side == b->side) {
                /* If two points are the same, neither is after the other */
                return false;
            }
            /* If points differ by side, a is after b if it's on the right */
            return a->side == GUAC_TERMINAL_COLUMN_SIDE_RIGHT;

        }
        /* If points differ by column, a is after b if it's column is larger */
        return a->column > b->column;

    }
    /* If points differ by row, a is after b if it's row is larger */
    return a->row > b->row;

}

bool guac_terminal_selection_points_enclose_text(
    const guac_terminal_selection_point *start,
    const guac_terminal_selection_point *end) {
    
    /* Different rows will always contain a character */
    if (start->row != end->row) {
        return true;
    }

    /* First check if the starting point completely contains a character */
    int start_char_end = start->char_starting_column + start->char_width - 1;
    if ((start->side == GUAC_TERMINAL_COLUMN_SIDE_LEFT &&
        start->column == start->char_starting_column) &&
        ((end->column > start_char_end) ||
         (end->column == start_char_end && end->side == GUAC_TERMINAL_COLUMN_SIDE_RIGHT)))
        return true;

    /* Otherwise check the next character after start */
    int end_char_end = end->char_starting_column + end->char_width - 1;
    int second_char_start = start_char_end + 1;
    if (second_char_start < end->char_starting_column ||
        (second_char_start == end->char_starting_column &&
        end->column == end_char_end && end->side == GUAC_TERMINAL_COLUMN_SIDE_RIGHT))
        return true;

    return false;

}

int guac_terminal_selection_point_round_up(
    const guac_terminal_selection_point *point) {

    if (point->column == point->char_starting_column &&
        point->side == GUAC_TERMINAL_COLUMN_SIDE_LEFT)
        return point->column;
    
    return point->char_starting_column + point->char_width;
}

int guac_terminal_selection_point_round_down(
    const guac_terminal_selection_point *point) {

    int end_char_column = point->char_starting_column + point->char_width - 1;
    if (point->column == end_char_column &&
        point->side == GUAC_TERMINAL_COLUMN_SIDE_RIGHT)
        return end_char_column;
    
    return point->char_starting_column - 1;
}
