## Process this file with automake to produce Makefile.in

INCLUDES = \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)\"


AM_CFLAGS = @ENNA_CFLAGS@ @LIBCDDB_CFLAGS@

if BUILD_LIBSVDRP
AM_CFLAGS += @LIBSVDRP_CFLAGS@
endif

if BUILD_BACKEND_LIBPLAYER
AM_CFLAGS += @LIBPLAYER_CFLAGS@
endif

if BUILD_BACKEND_EMOTION
AM_CFLAGS += @EMOTION_CFLAGS@
endif

bin_PROGRAMS = enna enna_thumb

enna_SOURCES = \
enna.c\
enna_config.c\
utils.c\
buffer.c\
metadata.c\
mainmenu.c\
module.c\
activity.c\
vfs.c\
volumes.c\
image.c\
browser.c\
browser_obj.c\
content.c\
view_list.c\
view_list2.c\
view_wall.c\
mediaplayer_obj.c\
xml_utils.c\
url_utils.c\
logs.c\
parser_cdda.c\
reflection.c\
box.c\
exit.c\
thumb.c\
ipc.c\
weather_api.c\
weather_notification.c\
volume_notification.c\
ini_parser.c\
input.c\
geoip.c\
search.c

if BUILD_BACKEND_EMOTION
enna_SOURCES += mediaplayer_emotion.c
else
enna_SOURCES += mediaplayer.c
endif

enna_LDADD = @ENNA_LIBS@ @LIBCDDB_LIBS@ @LIBPLAYER_LIBS@
enna_LDFLAGS = -rdynamic

if BUILD_BACKEND_LIBPLAYER
enna_LDADD += @LIBPLAYER_LIBS@
endif

if BUILD_BACKEND_EMOTION
enna_LDADD += @EMOTION_LIBS@
endif

if BUILD_LIBSVDRP
enna_LDADD += @LIBSVDRP_LIBS@
endif

if BUILD_LIBXDG_BASEDIR
enna_SOURCES += xdg.c
enna_LDADD += @LIBXDG_BASEDIR_LIBS@
else
enna_SOURCES += xdg-dummy.c
endif

##########################################################################
# For Modules Static linking : BEGIN
##########################################################################

if USE_STATIC_MODULES

if BUILD_LIBXRANDR
AM_CFLAGS += @LIBXRANDR_CFLAGS@
enna_LDADD += @LIBXRANDR_LIBS@
endif

include ../modules/activity/bookstore/config.mak
include ../modules/activity/configuration/config.mak
include ../modules/activity/games/config.mak
include ../modules/activity/music/config.mak
include ../modules/activity/photo/config.mak
include ../modules/activity/tv/config.mak
include ../modules/activity/video/config.mak
include ../modules/activity/weather/config.mak
include ../modules/browser/cdda/config.mak
include ../modules/browser/dvd/config.mak
include ../modules/browser/ipod/config.mak
include ../modules/browser/jamendo/config.mak
include ../modules/browser/localfiles/config.mak
include ../modules/browser/netstreams/config.mak
include ../modules/browser/podcasts/config.mak
include ../modules/browser/shoutcast/config.mak
include ../modules/browser/upnp/config.mak
include ../modules/browser/valhalla/config.mak
include ../modules/input/kbd/config.mak
include ../modules/input/lirc/config.mak
include ../modules/input/wiimote/config.mak
include ../modules/volume/hal/config.mak
include ../modules/volume/mtab/config.mak
include ../modules/volume/udev/config.mak

if BUILD_ACTIVITY_BOOKSTORE
enna_SOURCES += $(SRCS_ACTIVITY_BOOKSTORE)
endif

if BUILD_ACTIVITY_CONFIGURATION
enna_SOURCES += $(SRCS_ACTIVITY_CONFIGURATION)
endif

if BUILD_ACTIVITY_GAMES
enna_SOURCES += $(SRCS_ACTIVITY_GAMES)
endif

if BUILD_ACTIVITY_MUSIC
enna_SOURCES += $(SRCS_ACTIVITY_MUSIC)
endif

if BUILD_ACTIVITY_PHOTO
enna_SOURCES += $(SRCS_ACTIVITY_PHOTO)
endif

