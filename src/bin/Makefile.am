## Process this file with automake to produce Makefile.in

INCLUDES = \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\""$(datadir)"\"

AM_CFLAGS = @ENNA_CFLAGS@ @LIBXML2_CFLAGS@ @LIBCURL_CFLAGS@ @LIBCDDB_CFLAGS@ @LIBPLAYER_CFLAGS@

bin_PROGRAMS = enna

enna_SOURCES = \
enna.c\
enna_config.c\
utils.c\
buffer.c\
metadata.c\
mainmenu.c\
background.c\
module.c\
activity.c\
vfs.c\
volumes.c\
image.c\
list.c\
browser.c\
content.c\
wall.c\
switcher.c\
location.c\
mediaplayer.c\
event_key.c\
xml_utils.c\
url_utils.c\
logs.c\
parser_cdda.c\
slideshow.c\
mediacontrol.c\
reflection.c

enna_LDADD = @ENNA_LIBS@ @LIBXML2_LIBS@ @LIBCURL_LIBS@ @LIBCDDB_LIBS@ @LIBPLAYER_LIBS@
enna_LDFLAGS = -rdynamic

EXTRA_DIST= \
enna.h\
enna_config.h\
utils.h\
buffer.h\
metadata.h\
mainmenu.h\
background.h\
module.h\
vfs.h\
volumes.h\
activity.h\
image.h\
list.h\
browser.h\
content.h\
wall.h\
switcher.h\
location.h\
mediaplayer.h\
event_key.h\
xml_utils.h\
url_utils.h\
logs.h\
parser_cdda.h\
slideshow.h\
mediacontrol.h\
reflection.h


