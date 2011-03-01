
AUTOMAKE_OPTIONS = foreign 
initdir = @init_dir@

sbin_PROGRAMS = guacd
init_SCRIPTS = init.d/guacd

guacd_SOURCES = src/daemon.c

EXTRA_DIST = init.d/guacd.in
CLEANFILES = $(init_SCRIPTS)

init.d/guacd: init.d/guacd.in
	sed -e 's,[@]sbindir[@],$(sbindir),g' < init.d/guacd.in > init.d/guacd
	chmod +x init.d/guacd

