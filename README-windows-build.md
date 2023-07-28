# Building guacamole-server for Windows
Certain portions of `guacamole-server` can be built using MinGW on Windows, specifically the libguac libraries. `guacd` itself relies on `fork()` and other functionality that has no equivalent in Windows. Theoretically, Cygwin could provide a compatibility layer for these missing functions, but so far attempts to implement such a build have not resulted in a functional `guacd`.

This document will walk you through a known-working set of steps for building the libguac libraries for a Windows target. The following build steps were tested on a Windows Server 2022 x86_64 build node.

### Build Steps
1. Install MSYS2 (version 20230718 used here)
2. Install MSYS2 packages:
    * autoconf-wrapper
    * automake-wrapper
    * diffutils
    * git
    * libtool
    * libedit-devel
    * make
    * pkg-config
    * wget
    * msys2-runtime-devel
    * mingw-w64-x86_64-gcc
    * mingw-w64-x86_64-libwebsockets
    * mingw-w64-x86_64-libtool
    * mingw-w64-x86_64-dlfcn
    * mingw-w64-x86_64-pkg-config
    * mingw-w64-x86_64-cairo
    * mingw-w64-x86_64-gcc
    * mingw-w64-x86_64-gdb
    * mingw-w64-x86_64-libpng
    * mingw-w64-x86_64-libjpeg-turbo
    * mingw-w64-x86_64-freerdp (version < 3)
    * mingw-w64-x86_64-freetds
    * mingw-w64-x86_64-postgresql
    * mingw-w64-x86_64-libmariadbclient
    * mingw-w64-x86_64-libvncserver
    * mingw-w64-x86_64-dlfcn
    * mingw-w64-x86_64-libgcrypt
    * mingw-w64-x86_64-libgxps
    * mingw-w64-x86_64-libwebsockets
    * mingw-w64-x86_64-libwebp
    * mingw-w64-x86_64-libssh2
    * mingw-w64-x86_64-openssl
    * mingw-w64-x86_64-libvorbis
    * mingw-w64-x86_64-pulseaudio
    * mingw-w64-x86_64-zlib
    *
3. Build `libtelnet` from source using MSYS2 bash shell
    ```
    export PKG_CONFIG_PATH="/mingw64/lib/pkgconfig"
    export PATH="$PATH:/mingw64/bin:/usr/bin"
    curl -s -L https://github.com/seanmiddleditch/libtelnet/releases/download/0.23/libtelnet-0.23.tar.gz | tar xz
    cd libtelnet-0.23
    autoreconf -fi

    ./configure --prefix=/mingw64 --disable-static --disable-util LDFLAGS="-Wl,-no-undefined -L/mingw64/bin/ -L/mingw64/lib" || cat config.log
    cat config.log

    make LDFLAGS="-no-undefined"
    make install
    ```
4. Fix DLL prefixes so that the build can link against them, e.g. using this script
    ```
    #!/bin/bash

    set -e
    set -x

    # Enable fancy pattern matching on filenames (provides wildcards that can match
    # multiple contiguous digits, among others)
    shopt -s extglob

    # Strip architecture suffix
    for LIB in /mingw64/bin/lib*-x64.dll; do
        ln -sfv "$LIB" "$(echo "$LIB" | sed 's/-x64.dll$/.dll/')"
    done

    # Automatically add symlinks that strip the library version suffix
    for LIB in /mingw64/bin/lib*-+([0-9]).dll; do
        ln -sfv "$LIB" "$(echo "$LIB" | sed 's/-[0-9]*\.dll$/.dll/')"
    done

    # Automatically add symlinks that strip the library version suffix
    for LIB in /mingw64/bin/lib*([^0-9.])@([^0-9.-])+([0-9]).dll; do
        ln -sfv "$LIB" "$(echo "$LIB" | sed 's/[0-9]*\.dll$/.dll/')"
    done

    ```
5. Build `guacamole-server` from source using MSYS2 bash shell
    ```
    export PKG_CONFIG_PATH="/mingw64/lib/pkgconfig:/usr/lib/pkgconfig"
    export PATH="$PATH:/mingw64/bin:/usr/bin"

    # FIXME: Update this to check out master once this PR is ready for merge
    git clone https://github.com/jmuehlner/guacamole-server.git
    cd guacamole-server
    git checkout GUACAMOLE-1841-cygwin-build-clean

    autoreconf -fi
    export LDFLAGS="-L/mingw64/bin/ -L/usr/bin/ -L/mingw64/lib -lws2_32"
    export CFLAGS="-isystem/mingw64/include/ \
                    -I/mingw64/include/pango-1.0 \
                    -I/mingw64/include/glib-2.0/ \
                    -I/mingw64/lib/glib-2.0/include/ \
                    -I/mingw64/include/harfbuzz/ \
                    -I/mingw64/include/cairo/ \
                    -I/mingw64/include/freerdp2 \
                    -I/mingw64/include/winpr2 \
                    -Wno-error=expansion-to-defined
                    -Wno-error=attributes"

    ./configure --prefix=/mingw64 --with-windows --disable-guacenc --disable-guacd --disable-guaclog || cat config.log

    make
    make install
    ```

## Closing Notes
FIXME: Update this document to explain how to configure fonts under Windows once I figure it out.
