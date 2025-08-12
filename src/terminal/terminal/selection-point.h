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

#ifndef GUAC_TERMINAL_SELECTION_POINT_H
#define GUAC_TERMINAL_SELECTION_POINT_H

/**
 * Function definitions related to selection points in the terminal.
 *
 * @file selection-point.h
 */

#include <stdbool.h>

/**
 * Specific side of a terminal column.
 */
typedef enum guac_terminal_column_side {

    /**
     * Left side of the column.
     */
    GUAC_TERMINAL_COLUMN_SIDE_LEFT,

    /**
     * Right side of the column.
     */
    GUAC_TERMINAL_COLUMN_SIDE_RIGHT

} guac_terminal_column_side;

/**
 * A reference to a specific point within a terminal window,
 * with additional data about the charater pointed to.
 */
typedef struct guac_terminal_selection_point {

    /**
     * The row value of the pointer.
     */
    int row;

    /**
     * The column value of the pointer.
     */
    int column;

    /**
     * The specific side of the column pointed to
     */
    guac_terminal_column_side side;

    /**
     * The starting column of the character pointed to
     */
    int char_starting_column;

    /**
     * The width of the character pointed to
     */
    int char_width;

} guac_terminal_selection_point;

/**
 * Determine if a point comes after another. This uses the right-to-left
 * language convention, so a point is considered coming after if it is
 * further right or down from the original point.
 *
 * @param a
 *     Point being tested.
 *
 * @param b
 *     Point being compared to.
 *     
 * @return
 *     True if point a comes after b, false otherwise.
 */
bool guac_terminal_selection_point_is_after(
    const guac_terminal_selection_point *a,
    const guac_terminal_selection_point *b);

/**
 * Determine if two points enclose at least one character. 
 * Enclosing is defined as a range stretching from the 
 * furthest left to furthest right of any character.
 * 
 * This function assumes that start comes before end, otherwise
 * behavior is undefined.
 *
 * @param start
 *     Starting point of the range.
 *
 * @param end
 *     Ending point of the range.
 *     
 * @return
 *     True if range from start to end contains at least
 *     one complete character, false otherwise.
 */
bool guac_terminal_selection_points_enclose_text(
    const guac_terminal_selection_point *start,
    const guac_terminal_selection_point *end);

/**
 * Produce a rounded column value based on the position 
 * of the point within the character. This means if we are
 * anywhere except the furthest left spot in the character,
 * we will return the starting column for the next character.
 *
 * @param point
 *     Point to round up.
 *
 * @return
 *     Column value for the start of the selection.
 */
int guac_terminal_selection_point_round_up(
    const guac_terminal_selection_point *point);

/**
 * Produce a rounded column value based on the position 
 * of the point within the character. This means if we are
 * anywhere except the furthest right spot in the character,
 * we will return the ending column for the previous character.
 *
 * @param point
 *     Point to round down.
 *
 * @return
 *     Column value for the end of the selection.
 */
int guac_terminal_selection_point_round_down(
    const guac_terminal_selection_point *point);

#endif
