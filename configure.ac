# get rid of that stupid cache mechanism
rm -f config.cache

AC_INIT(enna, 0.3.0, www.digital-corner.org)
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(configure.ac)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AM_INIT_AUTOMAKE(1.6 dist-bzip2)
AM_CONFIG_HEADER(config.h)

define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
AC_PROG_LIBTOOL

AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_C_CONST

PKG_CHECK_MODULES([ENNA],
  [
   eet >= 1.0.0
   evas >= 0.9.9
   ecore >= 0.9.9.40
   ecore-evas
   ecore-txt
   ecore-file
   embryo >= 0.9.1
   edje >= 0.5.0
  ]
)

have_lirc_input=no
AC_CHECK_HEADER(lirc/lirc_client.h, have_lirc_input=yes)
if test "x$have_lirc_input" = "xyes"; then
  AC_DEFINE(HAVE_LIRC, 1, [Define if LIRC client lib is present])
  lirc_libs=-llirc_client
  AC_SUBST(lirc_libs)
fi

#######################################
## Check if we should build the emotion backend
want_emotion_backend="auto"
have_emotion_backend="no"
AC_MSG_CHECKING(whether emotion backend is to be built)
AC_ARG_ENABLE(emotion-backend,
  AC_HELP_STRING([--enable-emotion-backend], [Enable Emotion media backend]),
  [ want_emotion_backend=$enableval ]
)
AC_MSG_RESULT($want_emotion_backend)
if test "x$want_emotion_backend" = "xyes" -o "x$want_emotion_backend" = "xauto"; then
  PKG_CHECK_MODULES(
    [EMOTION],
    [emotion],
    [
      AC_DEFINE(BUILD_EMOTION_BACKEND, 1, [Emotion media backend])
      have_emotion_backend="yes"
    ],
    [
      if test "x$want_emotion_backend" = "xyes" -a "x$use_strict" = "xyes" ; then
        AC_MSG_ERROR([Emotion not found (strict dependencies checking)])
      fi
    ]
  )
fi
AM_CONDITIONAL(BUILD_EMOTION_BACKEND, test "x$have_emotion_backend" = "xyes")

#######################################
## Check if we should build the libplayer backend
want_libplayer_backend="auto"
have_libplayer_backend="no"

AC_MSG_CHECKING(whether libplayer backend is to be built)
AC_ARG_ENABLE(libplayer-backend,
    AC_HELP_STRING([--enable-libplayer-backend], [Enable libplayer media backend]),
    [ want_libplayer_backend=$enableval ]
)
AC_MSG_RESULT($want_libplayer_backend)
if test "x$want_libplayer_backend" = "xyes" -o "x$want_libplayer_backend" = "xauto"; then
  AC_CHECK_LIB([player], [player_init], [have_libplayer_backend=yes], [have_libplayer_backend=no],)
  if test "x$have_libplayer_backend" = "xyes" -a "x$use_strict" = "xyes" ; then
    AC_MSG_ERROR([libplayer support explicitly requested but libplayer couldn't be found])
  fi
  if test "x$have_libplayer_backend" = "xyes" ; then
    AC_DEFINE(BUILD_LIPLAYER_BACKEND, 1, [Libplayer media backend])
    libplayer_libs=-lplayer
    AC_SUBST(libplayer_libs)
  fi
fi
AM_CONDITIONAL(BUILD_LIBPLAYER_BACKEND, test "x$have_libplayer_backend" = "xyes")

AC_CONFIG_FILES([
Makefile
src/Makefile
src/bin/Makefile
src/modules/Makefile
src/modules/music/Makefile
src/modules/video/Makefile
src/modules/browser/Makefile
src/modules/backend/Makefile
src/modules/backend/emotion/Makefile
data/Makefile
data/theme/Makefile
data/theme/default/Makefile
])

AC_OUTPUT

echo
echo "$PACKAGE $VERSION"
echo
echo "Optional Modules:"
echo
echo "  lirc client...................... : $have_lirc_input"
echo "  emotion backend.................. : $have_emotion_backend"
echo "  libplayer backend................ : $have_libplayer_backend"
echo
echo "Now type 'make' ('gmake' on some systems) to compile $PACKAGE."
echo
