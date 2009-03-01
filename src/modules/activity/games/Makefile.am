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
@ENNA_CFLAGS@ @EFREET_CFLAGS@

if BUILD_ACTIVITY_GAMES
if BUILD_EFREET

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_games.la

activity_games_la_SOURCES= \
enna_module_games.c

activity_games_la_LIBADD = @ENNA_LIBS@ @EFREET_LIBS@
activity_games_la_LDFLAGS = -module -avoid-version
activity_games_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
endif
