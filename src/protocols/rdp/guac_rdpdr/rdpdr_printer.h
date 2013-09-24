
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

#ifndef __GUAC_RDPDR_PRINTER_H
#define __GUAC_RDPDR_PRINTER_H

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <freerdp/utils/svc_plugin.h>

/**
 * Data specific to an instance of the printer device.
 */
typedef struct guac_rdpdr_printer_data {

    /**
     * Stream for receiving printed files.
     */
    guac_stream* stream;

    /**
     * File descriptor that should be written to when sending documents to the
     * printer.
     */
    int printer_input;

    /**
     * File descriptor that should be read from when receiving output from the
     * printer.
     */
    int printer_output;

    /**
     * Thread which transfers data from the printer to the Guacamole client.
     */
    pthread_t printer_output_thread;

    /**
     * The number of bytes received in the current print job.
     */
    int bytes_received;

} guac_rdpdr_printer_data;

/**
 * Registers a new printer device within the RDPDR plugin. This must be done
 * before RDPDR connection finishes.
 */
void guac_rdpdr_register_printer(guac_rdpdrPlugin* rdpdr);

/**
 * The command to run when filtering postscript to produce PDF. This must be
 * a NULL-terminated array of arguments, where the first argument is the name
 * of the file to run.
 */
extern char* const guac_rdpdr_pdf_filter_command[];

#endif

