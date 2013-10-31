
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
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * James Muehlner <dagger10k@users.sourceforge.net>
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

#ifndef _SSH_GUAC_CLIENT_H
#define _SSH_GUAC_CLIENT_H

#include <pthread.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "terminal.h"
#include "cursor.h"
#include "sftp.h"

/**
 * SSH-specific client data.
 */
typedef struct ssh_guac_client_data {

    /**
     * The hostname of the SSH server to connect to.
     */
    char hostname[1024];

    /**
     * The port of the SSH server to connect to.
     */
    int port;

    /**
     * The name of the user to login as.
     */
    char username[1024];

    /**
     * The password to give when authenticating.
     */
    char password[1024];

#ifdef ENABLE_SSH_PUBLIC_KEY
    /**
     * The private key, encoded as base64.
     */
    char key_base64[4096];

    /**
     * The password to use to decrypt the given private key.
     */
    char key_passphrase[1024];

    /**
     * The private key to use for authentication, if any.
     */
    ssh_key key;
#endif

    /**
     * The name of the font to use for display rendering.
     */
    char font_name[1024];

    /**
     * The size of the font to use, in points.
     */
    int font_size;

    /**
     * Whether SFTP is enabled.
     */
    bool enable_sftp;

    /**
     * The SSH client thread.
     */
    pthread_t client_thread;

    /**
     * SSH session, used by the SSH client thread.
     */
    ssh_session session;

    /**
     * The distinct SSH session used for SFTP.
     */
    ssh_session sftp_ssh_session;

    /**
     * SFTP session, used for file transfers.
     */
    sftp_session sftp_session;

    /**
     * The path files will be sent to.
     */
    char sftp_upload_path[GUAC_SFTP_MAX_PATH];

    /**
     * SSH terminal channel, used by the SSH client thread.
     */
    ssh_channel term_channel;

    /**
     * The terminal which will render all output from the SSH client.
     */
    guac_terminal* term;
   
    /**
     * The current contents of the clipboard.
     */
    char* clipboard_data;

    /**
     * Whether the alt key is currently being held down.
     */
    int mod_alt;

    /**
     * Whether the control key is currently being held down.
     */
    int mod_ctrl;

    /**
     * Whether the shift key is currently being held down.
     */
    int mod_shift;

    /**
     * The current mouse button state.
     */
    int mouse_mask;

    /**
     * The cached I-bar cursor.
     */
    guac_ssh_cursor* ibar_cursor;

    /**
     * The cached invisible (blank) cursor.
     */
    guac_ssh_cursor* blank_cursor;

    /**
     * The current cursor, used to avoid re-setting the cursor.
     */
    guac_ssh_cursor* current_cursor;

} ssh_guac_client_data;

#endif

