
/* ***** BEGIN LICENSE BLOCK *****
 * queue.c
 * 
 * Implementation of a FIFO queue abstract data type.
 * 
 * by: Steven Skiena
 * begun: March 27, 2002
 *
 * Copyright 2003 by Steven S. Skiena; all rights reserved. 
 * 
 * Permission is granted for use in non-commerical applications
 * provided this copyright notice remains intact and unchanged.
 * 
 * Modified for the Guacamole project by:
 *   Craig Hokanson <craig.hokanson@sv.cmu.edu>
 *   Sion Chaudhuri <sion.chaudhuri@sv.cmu.edu>
 *   Gio Perez <gio.perez@sv.cmu.edu>
 * 
 * ***** END LICENSE BLOCK ***** */
 
#include <stdlib.h>
#include <string.h>
#include "queue.h"

void queue_init(queue* q, int size_of_element) {   
    
    int i;
    
    q->first = 0;
    q->last = QUEUESIZE-1;
    q->count = 0;
    
    for(i = 0; i < QUEUESIZE; i++)
        q->items[i] = malloc(size_of_element);
}

void queue_free(queue* q) {     
    
    int i;
    
    for(i = 0; i < QUEUESIZE; i++)
        free(q->items[i]);
}


int enqueue(queue* q, void* data, int size) {
    
    if (q->count >= QUEUESIZE)
        return -1;
        
    q->last = (q->last+1) % QUEUESIZE;
    memcpy(q->items[ q->last ], data, size);   
    q->count = q->count + 1;
    
    return 0;
    
}

int dequeue(queue* q, void* data, int size) {
    
    if (q->count <= 0) 
        return -1;
        
    memcpy(data, q->items[ q->first ], size);  
    q->first = (q->first+1) % QUEUESIZE;
    q->count = q->count - 1;

    return 0;
    
}
