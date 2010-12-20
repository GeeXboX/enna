# get rid of that stupid cache mechanism
rm -f config.cache

AC_INIT([enna],[0.4.0],[http://enna.geexbox.org])
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

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])
CFLAGS="$CFLAGS -DLOCALEDIR=\"\\\"$datadir/locale\\\"\""

PKG_CHECK_MODULES([ENNA],
  [
   eina >= 1.0.0
   eet >= 1.4.0
   evas >= 1.0.0
   ecore >= 1.0.0
   ecore-evas >= 1.0.0
   ecore-file >= 1.0.0
   ecore-ipc >= 1.0.0
   ecore-con >= 1.0.0
   edje >= 1.0.0
   edbus >= 1.0.0
   ethumb >= 0.1.0
   elementary >= 0.7.0
   libvalhalla >= 2.0.0 libvalhalla < 3.0.0
   libxml-2.0
   libcurl
  ]
)

# Evil library for compilation on Windows

case "$host_os" in
   mingw*)
      PKG_CHECK_MODULES([EVIL], [evil])
      AC_DEFINE(HAVE_EVIL, 1, [Set to 1 if evil package is installed])
   ;;
esac
AC_SUBST(EFL_EINA_BUILD)

##################
# Static Modules #
##################

AC_ARG_ENABLE(static_modules,
  [AC_HELP_STRING([--disable-static-modules], [disable static modules.
  [[default=enabled]]])],
  [use_static_modules=$enableval],
  [use_static_modules="yes"]
)
AM_CONDITIONAL([USE_STATIC_MODULES], test "x${use_static_modules}" = "xyes")
if test "x$use_static_modules" = "xyes"; then
  AC_DEFINE(USE_STATIC_MODULES, 1, [Static Modules])
fi

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

###############
# A/V Backend #
#########"#####

# Libplayer
want_backend_libplayer="yes"
have_backend_libplayer="no"
AC_MSG_CHECKING(whether libplayer backend is to be built)
AC_ARG_ENABLE(backend-libplayer,
  AS_HELP_STRING([--enable-backend-libplayer],[Enable Libplayer A/V backend]),
  [ want_backend_libplayer=$enableval ]
)
AC_MSG_RESULT($want_backend_libplayer)
AM_CONDITIONAL(BUILD_BACKEND_LIBPLAYER, test "x$want_backend_libplayer" = "xyes")
if test "x$want_backend_libplayer" = "xyes"; then
 PKG_CHECK_MODULES(
    [LIBPLAYER],
    [libplayer >= 2.0.0 libplayer < 3.0.0],
    [
      AC_DEFINE(BUILD_BACKEND_LIBPLAYER, 1, [Libplayer Backend])
      have_backend_libplayer="yes"
    ],
    [
      if test "x$want_backend_libplayer" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libplayer >= 2.0.0 not found (strict dependencies checking)])
      fi
    ]
  )
  AC_DEFINE(BUILD_BACKEND_LIBPLAYER, 1, [libplayer Backend])
fi

# Emotion
want_backend_emotion="no"
have_backend_emotion="no"
AC_MSG_CHECKING(whether emotion backend is to be built)
AC_ARG_ENABLE(backend-emotion,
  AS_HELP_STRING([--enable-backend-emotion],[Enable Emotion A/V backend]),
  [ want_backend_emotion=$enableval ]
)
AC_MSG_RESULT($want_backend_emotion)
AM_CONDITIONAL(BUILD_BACKEND_EMOTION, test "x$want_backend_emotion" = "xyes")
if test "x$want_backend_emotion" = "xyes"; then
 PKG_CHECK_MODULES(
    [EMOTION],
    [emotion],
    [
      AC_DEFINE(BUILD_BACKEND_EMOTION, 1, [Emotion Backend])
      have_backend_emotion="yes"
    ],
    [
      if test "x$want_backend_emotion" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([emotion not found (strict dependencies checking)])
      fi
    ]
  )
  AC_DEFINE(BUILD_BACKEND_EMOTION, 1, [emotion Backend])
fi

####################
# Activity Modules #
####################

# Bookstore
want_activity_bookstore="no"
AC_MSG_CHECKING(whether bookstore activity module is to be built)
AC_ARG_ENABLE(activity-bookstore,
  AS_HELP_STRING([--enable-activity-bookstore],[Enable bookstore Activity Module]),
  [ want_activity_bookstore=$enableval ]
)
AC_MSG_RESULT($want_activity_bookstore)
AM_CONDITIONAL(BUILD_ACTIVITY_BOOKSTORE, test "x$want_activity_bookstore" = "xyes")
if test "x$want_activity_bookstore" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_BOOKSTORE, 1, [bookstore Activity])
fi

