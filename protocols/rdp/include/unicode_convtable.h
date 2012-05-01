
#ifndef _GUAC_UNICODE_CONVTABLE_H
#define _GUAC_UNICODE_CONVTABLE_H

int keysym2uni(int keysym);

/* Keysym->unicode Exceptions tables */
int keysym2uni_base[65536];// = { 0x0 };
int keysym2uni_ext0[4096];// = { 0x0 };
int keysym2uni_ext1[4096];// = { 0x0 };
int keysym2uni_ext2[4096];// = { 0x0 };



/* Fill global tables, if needed (only on first call) */
void init_unicode_tables();

#endif
