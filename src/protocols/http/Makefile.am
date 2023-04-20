AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libguac-client-http.la

bin_PROGRAMS = cef_process

# Include source files for your protocol
libguac_client_http_la_SOURCES = \
    argv.c \
    client.c \
    http.c \
    resolution.c \
    settings.c \
    user.c

noinst_HEADERS = \
    argv.h \
    client.h \
    http.h \
    resolution.h \
    settings.h \
    user.h \
    cef/cef_client.h \
    cef/cef_life_span_handler.h \
    cef/cef_render_handler.h

libguac_client_http_la_CFLAGS = \
    -Werror -Wall -Iinclude \
    @LIBGUAC_INCLUDE@ \
    -DCEF_PROCESS_PATH=\"$(abs_top_builddir)/src/protocols/http/cef_process\"

libguac_client_http_la_LIBADD = \
    @COMMON_LTLIB@ \
    @LIBGUAC_LTLIB@ 

libguac_client_http_la_LDFLAGS = \
    -version-info 0:0:0 \
    @PTHREAD_LIBS@

cef_process_SOURCES = \
    cef/cef_client.c \
    cef/cef_life_span_handler.c \
    cef/cef_main.c \
    cef/cef_render_handler.c

cef_process_CFLAGS = \
    -Werror -Wall -Iinclude \
    -DCEF_RESOURCE_PATH=\"@_CEF_RESOURCE_PATH@\" \
    -DCEF_LOCALES_PATH=\"@_CEF_LOCALES_PATH@\" \
    -DCEF_PROCESS_PATH=\"$(abs_top_builddir)/src/protocols/http/cef_process\" \
    @LIBGUAC_INCLUDE@ \
    @CEF_INCLUDE@

cef_process_LDADD = \
    @COMMON_LTLIB@ \
    @LIBGUAC_LTLIB@ \
    @CEF_LIBS@

cef_process_LDFLAGS = \
    @PTHREAD_LIBS@ \
    -Wl,-rpath,@CEF_RELEASE@
