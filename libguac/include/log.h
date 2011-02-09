
/*
 *  Guacamole - Clientless Remote Desktop
 *  Copyright (C) 2010  Michael Jumper
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LOG_H
#define _LOG_H

#ifdef HAVE_SYSLOG_H

    /* Logging for UNIX */
    #include <syslog.h>
    #define GUAC_LOG_ERROR(...) syslog(LOG_ERR,  __VA_ARGS__)
    #define GUAC_LOG_INFO(...)  syslog(LOG_INFO, __VA_ARGS__)

#else

    /* Logging for W32 */
    #define GUAC_LOG_ERROR(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
    #define GUAC_LOG_INFO(...)  fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")

#endif

#endif
