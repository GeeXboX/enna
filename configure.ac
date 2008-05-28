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
  AC_DEFINE([HAVE_LIRC], [1], [Define if LIRC client lib is present])
  lirc_libs=-llirc_client
  AC_SUBST(lirc_libs)
fi

#######################################
## Check if we should build the emotion backend
want_emotion_backend="auto"
have_emotion_backend="no"
AC_MSG_CHECKING(whether emotion backend is to be built)
AC_ARG_ENABLE(emotion-backend,
  AS_HELP_STRING([--enable-emotion-backend],[Enable Emotion media backend]),
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
    AS_HELP_STRING([--enable-libplayer-backend],[Enable libplayer media backend]),
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

if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_REENTRANT"
fi

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
src/modules/backend/libplayer/Makefile
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
