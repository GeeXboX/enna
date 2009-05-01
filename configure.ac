# get rid of that stupid cache mechanism
rm -f config.cache

AC_INIT([enna],[0.3.0],[www.digital-corner.org])
AC_PREREQ(2.61)
AC_CONFIG_SRCDIR(configure.ac)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AM_INIT_AUTOMAKE(1.6 dist-bzip2)
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CC
AC_DIAGNOSE([obsolete],[AM_PROG_CC_STDC:
	your code should no longer depend upon `am_cv_prog_cc_stdc', but upon
	`ac_cv_prog_cc_stdc'.  Remove this warning and the assignment when
	you adjust the code.  You can also remove the above call to
	AC_PROG_CC if you already called it elsewhere.])
am_cv_prog_cc_stdc=$ac_cv_prog_cc_stdc

AC_HEADER_STDC
AC_C_CONST
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL

define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl

AC_PROG_LIBTOOL


PKG_CHECK_MODULES([ENNA],
  [
   eina-0 >= 0.0.1
   eet >= 1.0.0
   evas >= 0.9.9
   ecore >= 0.9.9.40
   ecore-evas
   ecore-txt
   ecore-file
   embryo >= 0.9.1
   edje >= 0.5.0
   elementary
   libplayer
  ]
)

#########
# Theme #
#########

AC_ARG_ENABLE(theme,
	[AC_HELP_STRING([--disable-theme], [disable theme.
        [[default=enabled]]])],
	[build_theme=$enableval],
	[build_theme="yes"]
)
AM_CONDITIONAL([ENNA_BUILD_THEME], test "x${build_theme}" = "xyes")

####################
# Activity Modules #
####################

# Infos
want_activity_infos="yes"
AC_MSG_CHECKING(whether information activity module is to be built)
AC_ARG_ENABLE(activity-infos,
  AS_HELP_STRING([--enable-activity-infos],[Enable Infos Activity Module]),
  [ want_activity_info=$enableval ]
)
AC_MSG_RESULT($want_activity_info)
AM_CONDITIONAL(BUILD_ACTIVITY_INFOS, test "x$want_activity_infos" = "xyes")
if test "x$want_activity_infos" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_INFOS, 1, [Infos Activity])
fi

# Games
want_activity_games="yes"
AC_MSG_CHECKING(whether games activity module is to be built)
AC_ARG_ENABLE(activity-games,
  AS_HELP_STRING([--enable-activity-games],[Enable Games Activity Module]),
  [ want_activity_games=$enableval ]
)
AC_MSG_RESULT($want_activity_games)
AM_CONDITIONAL(BUILD_ACTIVITY_GAMES, test "x$want_activity_games" = "xyes")
if test "x$want_activity_games" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_GAMES, 1, [Games Activity])
fi

# Music
want_activity_music="yes"
AC_MSG_CHECKING(whether music activity module is to be built)
AC_ARG_ENABLE(activity-music,
  AS_HELP_STRING([--enable-activity-music],[Enable Music Activity Module]),
  [ want_activity_music=$enableval ]
)
AC_MSG_RESULT($want_activity_music)
AM_CONDITIONAL(BUILD_ACTIVITY_MUSIC, test "x$want_activity_music" = "xyes")
if test "x$want_activity_music" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_MUSIC, 1, [Music Activity])
fi

# Photo
want_activity_photo="yes"
AC_MSG_CHECKING(whether photo activity module is to be built)
AC_ARG_ENABLE(activity-photo,
  AS_HELP_STRING([--enable-activity-photo],[Enable Photo Activity Module]),
  [ want_activity_photo=$enableval ]
)
AC_MSG_RESULT($want_activity_photo)
AM_CONDITIONAL(BUILD_ACTIVITY_PHOTO, test "x$want_activity_photo" = "xyes")
if test "x$want_activity_photo" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_PHOTO, 1, [Photo Activity])
fi