# Configuration
want_activity_configuration="yes"
AC_MSG_CHECKING(whether configuration activity module is to be built)
AC_ARG_ENABLE(activity-configuration,
  AS_HELP_STRING([--enable-activity-configuration],[Enable configuration Activity Module]),
  [ want_activity_configuration=$enableval ]
)
AC_MSG_RESULT($want_activity_configuration)
AM_CONDITIONAL(BUILD_ACTIVITY_CONFIGURATION, test "x$want_activity_configuration" = "xyes")
if test "x$want_activity_configuration" = "xyes"; then
  AC_DEFINE(BUILD_ACTIVITY_CONFIGURATION, 1, [configuration Activity])
fi

# Games
want_activity_games="no"
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
want_activity_tv="no"
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
want_browser_cdda="no"
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

# iPod/iPhone support
want_browser_ipod="no"
have_browser_ipod="no"
AC_MSG_CHECKING(whether iPod browser is to be built)
AC_ARG_ENABLE(browser-ipod,
  AS_HELP_STRING([--enable-browser-ipod],[Enable iPod Browser Module]),
  [ want_browser_ipod=$enableval ]
)
AC_MSG_RESULT($want_browser_ipod)
if test "x$want_browser_ipod" = "xyes" -o "x$want_browser_ipod" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBGPOD],
    [libgpod-1.0 >= 0.7.93],
    [
      AC_DEFINE(BUILD_BROWSER_IPOD, 1, [iPod Browser Module])
      have_browser_ipod="yes"
    ],
    [
      if test "x$want_browser_ipod" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libgpod not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_BROWSER_IPOD, test "x$have_browser_ipod" = "xyes")

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
want_browser_netstreams="no"
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
want_browser_podcasts="no"
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
want_browser_shoutcast="no"
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

