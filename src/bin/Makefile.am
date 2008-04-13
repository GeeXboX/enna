## Process this file with automake to produce Makefile.in

INCLUDES = \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\""$(datadir)"\"

AM_CFLAGS = @ENNA_CFLAGS@

bin_PROGRAMS = enna

enna_SOURCES = \
enna.c\
enna_config.c\
enna_util.c\
enna_scanner.c\
enna_box.c\
enna_mainmenu.c\
enna_background.c\
enna_module.c

enna_LDADD = @ENNA_LIBS@

EXTRA_DIST= \
enna.h\
enna_inc.h\
enna_config.h\
enna_util.h\
enna_scanner.h\
enna_box.h\
enna_mainmenu.h\
enna_background.h\
enna_module.h



