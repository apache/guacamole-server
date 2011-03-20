
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

#ifndef _GUAC_LOG_H
#define _GUAC_LOG_H

/**
 * Provides basic cross-platform logging facilities.
 *
 * @file log.h
 */

/**
 * Logs an informational message in the system log, whatever
 * that may be for the system being used. This will currently
 * log to syslog for platforms supporting it, and stderr for
 * all others.
 *
 * @param str A printf-style format string to log.
 * @param ... Arguments to use when filling the format string for printing.
 */
void guac_log_info(const char* str, ...);

/**
 * Logs an error message in the system log, whatever
 * that may be for the system being used. This will currently
 * log to syslog for platforms supporting it, and stderr for
 * all others.
 *
 * @param str A printf-style format string to log.
 * @param ... Arguments to use when filling the format string for printing.
 */
void guac_log_error(const char* str, ...);

#endif
