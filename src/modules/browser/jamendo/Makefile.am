include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = jamendo

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

if BUILD_BROWSER_JAMENDO

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = browser_jamendo.la

browser_jamendo_la_SOURCES = $(SRCS_BROWSER_JAMENDO)

browser_jamendo_la_LIBADD = @ENNA_LIBS@
browser_jamendo_la_LDFLAGS = -module -avoid-version
browser_jamendo_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/browser_$(MODULE).*

endif
