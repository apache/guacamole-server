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
    mingw-w64-ucrt-x86_64-libgcrypt \
    mingw-w64-ucrt-x86_64-libwebsockets \
    mingw-w64-ucrt-x86_64-libwebp \
    mingw-w64-ucrt-x86_64-openssl \
    mingw-w64-ucrt-x86_64-libvorbis \
    mingw-w64-ucrt-x86_64-pulseaudio

ln -s /quasi-msys2/root/ucrt64/bin/libcairo-2.dll      /quasi-msys2/root/ucrt64/bin/libcairo.dll
ln -s /quasi-msys2/root/ucrt64/bin/libjpeg-8.dll       /quasi-msys2/root/ucrt64/bin/libjpeg.dll
ln -s /quasi-msys2/root/ucrt64/bin/libpng16-16.dll     /quasi-msys2/root/ucrt64/bin/libpng.dll
ln -s /quasi-msys2/root/ucrt64/bin/libcrypto-3-x64.dll /quasi-msys2/root/ucrt64/bin/libcrypto.dll
ln -s /quasi-msys2/root/ucrt64/bin/libssl-3-x64.dll    /quasi-msys2/root/ucrt64/bin/libssl.dll
ln -s /quasi-msys2/root/ucrt64/bin/libwebp-7.dll       /quasi-msys2/root/ucrt64/bin/libwebp.dll
ln -s /quasi-msys2/root/ucrt64/bin/libpulse-0.dll      /quasi-msys2/root/ucrt64/bin/libpulse.dll
ln -s /quasi-msys2/root/ucrt64/bin/libvorbis-0.dll     /quasi-msys2/root/ucrt64/bin/libvorbis.dll
ln -s /quasi-msys2/root/ucrt64/bin/libvorbisenc-2.dll  /quasi-msys2/root/ucrt64/bin/libvorbisenc.dll
ln -s /quasi-msys2/root/ucrt64/bin/libogg-0.dll        /quasi-msys2/root/ucrt64/bin/libogg.dll

