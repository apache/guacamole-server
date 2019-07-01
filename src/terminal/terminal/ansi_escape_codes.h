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


#ifndef GUAC_TERMINAL_ANSI_ESCAPE_CODES_H
#define GUAC_TERMINAL_ANSI_ESCAPE_CODES_H


/**
 * The maximum number of parameters the CSI sequence can have.
 */
#define GUAC_TERMINAL_CSI_MAX_ARGUMENTS 16

/**
 * This action inserts blank characters in the terminal. The
 * number of the blank characters is defined by the users.
 * If no parameter is provided, the function takes 0 by default
 * and adds no blank lines.
 */
#define GUAC_TERMINAL_CSI_BLANK_CHARS '@'

/**
 * This action moves the cursor up a certain number of rows as 
 * indicated by the user. If the number of rows is not provided,
 * then 0 is taken as a parameter.
 */
#define GUAC_TERMINAL_CSI_CURSOR_UP 'A'

/**
 * This action moves the cursor down a certain number of rows as 
 * indicated by the user. If the number of rows is not provided,
 * then 0 is taken as a parameter.
 */
#define GUAC_TERMINAL_CSI_CURSOR_DOWN 'B'

/**
 * This action moves the cursor down a certain number of rows as 
 * indicated by the user. If the number of rows is not provided,
 * then 0 is taken as a parameter.
 */
#define GUAC_TERMINAL_CSI_ROW_DOWN 'e'

/**
 * This CSI action moves the cursor right by an indicated number of
 * columns from the current cursor position. If no number is passed 
 * with this action, 0 is taken as parameter and the cursor is not 
 * moved right.
 */
#define GUAC_TERMINAL_CSI_CURSOR_RIGHT 'C'

/**
 * This CSI action moves the cursor right by an indicated number of
 * columns from the current cursor position. If no number is passed 
 * with this action, 0 is taken as parameter and the cursor is not 
 * moved right.
 */
#define GUAC_TERMINAL_CSI_COLUMN_RIGHT 'a'

/**
 * This CSI action moves the cursor left by an indicated number of 
 * columns from the current position of the cursor. Like any other 
 * CSI sequence, if no parameter is provided, then 0 is taken as a 
 * default parameter.
 */
#define GUAC_TERMINAL_CSI_CURSOR_LEFT 'D'

/**
 * This action is used to move the cursor down by an indicated number
 * of rows. Once the cursor is moved down, it is also moved to the
 * first column of that row. If no number of row is identified, 
 * then 0 is taken as a parameter.
 */
#define GUAC_TERMINAL_CSI_CURSOR_DOWN_COL_ONE 'E'

/**
 * This action is used to move the cursor up by an indicated number of
 * rows. Once the cursor is moved up by the defined amount, it is also
 * moved to the first column of that row. If no number of row is passed
 * by the user, then 0 is taken as a parameter.
 */
#define GUAC_TERMINAL_CSI_CURSOR_UP_COL_ONE 'F'

/**
 * This action is used to move the cursor to a specified column within the 
 * same row. If no parameters are provided to this action, the cursor 
 * is moved to the first column.
 */
#define GUAC_TERMINAL_CSI_MOVE_TO_COLUMN 'G'

/**
 * This action is used to move the cursor to a specified column within 
 * the same row. If no parameters are provided to this action, the cursor
 * is moved to the first column.
 */
#define GUAC_TERMINAL_CSI_MOVE_HORIZONTAL '`'

/**
 * This CSI action is used to change the position of the cursor to 
 * the indicated row and column. For that reason, two parameters
 * need to provided to change both row and column. If no parameter are 
 * provided, 0 is taken as parameter by the terminal.
 */
#define GUAC_TERMINAL_CSI_MOVE_CURSOR 'H'

/**
 * This CSI action is used to change the position of the cursor 
 * to the indicated row and column. For that reason, two parameters
 * need to be provided to change both row and column. If no parameter are 
 * provided, 0 is taken as parameter by the terminal.
 */
#define GUAC_TERMINAL_CSI_MOVE_CURSOR_ANYWHERE 'f'

/**
 * This represents the CSI "erase display" function. Various parameters
 * can cause different outcomes. If no paramater is passed, 0 is taken as
 * the default paramater and the screen is cleared from the current cursor 
 * position to the end of display.
 */
