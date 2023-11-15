#!/bin/bash
set -e

wget -qO- https://www.cairographics.org/releases/pixman-0.42.2.tar.gz | tar xz
pushd pixman-0.42.2

./configure --host=x86_64-w64-mingw32 --prefix=/usr/x86_64-w64-mingw32 \
    CPPFLAGS="-I/usr/x86_64-w64-mingw32/include" LDFLAGS="-L/usr/x86_64-w64-mingw32/lib/"
make
make install
popd

wget -qO- https://www.cairographics.org/releases/cairo-1.18.0.tar.xz | tar xJ
pushd cairo-1.18.0

meson setup --cross-file /x86_64-w64-mingw32.txt build-mingw
meson compile -C build-mingw
meson install
