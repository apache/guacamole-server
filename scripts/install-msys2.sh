#!/bin/bash

set -e
set -x

#bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"

git clone https://github.com/holyblackcat/quasi-msys2
pushd quasi-msys2

make install \
    mingw-w64-ucrt-x86_64-cairo \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-gdb \
    mingw-w64-ucrt-x86_64-libpng \
    mingw-w64-ucrt-x86_64-libjpeg-turbo \
    mingw-w64-ucrt-x86_64-freerdp \
    mingw-w64-ucrt-x86_64-libvncserver \
    mingw-w64-ucrt-x86_64-dlfcn \
    mingw-w64-ucrt-x86_64-libgcrypt

ln -s /quasi-msys2/root/ucrt64/bin/libcairo-2.dll /quasi-msys2/root/ucrt64/bin/libcairo.dll
ln -s /quasi-msys2/root/ucrt64/bin/libjpeg-8.dll /quasi-msys2/root/ucrt64/bin/libjpeg.dll
ln -s /quasi-msys2/root/ucrt64/bin/libpng16-16.dll /quasi-msys2/root/ucrt64/bin/libpng.dll