# TV
want_activity_tv="yes"
AC_MSG_CHECKING(whether tv activity module is to be built)
AC_ARG_ENABLE(activity-tv,
  AS_HELP_STRING([--enable-activity-tv],[Enable TV Activity Module]),
  [ want_activity_tv=$enableval ]
)
AC_MSG_RESULT($want_activity_tv)
AM_CONDITIONAL(BUILD_ACTIVITY_TV, test "x$want_activity_tv" = "xyes")
if test "x$want_activity_tv" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_TV, 1, [TV  Activity])
fi

# Video
want_activity_video="yes"
AC_MSG_CHECKING(whether video activity module is to be built)
AC_ARG_ENABLE(activity-video,
  AS_HELP_STRING([--enable-activity-video],[Enable Video Activity Module]),
  [ want_activity_video=$enableval ]
)
AC_MSG_RESULT($want_activity_video)
AM_CONDITIONAL(BUILD_ACTIVITY_VIDEO, test "x$want_activity_video" = "xyes")
if test "x$want_activity_video" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_VIDEO, 1, [Video Activity])
fi

# Weather
want_activity_weather="yes"
AC_MSG_CHECKING(whether weather activity module is to be built)
AC_ARG_ENABLE(activity-weather,
  AS_HELP_STRING([--enable-activity-weather],[Enable Weather Activity Module]),
  [ want_activity_weather=$enableval ]
)
AC_MSG_RESULT($want_activity_weather)
AM_CONDITIONAL(BUILD_ACTIVITY_WEATHER, test "x$want_activity_weather" = "xyes")
if test "x$want_activity_weather" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_WEATHER, 1, [Weather Activity])
fi

###################
# Browser Modules #
###################

# Audio CD (CDDA)
want_browser_cdda="yes"
AC_MSG_CHECKING(whether cdda browser module is to be built)
AC_ARG_ENABLE(browser-cdda,
  AS_HELP_STRING([--enable-browser-cdda],[Enable CDDA Browser Module]),
  [ want_browser_cdda=$enableval ]
)
AC_MSG_RESULT($want_browser_cdda)
AM_CONDITIONAL(BUILD_BROWSER_CDDA, test "x$want_browser_cdda" = "xyes")
if test "x$want_browser_cdda" = "xyes"; then
  AC_DEFINE(BUILD_BROWSER_CDDA, 1, [CDDA Browser])
fi

# DVD Video
want_browser_dvd="yes"
AC_MSG_CHECKING(whether dvd video browser module is to be built)
AC_ARG_ENABLE(browser-dvd,
  AS_HELP_STRING([--enable-browser-dvd],[Enable DVD Video Browser Module]),
  [ want_browser_dvd=$enableval ]
)
AC_MSG_RESULT($want_browser_dvd)
AM_CONDITIONAL(BUILD_BROWSER_DVD, test "x$want_browser_dvd" = "xyes")
if test "x$want_browser_dvd" = "xyes"; then
  AC_DEFINE(BUILD_BROWSER_DVD, 1, [Dvd Video Browser])
fi

