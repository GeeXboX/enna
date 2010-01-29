include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = games

# data files for the module
filesdir = $(libdir)/enna/modules/

# the module .so file
INCLUDES = \
-I. \
-I$(top_srcdir) \
-I$(top_srcdir)/src/modules/$(MODULE) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/modules \
@ENNA_CFLAGS@

EXTRA_DIST= \
games.h \
games_sys.h \
games_mame.h

if BUILD_ACTIVITY_GAMES

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_games.la

activity_games_la_SOURCES = $(SRCS_ACTIVITY_GAMES)

activity_games_la_LIBADD = @ENNA_LIBS@
activity_games_la_LDFLAGS = -module -avoid-version
activity_games_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