# Spotify support
want_browser_spotify="no"
have_browser_spotify="no"
AC_MSG_CHECKING(whether spotify browser is to be built)
AC_ARG_ENABLE(browser-spotify,
  AS_HELP_STRING([--enable-browser-spotify],[Enable spotify Browser Module]),
  [ want_browser_spotify=$enableval ]
)
AC_MSG_RESULT($want_browser_spotify)
if test "x$want_browser_spotify" = "xyes" -o "x$want_browser_spotify" = "xauto"; then
  PKG_CHECK_MODULES(
    [SPOTIFY],
    [libspotify],
    [
      AC_DEFINE(BUILD_BROWSER_SPOTIFY, 1, [spotify Browser Module])
      have_browser_spotify="yes"
    ],
    [
      if test "x$want_browser_spotify" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libspotify not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_BROWSER_SPOTIFY, test "x$have_browser_spotify" = "xyes")

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
    [gssdp-1.0 >= 0.7.0 gupnp-1.0 >= 0.13.1 gupnp-av-1.0 >= 0.5.2],
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
want_browser_valhalla="yes"
AC_MSG_CHECKING(whether Valhalla browser is to be built)
AC_ARG_ENABLE(browser_valhalla,
  AS_HELP_STRING([--enable-browser-valhalla],[Enable Valhalla Browser Module]),
  [ want_browser_valhalla=$enableval ]
)
AC_MSG_RESULT($want_browser_valhalla)
AM_CONDITIONAL(BUILD_BROWSER_VALHALLA, test "x$want_browser_valhalla" = "xyes")
if test "x$want_browser_valhalla" = "xyes"; then
  AC_DEFINE(BUILD_BROWSER_VALHALLA, 1, [Valhalla Browser Module])
fi

###################
# Gadgets Modules #
###################

# Date gadget
want_gadget_date="yes"
AC_MSG_CHECKING(whether Date gadget is to be built)
AC_ARG_ENABLE(gadget_date,
  AS_HELP_STRING([--enable-gadget-date],[Enable Date Gadget Module]),
  [ want_gadget_date=$enableval ]
)
AC_MSG_RESULT($want_gadget_date)
AM_CONDITIONAL(BUILD_GADGET_DATE, test "x$want_gadget_date" = "xyes")
if test "x$want_gadget_date" = "xyes"; then
  AC_DEFINE(BUILD_GADGET_DATE, 1, [Gadget date Module])
fi

# Dummy gadget
want_gadget_dummy="no"
AC_MSG_CHECKING(whether Dummy gadget is to be built)
AC_ARG_ENABLE(gadget_dummy,
  AS_HELP_STRING([--enable-gadget-dummy],[Enable Dummy Gadget Module]),
  [ want_gadget_dummy=$enableval ]
)
AC_MSG_RESULT($want_gadget_dummy)
AM_CONDITIONAL(BUILD_GADGET_DUMMY, test "x$want_gadget_dummy" = "xyes")
if test "x$want_gadget_dummy" = "xyes"; then
  AC_DEFINE(BUILD_GADGET_DUMMY, 1, [Gadget dummy Module])
fi

# Weather gadget
want_gadget_weather="yes"
AC_MSG_CHECKING(whether Weather gadget is to be built)
AC_ARG_ENABLE(gadget_weather,
  AS_HELP_STRING([--enable-gadget-weather],[Enable Weather Gadget Module]),
  [ want_gadget_weather=$enableval ]
)
AC_MSG_RESULT($want_gadget_weather)
AM_CONDITIONAL(BUILD_GADGET_WEATHER, test "x$want_gadget_weather" = "xyes")
if test "x$want_gadget_weather" = "xyes"; then
  AC_DEFINE(BUILD_GADGET_WEATHER, 1, [Gadget weather Module])
fi

#################
# Input Modules #
#################

# Keyboard
want_input_keyboard="yes"
AC_MSG_CHECKING(whether keyboard input module is to be built)
AC_ARG_ENABLE(input-keyboard,
  AS_HELP_STRING([--enable-input-keyboard],[Enable Keyboard Input Module]),
  [ want_input_keyboard=$enableval ]
)
AC_MSG_RESULT($want_input_keyboard)
AM_CONDITIONAL(BUILD_INPUT_KBD, test "x$want_input_keyboard" = "xyes")
if test "x$want_input_keyboard" = "xyes"; then
  AC_DEFINE(BUILD_INPUT_KBD, 1, [Keyboard Input])
fi

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

# Wiimote
want_input_wiimote="auto"
have_input_wiimote="no"
AC_MSG_CHECKING(whether wiimote input module is to be built)
AC_ARG_ENABLE(input-wiimote,
  AS_HELP_STRING([--enable-input-wiimote], [Enable Wiimote input module]),
  [ want_input_wiimote=$enableval ]
)
AC_MSG_RESULT($want_input_wiimote)
if test "x$want_input_wiimote" = "xyes" -o "x$want_input_wiimote" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBBLUETOOTH],
    [bluez],
    [
      AC_CHECK_HEADER(cwiid.h,
        [
          AC_DEFINE(BUILD_INPUT_WIIMOTE, 1, [Wiimote Input])
          LIBCWIID_LIBS=-lcwiid
          AC_SUBST(LIBCWIID_LIBS)
          have_input_wiimote="yes"
        ],
        [
          if test "x$want_input_wiimote" = "xyes" -a "x$use_strict" = "xyes" ; then
            AC_MSG_ERROR([libcwiid not found (strict dependencies checking)])
          fi
        ]
      )
    ],
    [
      if test "x$want_input_wiimote" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libbluetooth not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_INPUT_WIIMOTE, test "x$have_input_wiimote" = "xyes")

##################
# Volume Modules #
##################