# Light Media Scanner (LMS)
want_browser_lms="auto"
have_browser_lms="no"
AC_MSG_CHECKING(whether lms browser is to be built)
AC_ARG_ENABLE(browser-lms,
  AS_HELP_STRING([--enable-browser-lms],[Enable Light Media Scanner Browser Module]),
  [ want_browser_lms=$enableval ]
)
AC_MSG_RESULT($want_browser_lms)
if test "x$want_browser_lms" = "xyes" -o "x$want_browser_lms" = "xauto"; then
  PKG_CHECK_MODULES(
    [LMS],
    [lightmediascanner],
    [
      AC_DEFINE(BUILD_BROWSER_LMS, 1, [Light media scanner Module])
      have_browser_lms="yes"
    ],
    [
      if test "x$want_browser_lms" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([Light media scanner not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_BROWSER_LMS, test "x$have_browser_lms" = "xyes")

# Local Files
want_browser_localfiles="yes"
AC_MSG_CHECKING(whether localfiles browser module is to be built)
AC_ARG_ENABLE(browser-localfiles,
  AS_HELP_STRING([--enable-browser-localfiles],[Enable Local Files Browser Module]),
  [ want_browser_localfiles=$enableval ]
)
AC_MSG_RESULT($want_browser_localfiles)
AM_CONDITIONAL(BUILD_BROWSER_LOCALFILES, test "x$want_browser_localfiles" = "xyes")
if test "x$want_browser_localfiles" = "xyes"; then
  AC_DEFINE(BUILD_BROWSER_LOCALFILES, 1, [Localfiles Browser])
fi

# NetStreams
want_browser_netstreams="yes"
AC_MSG_CHECKING(whether netstreams browser module is to be built)
AC_ARG_ENABLE(browser-netstreams,
  AS_HELP_STRING([--enable-browser-netstreams],[Enable NetStreams Browser Module]),
  [ want_browser_netstreams=$enableval ]
)
AC_MSG_RESULT($want_browser_netstreams)
AM_CONDITIONAL(BUILD_BROWSER_NETSTREAMS, test "x$want_browser_netstreams" = "xyes")
if test "x$want_browser_netstreams" = "xyes"; then
  AC_DEFINE(BUILD_BROWSER_NETSTREAMS, 1, [NetStreams Browser])
fi

# Podcast
want_browser_podcasts="yes"
AC_MSG_CHECKING(whether podcasts browser module is to be built)
AC_ARG_ENABLE(browser-podcasts,
  AS_HELP_STRING([--enable-browser-podcasts],[Enable Podcasts Browser Module]),
  [ want_browser_podcasts=$enableval ]
)
AC_MSG_RESULT($want_browser_podcasts)
AM_CONDITIONAL(BUILD_BROWSER_PODCASTS, test "x$want_browser_podcasts" = "xyes")
if test "x$want_browser_podcasts" = "xyes"; then
  AC_DEFINE(BUILD_BROWSER_PODCASTS, 1, [Podcasts Browser])
fi

# SHOUTcast
want_browser_shoutcast="yes"
AC_MSG_CHECKING(whether shoutcast browser module is to be built)
AC_ARG_ENABLE(browser-shoutcast,
  AS_HELP_STRING([--enable-browser-shoutcast],[Enable SHOUTcast Browser Module]),
  [ want_browser_shoutcast=$enableval ]
)
AC_MSG_RESULT($want_browser_shoutcast)
AM_CONDITIONAL(BUILD_BROWSER_SHOUTCAST, test "x$want_browser_shoutcast" = "xyes")
if test "x$want_browser_shoutcast" = "xyes"; then
  AC_DEFINE(BUILD_BROWSER_SHOUTCAST, 1, [SHOUTcast Browser])
fi

# UPnP/DLNA
want_browser_upnp="no"
have_browser_upnp="no"
AC_MSG_CHECKING(whether UPnP/DLNA browser is to be built)
AC_ARG_ENABLE(browser-upnp,
  AS_HELP_STRING([--enable-browser-upnp],[Enable UPnP/DLNA Browser Module]),
  [ want_browser_upnp=$enableval ]
)
AC_MSG_RESULT($want_browser_upnp)
if test "x$want_browser_upnp" = "xyes" -o "x$want_browser_upnp" = "xauto"; then
  PKG_CHECK_MODULES(
    [GUPNP],
    [gssdp-1.0 gupnp-1.0 gupnp-av-1.0],
    [
      AC_DEFINE(BUILD_BROWSER_UPNP, 1, [UPnP/DLNA Browser Module])
      have_browser_upnp="yes"
    ],
    [
      if test "x$want_browser_upnp" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([gupnp-1.0 not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_BROWSER_UPNP, test "x$have_browser_upnp" = "xyes")

# Valhalla Scanner
want_browser_valhalla="auto"
have_browser_valhalla="no"
AC_MSG_CHECKING(whether Valhalla browser is to be built)
AC_ARG_ENABLE(browser_valhalla,
  AS_HELP_STRING([--enable-browser-valhalla],[Enable Valhalla Browser Module]),
  [ want_browser_valhalla=$enableval ]
)
AC_MSG_RESULT($want_browser_valhalla)
if test "x$want_browser_valhalla" = "xyes" -o "x$want_browser_valhalla" = "xauto"; then
  PKG_CHECK_MODULES(
    [VALHALLA],
    [libvalhalla],
    [
      AC_DEFINE(BUILD_BROWSER_VALHALLA, 1, [Valhalla Browser Module])
      have_browser_valhalla="yes"
    ],
    [
      if test "x$want_browser_valhalla" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([valhalla not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_BROWSER_VALHALLA, test "x$have_browser_valhalla" = "xyes")

#################
# Input Modules #
#################

# LIRC
have_input_lirc=no
AC_CHECK_HEADER(lirc/lirc_client.h, have_input_lirc=yes)
if test "x$have_input_lirc" = "xyes"; then
  AC_DEFINE([BUILD_INPUT_LIRC], [1], [Define if LIRC client lib is present])
  have_input_lirc="yes"
  LIRC_LIBS=-llirc_client
  AC_SUBST(LIRC_LIBS)
fi
AM_CONDITIONAL(BUILD_INPUT_LIRC, test "x$have_input_lirc" = "xyes")

####################
# Metadata Modules #
####################

# Allocine
want_metadata_allocine="yes"
AC_MSG_CHECKING(whether allocine.fr metadata module is to be built)
AC_ARG_ENABLE(metadata-allocine,
  AS_HELP_STRING([--enable-metadata-allocine],[Enable Allocine Metadata Module]),
  [ want_metadata_allocine=$enableval ]
)
AC_MSG_RESULT($want_metadata_allocine)
AM_CONDITIONAL(BUILD_METADATA_ALLOCINE, test "x$want_metadata_allocine" = "xyes")
if test "x$want_metadata_allocine" = "xyes"; then
  AC_DEFINE(BUILD_METADATA_ALLOCINE, 1, [Allocine.fr Metadata])
fi

# Amazon.com
want_metadata_amazon="yes"
AC_MSG_CHECKING(whether amazon.com metadata module is to be built)
AC_ARG_ENABLE(metadata-amazon,
  AS_HELP_STRING([--enable-metadata-amazon],[Enable Amazon Metadata Module]),
  [ want_metadata_amazon=$enableval ]
)
AC_MSG_RESULT($want_metadata_amazon)
AM_CONDITIONAL(BUILD_METADATA_AMAZON, test "x$want_metadata_amazon" = "xyes")
if test "x$want_metadata_amazon" = "xyes"; then
  AC_DEFINE(BUILD_METADATA_AMAZON, 1, [Amazon.com Metadata])
fi

# libplayer
want_metadata_libplayer="auto"
have_metadata_libplayer="no"
AC_MSG_CHECKING(whether libplayer metadata is to be built)
AC_ARG_ENABLE(metadata-libplayer,
    AS_HELP_STRING([--enable-metadata-libplayer],[Enable libplayer Metadata module]),
    [ want_metadata_libplayer=$enableval ]
)
AC_MSG_RESULT($want_metadata_libplayer)
if test "x$want_metadata_libplayer" = "xyes" -o "x$want_metadata_libplayer" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBPLAYER],
    [libplayer],
    [
      AC_DEFINE(BUILD_METADATA_LIBPLAYER, 1, [libplayer metadata module])
      have_metadata_libplayer="yes"
    ],
    [
      if test "x$want_metadata_libplayer" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libplayer not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_METADATA_LIBPLAYER, test "x$have_metadata_libplayer" = "xyes")

# Local Files
want_metadata_localfiles="yes"
AC_MSG_CHECKING(whether local files metadata module is to be built)
AC_ARG_ENABLE(metadata-localfiles,
  AS_HELP_STRING([--enable-metadata-localfiles],[Enable Local Files Metadata Module]),
  [ want_metadata_localfiles=$enableval ]
)
AC_MSG_RESULT($want_metadata_localfiles)
AM_CONDITIONAL(BUILD_METADATA_LOCALFILES, test "x$want_metadata_localfiles" = "xyes")
if test "x$want_metadata_localfiles" = "xyes"; then
  AC_DEFINE(BUILD_METADATA_LOCALFILES, 1, [Local Files Metadata])
fi

# NFO Reader
want_metadata_nfo="yes"
AC_MSG_CHECKING(whether NFO reader metadata module is to be built)
AC_ARG_ENABLE(metadata-nfo,
  AS_HELP_STRING([--enable-metadata-nfo],[Enable NFO Reader Metadata Module]),
  [ want_metadata_nfo=$enableval ]
)
AC_MSG_RESULT($want_metadata_nfo)
AM_CONDITIONAL(BUILD_METADATA_NFO, test "x$want_metadata_nfo" = "xyes")
if test "x$want_metadata_nfo" = "xyes"; then
  AC_DEFINE(BUILD_METADATA_NFO, 1, [NFO Reader Metadata])
fi

# TheMovieDB.org (TMDB)
want_metadata_tmdb="yes"
AC_MSG_CHECKING(whether themoviedb.org metadata module is to be built)
AC_ARG_ENABLE(metadata-tmdb,
  AS_HELP_STRING([--enable-metadata-tmdb],[Enable TheMovieDB Metadata Module]),
  [ want_metadata_tmdb=$enableval ]
)
AC_MSG_RESULT($want_metadata_tmdb)
AM_CONDITIONAL(BUILD_METADATA_TMDB, test "x$want_metadata_tmdb" = "xyes")
if test "x$want_metadata_tmdb" = "xyes"; then
  AC_DEFINE(BUILD_METADATA_TMDB, 1, [TheMovieDB.org Metadata])
fi

# TheTVDB.org (TVDB)
want_metadata_tvdb="yes"
AC_MSG_CHECKING(whether thetvdb.com metadata module is to be built)
AC_ARG_ENABLE(metadata-tvdb,
  AS_HELP_STRING([--enable-metadata-tvdb],[Enable TheTVDB Metadata Module]),
  [ want_metadata_tvdb=$enableval ]
)
AC_MSG_RESULT($want_metadata_tvdb)
AM_CONDITIONAL(BUILD_METADATA_TVDB, test "x$want_metadata_tvdb" = "xyes")
if test "x$want_metadata_tvdb" = "xyes"; then
  AC_DEFINE(BUILD_METADATA_TVDB, 1, [TheTVDB.com Metadata])
fi

##################
# Volume Modules #
##################

# HAL
want_volume_hal="auto"
have_volume_hal="no"
AC_MSG_CHECKING(whether hal volume module is to be built)
AC_ARG_ENABLE(volume-hal,
  AS_HELP_STRING([--enable-volume-hal],[Enable HAL Volume]),
  [ want_volume_hal=$enableval ]
)
AC_MSG_RESULT($want_volume_hal)
if test "x$want_volume_hal" = "xyes" -o "x$want_volume_hal" = "xauto"; then
  PKG_CHECK_MODULES(
    [EHAL],
    [ehal hal-storage],
    [
      AC_DEFINE(BUILD_VOLUME_HAL, 1, [HAL Volume])
      have_volume_hal="yes"
    ],
    [
      if test "x$want_volume_hal" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([ehal or hal-storage not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_VOLUME_HAL, test "x$have_volume_hal" = "xyes")

# mtab
want_volume_mtab="yes"
AC_MSG_CHECKING(whether mtab volume module is to be built)
AC_ARG_ENABLE(volume-mtab,
  AS_HELP_STRING([--enable-volume-mtab],[Enable mtab Volume Module]),
  [ want_volume_mtab=$enableval ]
)
AC_MSG_RESULT($want_volume_mtab)
AM_CONDITIONAL(BUILD_VOLUME_MTAB, test "x$want_volume_mtab" = "xyes")
if test "x$want_volume_mtab" = "xyes"; then
  AC_DEFINE(BUILD_VOLUME_MTAB, 1, [mtab Volume])
fi

##############
# Librairies #
##############

# efreet
want_efreet="auto"
have_efreet="no"
AC_MSG_CHECKING(whether efreet support is to be built)
AC_ARG_ENABLE(efreet,
    AS_HELP_STRING([--enable-efreet],[Enable efreet support]),
    [ want_efreet=$enableval ]
)
AC_MSG_RESULT($want_efreet)
if test "x$want_efreet" = "xyes" -o "x$want_efreet" = "xauto"; then
  PKG_CHECK_MODULES(
    [EFREET],
    [efreet],
    [
      AC_DEFINE(BUILD_EFREET, 1, [efreet support])
      have_efreet="yes"
    ],
    [
      if test "x$want_efreet" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([efreet not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_EFREET, test "x$have_efreet" = "xyes")

# libcddb
want_libcddb="auto"
have_libcddb="no"
AC_MSG_CHECKING(whether libcddb support is to be built)
AC_ARG_ENABLE(libcddb,
    AS_HELP_STRING([--enable-libcddb],[Enable libcddb support]),
    [ want_libcddb=$enableval ]
)
AC_MSG_RESULT($want_libcddb)
if test "x$want_libcddb" = "xyes" -o "x$want_libcddb" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBCDDB],
    [libcddb],
    [
      AC_DEFINE(BUILD_LIBCDDB, 1, [libcddb support])
      have_libcddb="yes"
    ],
    [
      if test "x$want_libcddb" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libcddb not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_LIBCDDB, test "x$have_libcddb" = "xyes")

# libcurl
want_libcurl="auto"
have_libcurl="no"
AC_MSG_CHECKING(whether libcurl support is to be built)
AC_ARG_ENABLE(libcurl,
    AS_HELP_STRING([--enable-libcurl],[Enable libcurl support]),
    [ want_libcurl=$enableval ]
)
AC_MSG_RESULT($want_libcurl)
if test "x$want_libcurl" = "xyes" -o "x$want_libcurl" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBCURL],
    [libcurl],
    [
      AC_DEFINE(BUILD_LIBCURL, 1, [lib support])
      have_libcurl="yes"
    ],
    [
      if test "x$want_libcurl" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libcurl not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_LIBCURL, test "x$have_libcurl" = "xyes")

# libexif
want_libexif="auto"
have_libexif="no"
AC_MSG_CHECKING(whether libexif support is to be built)
AC_ARG_ENABLE(libexif,
    AS_HELP_STRING([--enable-libexif],[Enable libexif support]),
    [ want_libexif=$enableval ]
)
AC_MSG_RESULT($want_libexif)
if test "x$want_libexif" = "xyes" -o "x$want_libexif" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBEXIF],
    [libexif],
    [
      AC_DEFINE(BUILD_LIBEXIF, 1, [libexif support])
      have_libexif="yes"
    ],
    [
      if test "x$want_libexif" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libexif not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_LIBEXIF, test "x$have_libexif" = "xyes")

# libxml2
want_libxml2="auto"
have_libxml2="no"
AC_MSG_CHECKING(whether libxml2 support is to be built)
AC_ARG_ENABLE(libxml2,
    AS_HELP_STRING([--enable-libxml2],[Enable libxml2 support]),
    [ want_libxml2=$enableval ]
)
AC_MSG_RESULT($want_libxml2)
if test "x$want_libxml2" = "xyes" -o "x$want_libxml2" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBXML2],
    [libxml-2.0],
    [
      AC_DEFINE(BUILD_LIBXML2, 1, [libxml2 support])
      have_libxml2="yes"
    ],
    [
      if test "x$want_libxml2" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libxml2 not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_LIBXML2, test "x$have_libxml2" = "xyes")

# libxrandr
want_libxrandr="auto"
have_libxrandr="no"
AC_MSG_CHECKING(whether libxrandr support is to be built)
AC_ARG_ENABLE(libxrandr,
    AS_HELP_STRING([--enable-libxrandr],[Enable libxrandr support]),
    [ want_libxrandr=$enableval ]
)
AC_MSG_RESULT($want_libxrandr)
if test "x$want_libxrandr" = "xyes" -o "x$want_libxrandr" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBXRANDR],
    [xrandr],
    [
      AC_DEFINE(BUILD_LIBXRANDR, 1, [libxrandr support])
      have_libxrandr="yes"
    ],
    [
      if test "x$want_libxrandr" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libxrandr not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_LIBXRANDR, test "x$have_libxrandr" = "xyes")

if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_REENTRANT"
fi

SHAVE_INIT([.], [enable])

AC_CONFIG_FILES([
shave
shave-libtool
Makefile
src/Makefile
src/bin/Makefile
src/modules/Makefile
src/modules/activity/Makefile
src/modules/activity/infos/Makefile
src/modules/activity/games/Makefile
src/modules/activity/music/Makefile
src/modules/activity/photo/Makefile
src/modules/activity/tv/Makefile
src/modules/activity/video/Makefile
src/modules/activity/weather/Makefile
src/modules/browser/Makefile
src/modules/browser/cdda/Makefile
src/modules/browser/dvd/Makefile
src/modules/browser/lms/Makefile
src/modules/browser/localfiles/Makefile
src/modules/browser/netstreams/Makefile
src/modules/browser/podcasts/Makefile
src/modules/browser/shoutcast/Makefile
src/modules/browser/upnp/Makefile
src/modules/browser/valhalla/Makefile
src/modules/input/Makefile
src/modules/input/lirc/Makefile
src/modules/metadata/Makefile
src/modules/metadata/amazon/Makefile
src/modules/metadata/libplayer/Makefile
src/modules/metadata/localfiles/Makefile
src/modules/metadata/nfo/Makefile
src/modules/metadata/tmdb/Makefile
src/modules/metadata/tvdb/Makefile
src/modules/metadata/allocine/Makefile
src/modules/volume/Makefile
src/modules/volume/hal/Makefile
src/modules/volume/mtab/Makefile
data/Makefile
data/other/Makefile
data/theme/Makefile
data/theme/default/Makefile
])

AC_OUTPUT

echo
echo "$PACKAGE $VERSION"
echo
eval echo "Installation Path.................. : $exec_prefix"
eval echo "\  Binaries......................... : $exec_prefix/bin"
eval echo "\  Modules.......................... : $exec_prefix/lib/enna/modules"
eval echo "\  Theme............................ : $exec_prefix/share/enna/theme"
echo
echo "Build theme........................ : $build_theme"
echo
echo "Supported Activity Modules:"
echo
echo "  infos............................ : $want_activity_infos"
echo "  games............................ : $want_activity_games"
echo "  music............................ : $want_activity_music"
echo "  photo............................ : $want_activity_photo"
echo "  tv............................... : $want_activity_tv"
echo "  video............................ : $want_activity_video"
echo "  weather.......................... : $want_activity_weather"
echo
echo "Supported Browser Modules:"
echo
echo "  Audio CD......................... : $want_browser_cdda"
echo "  DVD Video........................ : $want_browser_dvd"
echo "  Light Media Scanner.............. : $have_browser_lms"
echo "  Local files...................... : $want_browser_localfiles"
echo "  Network Streams.................. : $want_browser_netstreams"
echo "  Podcasts......................... : $want_browser_podcasts"
echo "  SHOUTcast........................ : $want_browser_shoutcast"
echo "  UPnP/DLNA........................ : $have_browser_upnp"
echo "  Valhalla Scanner................. : $have_browser_valhalla"
echo
echo "Supported Input Modules:"
echo
echo "  LIRC client...................... : $have_input_lirc"
echo
echo "Supported Metadata Modules:"
echo
echo "  Allocine.fr...................... : $want_metadata_allocine"
echo "  Amazon.com....................... : $want_metadata_amazon"
echo "  libplayer........................ : $have_metadata_libplayer"
echo "  Local files...................... : $want_metadata_localfiles"
echo "  NFO Reader....................... : $want_metadata_nfo"
echo "  TheMovieDB.org................... : $want_metadata_tmdb"
echo "  TheTVDB.com...................... : $want_metadata_tvdb"
echo
echo "Supported Volume Modules:"
echo
echo "  HAL.............................. : $have_volume_hal"
echo "  mtab............................. : $want_volume_mtab"
echo
echo "Optional linked-in librairies:"
echo
echo "  efreet........................... : $have_efreet"
echo "  libcddb.......................... : $have_libcddb"
echo "  libcurl.......................... : $have_libcurl"
echo "  libexif.......................... : $have_libexif"
echo "  libxml2.......................... : $have_libxml2"
echo "  libxrandr........................ : $have_libxrandr"
echo
echo "Now type 'make' ('gmake' on some systems) to compile $PACKAGE."
echo

