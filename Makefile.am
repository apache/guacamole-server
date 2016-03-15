#
# Copyright (C) 2015 Glyptodon LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

ACLOCAL_AMFLAGS = -I m4

# Subprojects
DIST_SUBDIRS =           \
    src/libguac          \
    src/common           \
    src/common-ssh       \
    src/terminal         \
    src/guacd            \
    src/protocols/rdp    \
    src/protocols/ssh    \
    src/protocols/telnet \
    src/protocols/vnc    \
    tests

SUBDIRS =       \
    src/libguac \
    src/common  \
    src/guacd   \
    tests

if ENABLE_COMMON_SSH
SUBDIRS += src/common-ssh
endif

if ENABLE_TERMINAL
SUBDIRS += src/terminal
endif

if ENABLE_RDP
SUBDIRS += src/protocols/rdp
endif

if ENABLE_SSH
SUBDIRS += src/protocols/ssh
endif

if ENABLE_TELNET
#SUBDIRS += src/protocols/telnet
endif

if ENABLE_VNC
SUBDIRS += src/protocols/vnc
endif

EXTRA_DIST =     \
    LICENSE      \
    bin/guacctl  \
    doc/Doxyfile