if BUILD_ACTIVITY_TV
enna_SOURCES += $(SRCS_ACTIVITY_TV)
endif

if BUILD_ACTIVITY_VIDEO
enna_SOURCES += $(SRCS_ACTIVITY_VIDEO)
endif

if BUILD_ACTIVITY_WEATHER
enna_SOURCES += $(SRCS_ACTIVITY_WEATHER)
endif

if BUILD_BROWSER_CDDA
enna_SOURCES += $(SRCS_BROWSER_CDDA)
endif

if BUILD_BROWSER_DVD
enna_SOURCES += $(SRCS_BROWSER_DVD)
endif

if BUILD_BROWSER_IPOD
enna_SOURCES += $(SRCS_BROWSER_IPOD)
endif

if BUILD_BROWSER_JAMENDO
enna_SOURCES += $(SRCS_BROWSER_JAMENDO)
endif

if BUILD_BROWSER_LOCALFILES
enna_SOURCES += $(SRCS_BROWSER_LOCALFILES)
endif

if BUILD_BROWSER_NETSTREAMS
enna_SOURCES += $(SRCS_BROWSER_NETSTREAMS)
endif

if BUILD_BROWSER_PODCASTS
enna_SOURCES += $(SRCS_BROWSER_PODCASTS)
endif

if BUILD_BROWSER_SHOUTCAST
enna_SOURCES += $(SRCS_BROWSER_SHOUTCAST)
endif

if BUILD_BROWSER_UPNP
enna_SOURCES += $(SRCS_BROWSER_UPNP)
AM_CFLAGS += @GUPNP_CFLAGS@
enna_LDADD += @GUPNP_LIBS@
endif

if BUILD_BROWSER_VALHALLA
enna_SOURCES += $(SRCS_BROWSER_VALHALLA)
endif

if BUILD_INPUT_KBD
enna_SOURCES += $(SRCS_INPUT_KBD)
endif

if BUILD_INPUT_LIRC
enna_SOURCES += $(SRCS_INPUT_LIRC)
enna_LDADD += @LIRC_LIBS@
endif

if BUILD_INPUT_WIIMOTE
enna_SOURCES += $(SRCS_INPUT_WIIMOTE)
AM_CFLAGS += @LIBBLUETOOTH_CFLAGS@
enna_LDADD += @LIBBLUETOOTH_LIBS@ @LIBCWIID_LIBS@
endif

if BUILD_VOLUME_HAL
enna_SOURCES += $(SRCS_VOLUME_HAL)
AM_CFLAGS += @EHAL_CFLAGS@
enna_LDADD += @EHAL_LIBS@
endif

if BUILD_VOLUME_MTAB
enna_SOURCES += $(SRCS_VOLUME_MTAB)
endif

if BUILD_VOLUME_UDEV
enna_SOURCES += $(SRCS_VOLUME_UDEV)
AM_CFLAGS += @UDEV_CFLAGS@
enna_LDADD += @UDEV_LIBS@
endif

endif

##########################################################################
# For Modules Static linking : END
##########################################################################

enna_thumb_SOURCES = \
enna_thumb_main.c\
utils.c\
buffer.c\
image.c

enna_thumb_LDADD = @ENNA_LIBS@

if BUILD_LIBSVDRP
enna_thumb_LDADD += @LIBSVDRP_LIBS@
endif

if BUILD_LIBXDG_BASEDIR
enna_thumb_SOURCES += xdg.c
enna_thumb_LDADD += @LIBXDG_BASEDIR_LIBS@
else
enna_thumb_SOURCES += xdg-dummy.c
endif

EXTRA_DIST= \
enna.h\
enna_config.h\
utils.h\
buffer.h\
metadata.h\
mainmenu.h\
module.h\
vfs.h\
volumes.h\
activity.h\
image.h\
browser.h\
content.h\
view_list.h\
view_list2.h\
view_wall.h\
mediaplayer.h\
mediaplayer_obj.h\
xml_utils.h\
url_utils.h\
logs.h\
parser_cdda.h\
reflection.h\
box.h\
exit.h\
gettext.h\
thumb.h\
ipc.h\
weather_api.h\
weather_notification.h\
volume_notification.h\
ini_parser.h\
input.h\
geoip.h\
xdg.h
