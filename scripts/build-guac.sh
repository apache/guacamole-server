#!/bin/bash
#set -e

cd guacamole-server

export PKG_CONFIG_PATH="/quasi-msys2/root/ucrt64/lib/pkgconfig/"
export LDFLAGS="-L/quasi-msys2/root/ucrt64/bin/"
export CFLAGS="-I/quasi-msys2/root/ucrt64/include/ -I/usr/x86_64-w64-mingw32/include"

autoreconf -fi
./configure --host=x86_64-w64-mingw32 --with-cygwin --with-telnet=no --with-ssh=no --with-rdp=no --with-vnc=no --disable-kubernetes --disable-guacenc --disable-guacd --disable-guaclog

cat config.log

make
make install
