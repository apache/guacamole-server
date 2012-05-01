/**
 * Copyright (C) 2012 Ulteo SAS
 * http://www.ulteo.com
 * Author Jocelyn DELALANDE <j.delalande@ulteo.com> 2012
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/


#include "unicode_convtable.h"

int keysym2uni(int keysym) {
	init_unicode_tables();
	/* Default: no exception */
	int exception = 0;

	if (keysym < 0x100000) {
		// Look for a 4-digits-form exception
		exception =  keysym2uni_base[keysym];
	} else {
		// Look for a 7-digits-form exception
		/* Switch to look for 0x1001XXX 0x1002XXX or 0x1002XXX 
		   the tables only indexes on XXX
		 */
		switch(keysym & 0xFFFF000) {
		case 0x1000000:
			exception = keysym2uni_ext0[keysym & 0x0000FFF];
			break;
		case 0x1001000:
			exception = keysym2uni_ext1[keysym & 0x0000FFF];
			break;
		case 0x1002000:
			exception = keysym2uni_ext2[keysym & 0x0000FFF];
			break;
		}

		/* If the keysym is not within exceptions, keysym = unicode */
	}
	if (exception != 0) {
		return exception;
	} else {
		return keysym;
	}
}
