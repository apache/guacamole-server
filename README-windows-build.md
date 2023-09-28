# Building guacamole-server for Windows
The `guacamole-server` Windows build relies on compatibility features provided by Cygwin (most notably, a `fork()` implmentation), and therefore _must_ be built using Cygwin tools. Since no Cygwin cross-compilation environment exists, this means that `guacamole-server` can only be built for Windows using the Windows OS. This document describes a build that produces a working `guacd.exe`, as well as shared libraries for every supported protocol.

## Build Specifics
In this example, `guacamole-server` was built under Cygwin, on a Windows Server 2022 x86_64 build node. Dependencies were installed using packages from Cygwin and MSYS2 (and built from source where no package is available, in the case of `libtelnet`).

### Build Steps
1. Install Cygwin (version 2.926 used here)
2. Install MSYS2 (version 20230718 used here)
3. Install Cygwin packages:
    * git
    * make
    * automake
    * autoconf
    * gcc-core
    * libtool
    * pkg-config
    * libuuid-devel
    * libwinpr2-devel
    * libfreerdp2-devel
4. Install MSYS2 packages:
    * autoconf-wrapper
    * automake-wrapper
    * diffutils
    * make
    * mingw-w64-x86_64-gcc
    * mingw-w64-x86_64-cairo
    * mingw-w64-x86_64-pango
    * mingw-w64-x86_64-libwebsockets
    * mingw-w64-x86_64-libvncserver
    * mingw-w64-x86_64-libssh2
    * mingw-w64-x86_64-libtool
    * mingw-w64-x86_64-libmariadbclient
    * mingw-w64-x86_64-postgresql
    * mingw-w64-x86_64-libpng
    * mingw-w64-x86_64-libjpeg
    * mingw-w64-x86_64-dlfcn
    * mingw-w64-x86_64-pkg-config
    * mingw-w64-x86_64-pulseaudio
    * mingw-w64-x86_64-libvorbis
    * mingw-w64-x86_64-libwebp
    * mingw-w64-x86_64-zlib
5. Build `libtelnet` from source using MSYS2 bash shell
    ```
    curl -s -L https://github.com/seanmiddleditch/libtelnet/releases/download/0.23/libtelnet-0.23.tar.gz | tar xz
    cd libtelnet-0.23

    autoreconf -fi
    ./configure --disable-static --disable-util LDFLAGS="-Wl,-no-undefined"

    make LDFLAGS="-no-undefined"
    make install

    # Required for the Cygwin build to understand how to link against this DLL
    ln -s /usr/bin/msys-telnet-2.dll /usr/bin/libtelnet.dll
    ```
6. Build `guacamole-server` from source using Cygwin bash shell
    ```
    # FIXME: Update this to check out master once this PR is ready for merge
    git clone https://github.com/jmuehlner/guacamole-server.git
    cd guacamole-server
    git checkout GUACAMOLE-1841-cygwin-build

    autoreconf -fi
    export LDFLAGS="-L/usr/bin/ -L/usr/lib/ -L/usr/local/lib -L/usr/local/bin -L/cygdrive/c/msys64/mingw64/bin -L/cygdrive/c/msys64/mingw64/lib -L/cygdrive/c/msys64/usr/bin"
    export CFLAGS="-idirafter /cygdrive/c/msys64/mingw64/include/winpr2 -idirafter /usr/include/ -idirafter /cygdrive/c/msys64/mingw64/include -idirafter /cygdrive/c/msys64/mingw64/include/pango-1.0 -idirafter /cygdrive/c/msys64/mingw64/include/cairo -idirafter /cygdrive/c/msys64/mingw64/include/freerdp2 -idirafter /cygdrive/c/msys64/mingw64/include/glib-2.0 -idirafter /cygdrive/c/msys64/mingw64/include/harfbuzz -idirafter /cygdrive/c/msys64/mingw64/lib/glib-2.0/include -idirafter /cygdrive/c/msys64/usr/include"
    export PKG_CONFIG_PATH="/cygdrive/c/msys64/mingw64/lib/pkgconfig/"

    # NOTE: The libfreerdp2-devel package provided by Cygwin is detected as a snapshot version, and must be explicitly allowed
    ./configure --with-cygwin --enable-allow-freerdp-snapshots || cat config.log

    make
    ```

## Closing Notes
The generated `guacd.exe` will run as expected when invoked from the Cygwin bash shell, but when run outside of a Cygwin environment (e.g. using powershell), connections using text-based protocols (ssh, telnet, kubernetes) will fail with `Fontconfig error: Cannot load default config file`.

FIXME: Update this document to explain how to configure fonts under Windows once I figure it out.

In addition, to enable running `guacd.exe` outside of a Cygwin environment, all (non-system) DLLs that it depends on should be copied into the same directory as the executable. These can be found by running `ldd` in the Cygwin bash shell - e.g.
```
ldd /usr/local/sbin/guacd.exe
find /usr -name '*guac*.dll' | xargs ldd
```