#define GUAC_TERMINAL_CSI_ERASE_DISPLAY 'J'

/**
 * When this paramater is passed to the CSI "erase display" function,
 * the display is cleared from the start to the current cursor position.
 * The absense of this parameter will result in 0 being passed which
 * erases display from the current cursor position to the end of display.
 */
#define GUAC_TERMINAL_ERASE_DISPLAY_UPTO 1

/**
 * When the parameter '2' is passed to the CSI "erase display" function, 
 * it erases the entire display. In case where no parameter is passed 
 * where only the lines proceeding the cursor to the end of display are 
 * deleted. 
 */
#define GUAC_TERMINAL_ERASE_DISPLAY 2

/**
 * When this parameter is provided to the CSI "escape display" sequence
 * the entire display is cleared including the scroll-back buffer. The 
 * absense of this parameter will result in 0 being passed which erases 
 * display from the current cursor position to the end of display.
 */
#define GUAC_TERMINAL_ERASE_SCROLLBACK 3

/**
 * This is the CSI erase line function. Various parameters passed to
 * this function can cause various outcomes. If no parameter is passed to
 * the function, 0 is taken by default and the line is erased from the
 * cursor position to the end of line. 
 */
#define GUAC_TERMINAL_CSI_ERASE_LINE 'K'

/**
 * When the parameter '1' is passed to the CSI "erase line" function,
 * the line is cleared from the start of the line to the position of
 * the cursor. If no paramater is passed, the 0 is taken as default and the 
 * line is erased from the cursor to the end of line.
 */
#define GUAC_TERMINAL_ERASE_LINE_UPTO 1

/**
 * When the parameter '2' is passed to the CSI "erase line" function,
 * the entire line is cleared. If no paramater is passed, the parameter
 * 0 is taken by default and the line is erased from the cursor to the
 * end of line.
 */
#define GUAC_TERMINAL_ERASE_WHOLE_LINE 2

/**
 * This CSI function is used to insert as many number of lines as indicated 
 * by the user. If no parameter is passed, 0 is taken as parameter and no 
 * lines are inserted.
 */
#define GUAC_TERMINAL_CSI_INSERT_LINES 'L'

/**
 * This CSI function is used to delete as many number of lines as indicated 
 * by the user. If no parameter is passed, 0 is taken as parameter and no 
 * line is deleted.
 */
#define GUAC_TERMINAL_CSI_DELETE_LINES 'M'

/**
 * This CSI function deletes a certain amount of characters on the current
 * line. The number of characters to be deleted is determined  by the parameter 
 * that is passed in by the user. If nothing is  passed by the user, 0 is 
 * taken as a default parameter and nothing gets deleted.
 *
 * The delete function simply removes the characters and shifts the characters 
 * on right to take their place.
 */
#define GUAC_TERMINAL_CSI_DELETE_CHARS 'P'

/**
 * This CSI function erases a certain amount of characters on the line. The 
 * number of characters to be erased is determined by the parameter that 
 * is passed in by the user. If nothing is passed, 0 is takes as parameter
 * and nothing gets erased.
 *
 * Using the erase function removes the characters and replaces them
 * with blank characters avoiding shift of the characters on the right.
 */
#define GUAC_TERMINAL_CSI_ERASE_CHARS 'X'

/**
 * This CSI function prints out the video terminal name, which in this 
 * case is VT102. It therefore responds with "I am a VT102".
 */
#define GUAC_TERMINAL_CSI_DISPLAY_ANSWER 'c'

/**
 * This CSI function is native to the Linux console driver.
 */
#define GUAC_TERMINAL_LINUX_PRIVATE_CSI ']'

/**
 * This CSI function allows the user to change the cursor position to
 * a different row of the same column. If no parameter is passed, 0 is 
 * taken as a default parameter.
 */
#define GUAC_TERMINAL_CSI_CHANGE_ROW 'd'

/**
 * This CSI action allows the user to clear tab stops. A tabstop is a 
 * location where a cursor stops after the tabkey is pressed.
 *
 * If this function is not passed in a parameter, 0 is taken in
 * as default which only deletes the tabstop at the current position.
 */
