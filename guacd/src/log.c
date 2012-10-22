
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
 * The Original Code is guacd.
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

#include <errno.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <guacamole/client.h>
#include <guacamole/error.h>

/* Log prefix, defaulting to "guacd" */
char log_prefix[64] = "guacd";

void vguacd_log_info(const char* format, va_list args) {

    /* Copy log message into buffer */
    char message[2048];
    vsnprintf(message, sizeof(message), format, args);

    /* Log to syslog */
    syslog(LOG_INFO, "%s", message);

    /* Log to STDERR */
    fprintf(stderr, "%s[%i]: INFO:  %s\n", log_prefix, getpid(), message);

}

void vguacd_log_error(const char* format, va_list args) {

    /* Copy log message into buffer */
    char message[2048];
    vsnprintf(message, sizeof(message), format, args);

    /* Log to syslog */
    syslog(LOG_ERR, "%s", message);

    /* Log to STDERR */
    fprintf(stderr, "%s[%i]: ERROR: %s\n", log_prefix, getpid(), message);

}

void guacd_log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vguacd_log_info(format, args);
    va_end(args);
}

void guacd_log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vguacd_log_error(format, args);
    va_end(args);
}

void guacd_client_log_info(guac_client* client, const char* format,
        va_list args) {
    vguacd_log_info(format, args);
}

void guacd_client_log_error(guac_client* client, const char* format,
        va_list args) {
    vguacd_log_error(format, args);
}

void guacd_log_guac_error(const char* message) {

    /* If error message provided, include in log */
    if (guac_error_message != NULL)
        guacd_log_error("%s: %s: %s",
                message,
                guac_status_string(guac_error),
                guac_error_message);

    /* Otherwise just log with standard status string */
    else
        guacd_log_error("%s: %s",
                message,
                guac_status_string(guac_error));

}

void guacd_client_log_guac_error(guac_client* client, const char* message) {

    /* If error message provided, include in log */
    if (guac_error_message != NULL)
        guac_client_log_error(client, "%s: %s: %s",
                message,
                guac_status_string(guac_error),
                guac_error_message);

    /* Otherwise just log with standard status string */
    else
        guac_client_log_error(client, "%s: %s",
                message,
                guac_status_string(guac_error));

}

