include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = podcast

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

if BUILD_BROWSER_PODCASTS

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = browser_podcast.la

browser_podcast_la_SOURCES = $(SRCS_BROWSER_PODCASTS)

browser_podcast_la_LIBADD = @ENNA_LIBS@
browser_podcast_la_LDFLAGS = -module -avoid-version
browser_podcast_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/browser_$(MODULE).*

endif