#define GUAC_TERMINAL_CSI_CLEAR_TABSTOP 'g'

/**
 * When the parameter '3' is passed to the "clear tabstops" CSI function, 
 * all tabstops are cleared.
 *
 * The absence of parameter leads to 0 being passed in as a parameter which
 * only deletes the tab stop at the current position.
 */
#define GUAC_TERMINAL_CLEAR_ALL_TABSTOPS 3

/**
 * This function is used to set mode of the terminal. Modes
 * include Keyboard Action Mode, Insert Mode, Send/Recieve Mode
 * and some others.
 */
#define GUAC_TERMINAL_CSI_SET_MODE 'h'

/**
 * This function is used to reset the mode of the terminal. Modes
 * include Keyboard Action Mode, Insert Mode, Send/Recieve Mode
 * and some others.
 */
#define GUAC_TERMINAL_CSI_RESET_MODE 'l'

/**
 * This function is used to set graphic attributes of the terminal.
 * With various parameters, different aspects of the graphics can
 * be modified. If a parameter is not passed, 0 is taken in as a 
 * parameter and all attributes are set to default.
 */
#define GUAC_TERMINAL_CSI_GRAPHICS_RENDITION 'm'

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' bolds the terminal font.
 */
#define GUAC_TERMINAL_CSI_BOLD_TEXT 1

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' renders the graphics to decrease the 
 * intensity of the font(dimmer text) in the terminal.
 */
#define GUAC_TERMINAL_FAINT 2

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' adds an underline to the text in the terminal.
 */
#define GUAC_TERMINAL_UNDERLINE_ON 4

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' swaps foreground and background colors.
 */
#define GUAC_TERMINAL_REVERSE_VIDEO 7

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' adds double underline to the terminal text. 
 */
#define GUAC_TERMINAL_DOUBLY_UNDERLINED 21

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' renders terminal text to be neither bold 
 * nor faint.
 */
#define GUAC_TERMINAL_NORMAL_INTENSITY 22

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' turns off any kind of underline whether
 * it is singly or doubly underline currently in effect. 
 */
#define GUAC_TERMINAL_UNDERLINE_OFF 24

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' reswaps the foreground and background colors 
 * to their original settings defined by the user.
 *
 * Note: This has no effect if the reverse is not turned on 
 * to begin with.
 */
#define GUAC_TERMINAL_REVERSE_VIDEO_OFF 27

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' sets the foreground color to black which is a 
 * lower bound of the available colors.
 */
#define GUAC_TERMINAL_BLACK_FOREGROUND 30

/**
 * This parameter when passed to the CSI graphics rendition function 
 * 'm' sets the foreground color to white which is an upper bound of 
 * the available colors. 
 */
#define GUAC_TERMINAL_WHITE_FOREGROUND 37

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' sets a default foreground color and turns on
 * the underscore as well. It takes in rgb values in the range
 * of 0-255. The mapped color is then set to default.
 */
#define GUAC_TERMINAL_DEF_FOREGROUND_UNDERSCORE_ON 38

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' reverts the terminal to the default foreground 
 * color and also turns the underscore off. 
 */
#define GUAC_TERMINAL_DEF_FOREGROUND_UNDERSCORE_OFF 39

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' sets the foreground color to grey which is a 
 * lower bound of the available bright colors.
 *
 * In addition to the regular foreground colors, bright colors 
 * can also be used.
 */
#define GUAC_TERMINAL_BRIGHT_FOREGROUND_LOW 90

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' sets the foreground color to bright white which 
 * is an upper bound of the available bright colors.
 *
 * In addition to the regular foreground colors, bright colors   
 * can also be used.
 */
#define GUAC_TERMINAL_BRIGHT_FOREGROUND_HIGH 97

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' sets the background color to black which is a 
 * lower bound of the available colors.
 */
#define GUAC_TERMINAL_BLACK_BACKGROUND 40

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' sets the background color to white which is 
 * an upper bound of the available colors.
 */
#define GUAC_TERMINAL_WHITE_BACKGROUND 47

/**
 * This parameter when passed to the CSI graphics rendition  
 * function 'm' sets a default background color. Three additional 
 * parameters are taken in which are the rgb colors that range from 
 * 0-255. Once the color is mapped, the underscore is turned off as well.
 */
