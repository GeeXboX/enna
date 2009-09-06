## Process this file with automake to produce Makefile.in

INCLUDES = \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)\"


AM_CFLAGS = @ENNA_CFLAGS@ @LIBXML2_CFLAGS@ @LIBCURL_CFLAGS@ @LIBCDDB_CFLAGS@ @LIBEXIF_CFLAGS@ @VALHALLA_CFLAGS@

if BUILD_LIBSVDRP
AM_CFLAGS += @LIBSVDRP_CFLAGS@
endif

bin_PROGRAMS = enna enna_thumb

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
browser.c\
content.c\
view_list.c\
view_wall.c\
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
reflection.c\
events_stack.c\
view_cover.c\
popup.c\
exit.c\
thumb.c\
ipc.c



enna_LDADD = @ENNA_LIBS@ @LIBXML2_LIBS@ @LIBCURL_LIBS@ @LIBCDDB_LIBS@ @LIBEXIF_LIBS@ @VALHALLA_LIBS@
enna_LDFLAGS = -rdynamic

if BUILD_LIBSVDRP
enna_LDADD += @LIBSVDRP_LIBS@
endif

enna_thumb_SOURCES = \
enna_thumb_main.c\
utils.c\
image.c

enna_thumb_LDADD = @ENNA_LIBS@

if BUILD_LIBSVDRP
enna_thumb_LDADD += @LIBSVDRP_LIBS@
endif


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
browser.h\
content.h\
view_list.h\
view_wall.h\
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
reflection.h\
events_stack.h\
view_cover.h\
popup.h\
exit.h\
gettext.h\
thumb.h\
ipc.h

