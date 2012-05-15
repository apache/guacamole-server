#!/usr/bin/env python
#
# Copyright (C) 2012 Ulteo SAS
# http://www.ulteo.com
# Author Jocelyn DELALANDE <j.delalande@ulteo.com> 2012
#
# Converts a .ini file defining unicode exceptions to a dot_h file
# The dot_h file defines an array of keysim->unicode
#
# Used to extract the keysym<->unicode mapping exceptions from 
# unicode_exception.ini (can be found in Ulteo patched version of xrdp)
#
# Such an ini file can be found at
# http://www.ulteo.com/home/en/download/sourcecode (xrdp folder)
#

import sys
import ConfigParser


class KeysymMaps:
    def __init__(self):
        # 4 digits keysyms
        self.base = []
        # Extented keysym starting with 0x1000
        self.ext0 = []
        # Extented keysym starting with 0x1001
        self.ext1 = []
        # Extented keysym starting with 0x1002
        self.ext2 = []

    def insert(self, keysym, uni):
        # Main keysym table is 4-digit hexa keysyms
        # (most of 'em are 3-digits but they lack the leading zero)
        if len(keysym) <= 6:
            self.base.append((keysym[2:], uni))

        elif keysym.startswith('0x100'):
            if keysym[:6] == '0x1000':
                self.ext0.append((keysym[6:], uni))
            elif keysym[:6] == '0x1001':
                self.ext1.append((keysym[6:], uni))
            elif keysym[:6] == '0x1002':
                self.ext2.append((keysym[6:], uni))

            else:
                raise ValueError("Unexpected keysym : %s" % keysym)

        else:
            raise ValueError("Unexpected keysym : %s" % keysym)

    def get_h_content(self, base_name, ext0_name, ext1_name, ext2_name):
        out = ''
        maps = ((base_name, self.base), 
                (ext0_name, self.ext0), 
                (ext1_name, self.ext1), 
                (ext2_name, self.ext2))

        for var_name, kmap in maps:
            for keysym, uni in kmap:
                out += '%s[0x%s] = %s;\n' %(var_name, keysym, uni)
            out += '\n\n'
        return out


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "ini2dot_h_unimap.py <ini_file>"
        exit(2)

    inifile = sys.argv[1]
    
    print "uni2keysym_map[]"

    ini = ConfigParser.ConfigParser()
    ini.read([inifile])
    mapping = ini.items('unicode_exception')
    
    maps = KeysymMaps()


    for uni, keysym in mapping:
        maps.insert(keysym, uni)

    dot_h_content = \
"""
int keysm2uni_base[4096];
int keysm2uni_ext0[4096];
int keysm2uni_ext1[4096];
int keysm2uni_ext2[4096];


"""
    dot_h_content += maps.get_h_content('keysm2uni_base', 'keysm2uni_ext0', 
                                        'keysm2uni_ext1', 'keysm2uni_ext2');

    print dot_h_content