#define GUAC_TERMINAL_SET_BACKGROUND 48

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' reverts to the default background color. 
 */
#define GUAC_TERMINAL_DEFAULT_BACKGROUND 49

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' sets the background color to grey  which is a
 * lower bound of the available bright colors.
 *
 * In addition to the regular background colors, bright colors
 * can also be used.
 */
#define GUAC_TERMINAL_BRIGHT_BACKGROUND_LOW 100

/**
 * This parameter when passed to the CSI graphics rendition
 * function 'm' sets the background color to bright white which 
 * is an upper bound of the available bright colors.
 *
 * In addition to the regular background colors, bright colors   
 * can also be used. 
 */
#define GUAC_TERMINAL_BRIGHT_BACKGROUND_HIGH 107

/**
 * This CSI function is used to run the status report commands.
 * It can either get the device status report or the cursur status
 * report, depending on the parameter that is passed to this function.
 */
#define GUAC_TERMINAL_CSI_STATUS_REPORT 'n'

/**
 * When this parameter is passed to the CSI status report function,
 * it gets the device status report. The device report is usually an 'n'
 * which represents that the terminal is ok.
 */
#define GUAC_TERMINAL_DEVICE_STATUS_REPORT 5

/**
 * When this parameter is passed to the CSI status report function, the 
 * cursor position report (CPR) is requested. The position is defined by 
 * the row and column the cursor is situated and is returned in the form 
 * row;columnR where row and column are integers.
 */
#define GUAC_TERMINAL_CURSOR_POSITION_REPORT 6

/**
 * The CSI keyboard LED function can be used to set and clear various LEDs
 * including Num Lock and Caps Lock. It takes in a parameter which targets
 * a particular LED. If no parameters are passed, it takes 0 as default
 * parameter and clears all LEDs.
 *
 * This functionality is ignored in the guac terminal.
 */
#define GUAC_TERMINAL_CSI_KEYBOARD_LED 'q'

/**
 * This CSI function sets the scrolling region of the terminal.
 * It takes in two parameters(top and bottom row). If no parameters
 * are passed, 0 is taken as a default parameter and it reverts the 
 * scrolling region to fit the entire window.
 */
#define GUAC_TERMINAL_CSI_SET_SCROLLING_REGION 'r'

/**
 * This constant represents the number of parameters that are taken
 * by the set scrolling region function of the CSI. Since it
 * takes in the top row and a bottom row, the number of parameters
 * are 2. 
 */
#define GUAC_TERMINAL_SCROLLING_REGION_PARAMS 2

/**
 * This CSI function saves the cursor position. The position is
 * denoted by the cursor's current row and column.
 */
#define GUAC_TERMINAL_SAVE_CURSOR_LOCATION 's'

/**
 * This CSI functions moves the cursor to the previously saved cursor 
 * position.
 */
#define GUAC_TERMINAL_RESTORE_CURSOR_LOCATION 'u'

/**
 * When this parameter is passed to the OSC control sequence,
 * the window title can be set to the text passed in by the user. 
 */
#define GUAC_TERMINAL_SET_WINDOW_TITLE 2

/**
 * When this parameter is passed to the OSC control sequence,
 * the user can assign a number to a given color.
 */
#define GUAC_TERMINAL_SET_COLOR 4

/**
 * Max color palette value since there are a predefined set fo 256
 * colors ranging from 0-255.
 */
#define GUAC_TERMINAL_MAX_COLOR_RANGE 255

/**
 * When the parameter is passed to the CSI terminal control function, the
 * entire screen is filled up with E's to carry out the terminal
 * alignment test.
 */
#define GUAC_TERMINAL_ALIGNMENT_TEST '8'

/**
 * When this parameter is passed to the OSC escape sequence, it saves the
 * position of the pointer.
 */
#define GUAC_TERMINAL_OSC_SAVE_CURSOR '7'

/**
 * When this parameter is passed to the OSC escape sequence, it restores
 * to the previously saved position of the pointer.
 */
#define GUAC_TERMINAL_OSC_RESTORE_CURSOR '8'

/**
 * This initiates the sequence defining the G0 character set. The 
 * following character then determines the particular type of mapping.
 */
