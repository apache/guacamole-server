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


#ifndef GUAC_TERMINAL_SELECT_H
#define GUAC_TERMINAL_SELECT_H

#include "config.h"
#include "terminal.h"

/**
 * Marks the start of text selection at the given row and column. Any existing
 * selection is cleared. This function should only be invoked while the
 * guac_terminal is locked through a call to guac_terminal_lock().
 *
 * @param terminal
 *     The guac_terminal instance associated with the text being selected.
 *
 * @param row
 *     The row number of the character at the start of the text selection,
 *     where the first (top-most) row in the terminal is row 0. Rows within
 *     the scrollback buffer (above the top-most row of the terminal) will be
 *     negative.
 *
 * @param column
 *     The column number of the character at the start of the text selection,
 *     where the first (left-most) column in the terminal is column 0.
 */
void guac_terminal_select_start(guac_terminal* terminal, int row, int column);

/**
 * Updates the end of text selection at the given row and column. This function
 * should only be invoked while the guac_terminal is locked through a call to
 * guac_terminal_lock().
 *
 * @param terminal
 *     The guac_terminal instance associated with the text being selected.
 *
 * @param row
 *     The row number of the character at the current end of the text
 *     selection, where the first (top-most) row in the terminal is row 0. Rows
 *     within the scrollback buffer (above the top-most row of the terminal)
 *     will be negative.
 *
 * @param column
 *     The column number of the character at the current end of the text
 *     selection, where the first (left-most) column in the terminal is
 *     column 0.
 */
void guac_terminal_select_update(guac_terminal* terminal, int row, int column);

/**
 * Ends text selection, removing any highlight and storing the selected
 * character data within the clipboard associated with the given terminal. If
 * more text is selected than can fit within the clipboard, text at the end of
 * the selected area will be dropped as necessary. This function should only be
 * invoked while the guac_terminal is locked through a call to
 * guac_terminal_lock().
 *
 * @param terminal
 *     The guac_terminal instance associated with the text being selected.
 */
void guac_terminal_select_end(guac_terminal* terminal);

#endif

