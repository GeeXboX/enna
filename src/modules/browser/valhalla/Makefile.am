include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = valhalla

# data files for the module
filesdir = $(libdir)/enna/modules/

# the module .so file
INCLUDES = \
-I. \
-I$(top_srcdir) \
-I$(top_srcdir)/src/modules/$(MODULE) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/modules \
@ENNA_CFLAGS@ @VALHALLA_CFLAGS@

if BUILD_BROWSER_VALHALLA

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = browser_valhalla.la

browser_valhalla_la_SOURCES = $(SRCS_BROWSER_VALHALLA)

browser_valhalla_la_LIBADD = @ENNA_LIBS@ @VALHALLA_LIBS@
browser_valhalla_la_LDFLAGS = -module -avoid-version
browser_valhalla_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/browser_$(MODULE).*

endif
