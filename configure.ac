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
   eet >= 0.9.9
   evas >= 0.9.9
   ecore >= 0.9.9.40
   ecore-evas
   ecore-txt
   ecore-file
   embryo >= 0.9.1
   edje >= 0.5.0
   edbus >= 0.0.1
   emotion
   sqlite3
   lightmediascanner >= 0.1
   libmusicbrainz
  ]
)
 
HAVE_LIRC=no
AC_CHECK_HEADER(lirc/lirc_client.h, HAVE_LIRC=yes)
if test "x$HAVE_LIRC" = "xyes"; then
  AC_DEFINE(HAVE_LIRC, 1, [Define if LIRC client lib is present])
  lirc_libs=-llirc_client
  AC_SUBST(lirc_libs)
fi
 
AC_CONFIG_FILES([
Makefile
src/Makefile
src/bin/Makefile
src/modules/Makefile
src/modules/music/Makefile
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
echo "  lirc client...................... : $HAVE_LIRC"
echo
echo "Now type 'make' ('gmake' on some systems) to compile $PACKAGE."
echo