#define GUAC_TERMINAL_G0_CHARSET '('

/**
 * This initiates the sequence defining the G1 character set. The
 * following character then determines the particular type of mapping.
 */
#define GUAC_TERMINAL_G1_CHARSET ')'

/**
 * This initiates the OSC(Operating System Command) sequence. The
 * characters following this would determine the type of OSC command
 * to be carried out.
 */
#define GUAC_TERMINAL_OSC ']'

/**
 * This initiates the CSI(Control Sequence Introducer). The characters
 * following this would determine the CSI sequence to be carried out.
 */
#define GUAC_TERMINAL_CSI '['

/**
 * This initiates the DECALN control function. If the following character
 * is '8' it fills the entire screen area with E's.
 */
#define GUAC_TERMINAL_DEC '#'

/**
 * This initiates the linefeed function in the terminal.
 */
#define GUAC_TERMINAL_LINEFEED 'D'

/**
 * This initiates the Next Line Function(NEL) in the terminal.
 */
#define GUAC_TERMINAL_NEWLINE 'E'

/**
 * This initiates the Horizontal Tab Set Function(HTS) which can be used 
 * to set a tabstop at the active column.
 */
#define GUAC_TERMINAL_TABSET 'H'

/**
 * This initiates the Reverse Linefeed(RL) function which can be used to 
 * reverse/undo the effects of an existing linefeed.
 */
#define GUAC_TERMINAL_REVERSE_LINEFEED 'M'

/**
 * This initiates the DEC Private Identification(DECID) function which
 * causes the kernel to return a string that claims it is a VT102.
 */
#define GUAC_TERMINAL_PVT_ID 'Z'

/**
 * This initiates the Reset to Initial State(RIS) function which resets
 * the terminal and replaces all set-up features with their saved
 * settings.
 */
#define GUAC_TERMINAL_RESET 'c'

/**
 * This identifier when passed to the sequence defining the G0 or G1 
 * character set selects the default(ISO 8859-1) mapping.
 */
#define GUAC_TERMINAL_DEFAULT_MAPPING 'B'

/**
 * This identifier when passed to the sequence defining the G0 or G1
 * character set selects the VT100 graphics mapping.
 */
#define GUAC_TERMINAL_VT100 '0'

/**
 * This identifier when passed to the sequence defining the G0 or G1
 * character set selects the null mapping.
 */
#define GUAC_TERMINAL_NULL 'U'

/**
 * This identifier when passed to the sequence defining the G0 or G1
 * character set selects user mapping.
 */
#define GUAC_TERMINAL_USER 'K'

/**
 * This identifier initiates the Cursor Keys Mode(DECCKM) in the CSI.
 */
#define GUAC_TERMINAL_PVT_MODE '?'

/**
 * When this parameter is passed to CSI's cursor key's mode, the user
 * can set application sequences.
 */
#define GUAC_TERMINAL_CSI_SET_SEQUENCES 1

/**
 * When this parameter is passed to CSI's set mode, it sets the terminal
 * to insert mode.
 */
#define GUAC_TERMINAL_CSI_INSERT_MODE 4

/**
 * When this parameter is passed to CSI's set mode, it sets an automatic
 * new line. 
 */
#define GUAC_TERMINAL_CSI_AUTOMATIC 20

/**
 * This represents a Guacamole specific console code which initiates download.
 */
#define GUAC_TERMINAL_DOWNLOAD_INIT 482200

/**
 * This represents a Guacamole specific console code which sets upload 
 * directory.
 */ 
#define GUAC_TERMINAL_UPLOAD_DIRECTORY 482201

/**
 * This represents a Guacamole specific console code which redirects output
 * to a specific pipestream.
 */
#define GUAC_TERMINAL_PIPESTREAM_REDIRECT 482202

/**
 * This represents a Guacamole specific console code which redirects output
 * back to the terminal emulator. This also closes the pipestream.
 */
#define GUAC_TERMINAL_TERMINAL_REDIRECT 482203

/**
 * This represents a Guacamole specific console code which resizes the
 * scrollback buffer.
 */
#define GUAC_TERMINAL_RESIZE_SCROLLBACK 482204

#endif 
