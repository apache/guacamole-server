
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
 
#ifndef __GUAC_VNC_QUEUE_H
#define __GUAC_VNC_QUEUE_H

#define QUEUESIZE       200

typedef struct {
        void* items[QUEUESIZE+1];		/* body of queue */
        int first;                      /* position of first element */
        int last;                       /* position of last element */
        int count;                      /* number of queue elements */
} queue;

void queue_init(queue* q, int size_of_element);

void queue_free(queue* q);

int enqueue(queue* q, void* data, int size);

int dequeue(queue* q, void* data, int size);

#endif
