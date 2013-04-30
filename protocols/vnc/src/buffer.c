
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
 * The Original Code is buffer.
 *
 * The Initial Developers of the Original Code are
 *   Craig Hokanson <craig.hokanson@sv.cmu.edu>
 *   Sion Chaudhuri <sion.chaudhuri@sv.cmu.edu>
 *   Gio Perez <gio.perez@sv.cmu.edu>
 *
 * Portions created by the Initial Developer are Copyright (C) 2013
 * the Initial Developers. All Rights Reserved.
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
 
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "queue.h"
 
void buffer_init(buffer* buf, int size_of_data) {
    
    queue_init(&(buf->data_queue), size_of_data);
    
}

void buffer_free(buffer* buf) {
    
    queue_free(&(buf->data_queue));
    
}

void buffer_close(buffer* buf) {
    
    pthread_cond_signal(&(buf->cond));
    
}

void buffer_insert(buffer* buf, void* data, int size_of_data) {
    
    pthread_mutex_lock(&(buf->update_lock));
    enqueue(&(buf->data_queue), data, size_of_data);
    pthread_mutex_unlock(&(buf->update_lock));
    pthread_cond_signal(&(buf->cond));
    
}

void buffer_remove(buffer* buf, void* data, int size_of_data, guac_client* client) {

    pthread_mutex_lock(&(buf->update_lock));
    if((buf->data_queue).count <= 0) 
        pthread_cond_wait(&(buf->cond), &(buf->update_lock));
    
    /* If the thread was signaled during a close, 
    the queue might still be empty */
    if((buf->data_queue).count > 0)
        dequeue(&(buf->data_queue), data, size_of_data);
    
    pthread_mutex_unlock(&(buf->update_lock));

}
