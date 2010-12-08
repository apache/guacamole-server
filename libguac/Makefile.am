AUTOMAKE_OPTIONS = gnu

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Werror -Wall -Iinclude

libguacincdir = $(includedir)/guacamole
libguacinc_HEADERS = include/client.h include/guacio.h include/protocol.h

lib_LTLIBRARIES = libguac.la

libguac_la_SOURCES = src/client.c src/guacio.c src/protocol.c

libguac_la_LDFLAGS = -version-info 0:0:0

