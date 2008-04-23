## Process this file with automake to produce Makefile.in

INCLUDES = \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\""$(datadir)"\"

AM_CFLAGS = @ENNA_CFLAGS@

bin_PROGRAMS = enna

enna_SOURCES = \
enna.c\
enna_config.c\
utils.c\
scanner.c\
box.c\
mainmenu.c\
background.c\
module.c\
image.c\
list.c

enna_LDADD = @ENNA_LIBS@
enna_LDFLAGS = -rdynamic

EXTRA_DIST= \
enna.h\
enna_inc.h\
enna_config.h\
util.h\
scanner.h\
box.h\
mainmenu.h\
background.h\
module.h\
image.h\
list.h