# HAL
want_volume_hal="no"
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
    [ehal edbus],
    [
      AC_DEFINE(BUILD_VOLUME_HAL, 1, [HAL Volume])
      have_volume_hal="yes"
    ],
    [
      if test "x$want_volume_hal" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([ehal or e-dbus not found (strict dependencies checking)])
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

# Udev
want_volume_udev="auto"
have_volume_udev="no"
AC_MSG_CHECKING(whether Udev volume module is to be built)
AC_ARG_ENABLE(volume-udev,
  AS_HELP_STRING([--enable-volume-udev],[Enable Udev Volume]),
  [ want_volume_udev=$enableval ]
)
AC_MSG_RESULT($want_volume_udev)
if test "x$want_volume_udev" = "xyes" -o "x$want_volume_udev" = "xauto"; then
  PKG_CHECK_MODULES(
    [UDEV],
    [libudev],
    [
      AC_DEFINE(BUILD_VOLUME_UDEV, 1, [Udev Volume])
      have_volume_udev="yes"
    ],
    [
      if test "x$want_volume_udev" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([udev not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_VOLUME_UDEV, test "x$have_volume_udev" = "xyes")

##############
# Librairies #
##############

# ecore-x
want_ecore_x="auto"
have_ecore_x="no"
AC_MSG_CHECKING(whether ecore-x support is to be built)
AC_ARG_ENABLE(ecore-x,
    AS_HELP_STRING([--enable-ecore-x],[Enable ecore X11 support]),
    [ want_ecore_x=$enableval ]
)
AC_MSG_RESULT($want_ecore_x)
if test "x$want_ecore_x" = "xyes" -o "x$want_ecore_x" = "xauto"; then
  PKG_CHECK_MODULES(
    [ECORE_X],
    [ecore-x],
    [
      AC_DEFINE(BUILD_ECORE_X, 1, [ecore-x support])
      have_ecore_x="yes"
    ],
    [
      if test "x$want_ecore_x" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([ecore-x not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_ECORE_X, test "x$have_ecore_x" = "xyes")

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

# libsvdrp
want_libsvdrp="auto"
have_libsvdrp="no"
AC_MSG_CHECKING(whether libsvdrp support is to be built)
AC_ARG_ENABLE(libsvdrp,
    AS_HELP_STRING([--enable-libsvdrp],[Enable libsvdrp support]),
    [ want_libsvdrp=$enableval ]
)
AC_MSG_RESULT($want_libsvdrp)
if test "x$want_libsvdrp" = "xyes" -o "x$want_libsvdrp" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBSVDRP],
    [libsvdrp],
    [
      AC_DEFINE(BUILD_LIBSVDRP, 1, [libsvdrp support])
      have_libsvdrp="yes"
    ],
    [
      if test "x$want_libsvdrp" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libsvdrp not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_LIBSVDRP, test "x$have_libsvdrp" = "xyes")

# libxdg-basedir
want_libxdg_basedir="auto"
have_libxdg_basedir="no"
AC_MSG_CHECKING(whether libxdg-basedir support is to be built)
AC_ARG_ENABLE(libxdg-basedir,
    AS_HELP_STRING([--enable-libxdg-basedir],[Enable libxdg-basedir support]),
    [ want_libxdg_basedir=$enableval ]
)
AC_MSG_RESULT($want_libxdg_basedir)
if test "x$want_libxdg_basedir" = "xyes" -o "x$want_libxdg_basedir" = "xauto"; then
  PKG_CHECK_MODULES(
    [LIBXDG_BASEDIR],
    [libxdg-basedir],
    [
      AC_DEFINE(BUILD_LIBXDG_BASEDIR, 1, [libxdg-basedir support])
      have_libxdg_basedir="yes"
    ],
    [
      if test "x$want_libxdg_basedir" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([libxdg-basedir not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_LIBXDG_BASEDIR, test "x$have_libxdg_basedir" = "xyes")

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

##########
# THEMES #
##########

# Default theme
want_theme_default="yes"
AC_MSG_CHECKING(whether Default Theme is to be built)
AC_ARG_ENABLE(theme_default,
  AS_HELP_STRING([--enable-theme-default],[Enable Default Theme]),
  [ want_theme_default=$enableval ]
)
AC_MSG_RESULT($want_theme_default)
AM_CONDITIONAL(BUILD_THEME_DEFAULT, test "x$want_theme_default" = "xyes")
if test "x$want_theme_default" = "xyes"; then
  AC_DEFINE(BUILD_THEME_DEFAULT, 1, [Default Theme])
fi

# Set Top Box theme
want_theme_stb="yes"
AC_MSG_CHECKING(whether Stb Theme is to be built)
AC_ARG_ENABLE(theme_stb,
  AS_HELP_STRING([--enable-theme-stb],[Enable Stb Theme]),
  [ want_theme_stb=$enableval ]
)
AC_MSG_RESULT($want_theme_stb)
AM_CONDITIONAL(BUILD_THEME_STB, test "x$want_theme_stb" = "xyes")
if test "x$want_theme_stb" = "xyes"; then
  AC_DEFINE(BUILD_THEME_STB, 1, [Stb Theme])
fi


# Tablet (and other touchscreen embedded devices) Theme
want_theme_tablet="no"
AC_MSG_CHECKING(whether tablet theme is to be built)
AC_ARG_ENABLE(theme_tablet,
  AS_HELP_STRING([--enable-theme-tablet],[Enable Tablet Theme]),
  [ want_theme_tablet=$enableval ]
)
AC_MSG_RESULT($want_theme_tablet)
AM_CONDITIONAL(BUILD_THEME_TABLET, test "x$want_theme_tablet" = "xyes")
if test "x$want_theme_tablet" = "xyes"; then
  AC_DEFINE(BUILD_THEME_TABLET, 1, [Tablet Theme])
fi

if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_REENTRANT"
fi

# silent rules require automake 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],[AC_SUBST([AM_DEFAULT_VERBOSITY],[1])])

AC_CONFIG_FILES([
Makefile
po/Makefile.in
src/Makefile
src/bin/Makefile
src/modules/Makefile
src/modules/activity/Makefile
src/modules/activity/bookstore/Makefile
src/modules/activity/configuration/Makefile
src/modules/activity/games/Makefile
src/modules/activity/music/Makefile
src/modules/activity/photo/Makefile
src/modules/activity/tv/Makefile
src/modules/activity/video/Makefile
src/modules/activity/weather/Makefile
src/modules/browser/Makefile
src/modules/browser/cdda/Makefile
src/modules/browser/dvd/Makefile
src/modules/browser/ipod/Makefile
src/modules/browser/localfiles/Makefile
src/modules/browser/netstreams/Makefile
src/modules/browser/podcasts/Makefile
src/modules/browser/shoutcast/Makefile
src/modules/browser/spotify/Makefile
src/modules/browser/upnp/Makefile
src/modules/browser/valhalla/Makefile
src/modules/gadgets/Makefile
src/modules/gadgets/weather/Makefile
src/modules/gadgets/date/Makefile
src/modules/gadgets/dummy/Makefile
src/modules/input/Makefile
src/modules/input/kbd/Makefile
src/modules/input/lirc/Makefile
src/modules/input/wiimote/Makefile
src/modules/volume/Makefile
src/modules/volume/hal/Makefile
src/modules/volume/mtab/Makefile
src/modules/volume/udev/Makefile
data/Makefile
data/other/Makefile
data/theme/Makefile
data/theme/default/Makefile
data/theme/stb/Makefile
data/theme/tablet/Makefile
])

AC_OUTPUT

echo
echo "$PACKAGE $VERSION"
echo
eval echo "Installation Path.................. : $exec_prefix"
eval echo "\  Binaries......................... : $exec_prefix/bin"

if test "x$use_static_modules" = "xno"; then
eval echo "\  Modules.......................... : $exec_prefix/lib/enna/modules"
fi

eval echo "\  Theme............................ : $exec_prefix/share/enna/theme"
echo
echo "Build theme........................ : $build_theme"
echo "Static Modules..................... : $use_static_modules"
echo "NLS................................ : $USE_NLS"
echo
echo "Supported Activity Modules:"
echo
echo "  Bookstore........................ : $want_activity_bookstore"
echo "  Configuration.................... : $want_activity_configuration"
echo "  Games............................ : $want_activity_games"
echo "  Music............................ : $want_activity_music"
echo "  Photo............................ : $want_activity_photo"
echo "  TV............................... : $want_activity_tv"
echo "  Video............................ : $want_activity_video"
echo "  Weather.......................... : $want_activity_weather"
echo
echo
echo "Supported Backend:"
echo
echo "  Libplayer........................ : $have_backend_libplayer"
echo "  Emotion.......................... : $have_backend_emotion"
echo
echo "Supported Browser Modules:"
echo
echo "  Audio CD......................... : $want_browser_cdda"
echo "  DVD Video........................ : $want_browser_dvd"
echo "  iPod/iPhone/iPad................. : $have_browser_ipod"
echo "  Local files...................... : $want_browser_localfiles"
echo "  Network Streams.................. : $want_browser_netstreams"
echo "  Podcasts......................... : $want_browser_podcasts"
echo "  SHOUTcast........................ : $want_browser_shoutcast"
echo "  Spotify.......................... : $want_browser_spotify"
echo "  UPnP/DLNA........................ : $have_browser_upnp"
echo "  Valhalla Scanner................. : $want_browser_valhalla"
echo
echo "Supported Gadget Modules:"
echo
echo "  Date............................. : $want_gadget_date"
echo "  Dummy............................ : $want_gadget_dummy"
echo "  Weather.......................... : $want_gadget_weather"
echo
echo "Supported Input Modules:"
echo
echo "  Keyboard client.................. : $want_input_keyboard"
echo "  LIRC client...................... : $have_input_lirc"
echo "  Wiimote client................... : $have_input_wiimote"
echo
echo "Supported Volume Modules:"
echo
echo "  HAL (deprecated)................. : $have_volume_hal"
echo "  mtab............................. : $want_volume_mtab"
echo "  Udev............................. : $have_volume_udev"
echo
echo "Optional linked-in librairies:"
echo
echo "  ecore-x.......................... : $have_ecore_x"
echo "  libcddb.......................... : $have_libcddb"
echo "  libsvdrp......................... : $have_libsvdrp"
echo "  libxdg-basedir................... : $have_libxdg_basedir"
echo "  libxrandr........................ : $have_libxrandr"
echo
echo "Suppported Themes:"
echo
echo "  default.......................... : $want_theme_default"
echo "  stb.............................. : $want_theme_stb"
echo "  tablet........................... : $want_theme_tablet"
echo
echo "Now type 'make' ('gmake' on some systems) to compile $PACKAGE."
echo

