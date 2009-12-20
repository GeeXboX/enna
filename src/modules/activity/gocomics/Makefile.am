include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = gocomics

# data files for the module
filesdir = $(libdir)/enna/modules/

# the module .so file
INCLUDES = \
-I. \
-I$(top_srcdir) \
-I$(top_srcdir)/src/modules/$(MODULE) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/modules \
@ENNA_CFLAGS@ @LIBCURL_CFLAGS@

if BUILD_ACTIVITY_GOCOMICS
if BUILD_LIBCURL

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_gocomics.la

activity_gocomics_la_SOURCES = $(SRCS_ACTIVITY_GOCOMICS)

activity_gocomics_la_LIBADD = @ENNA_LIBS@ @LIBCURL_LIBS@
activity_gocomics_la_LDFLAGS = -module -avoid-version
activity_gocomics_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
endif
