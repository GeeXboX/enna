MAINTAINERCLEANFILES = Makefile.in

MODULE = cdda

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

if BUILD_BROWSER_CDDA

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = browser_cdda.la

browser_cdda_la_SOURCES= \
cdda.c

browser_cdda_la_LIBADD = @ENNA_LIBS@
browser_cdda_la_LDFLAGS = -module -avoid-version
browser_cdda_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/browser_$(MODULE).*

endif
