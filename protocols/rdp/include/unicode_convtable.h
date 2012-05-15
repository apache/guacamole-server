#ifndef _GUAC_UNICODE_CONVTABLE_H
#define _GUAC_UNICODE_CONVTABLE_H

int keysym2uni(int keysym);

/* Keysym->unicode Exceptions tables */
int keysym2uni_base[65536];
int keysym2uni_ext0[4096];
int keysym2uni_ext1[4096];
int keysym2uni_ext2[4096];

/* Fill global tables, if needed (only on first call) */
void init_unicode_tables();

#endif
