
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
 *   Sion Chaudhuri <sion.chaudhuri@sv.cmu.edu>
 *   Craig Hokanson <craig.hokanson@sv.cmu.edu>
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
#include "queue.h"

void queue_init(queue* q, int queue_size, int element_size) {   
    
    int i;
    
    q->queue_size = queue_size;
    q->element_size = element_size;
    q->head = 0;
    q->tail = 0;
    q->num_elements = 0;
   
    q->elements = malloc(sizeof(void *) * queue_size);
    
    for(i = 0; i < queue_size; i++) {
        q->elements[i] = malloc(element_size);
    }
    
}

void queue_free(queue* q) {
    
    int i;
    
    for(i = 0; i < q->queue_size; i++) {
        free(q->elements[i]);
    }
    
    free(q->elements);
    
}

int queue_enqueue(queue* q, void* data) {
    
    int capacity = q->queue_size - q->num_elements;
    if (capacity <= 0)
        return -1;
    
    q->tail = (q->tail + 1) % q->queue_size;
    memcpy(q->elements[q->tail], data, q->element_size);
    q->num_elements = q->num_elements + 1;
    
    return 0;
    
}

int queue_dequeue(queue* q, void* data) {
    
    if (q->num_elements <= 0)
        return -1;
    
    memcpy(data, q->elements[q->head], q->element_size);
    q->head = (q->head + 1) % q->queue_size;
    q->num_elements = q->num_elements - 1;
    
    return 0;
    
}
