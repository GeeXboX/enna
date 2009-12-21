include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = onemanga

# data files for the module
filesdir = $(libdir)/enna/modules/

# the module .so file
INCLUDES = \
-I. \
-I$(top_srcdir) \
-I$(top_srcdir)/src/modules/$(MODULE) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/modules \
@ENNA_CFLAGS@ @LIBXML2_CFLAGS@ @LIBCURL_CFLAGS@

if BUILD_ACTIVITY_ONEMANGA
if BUILD_LIBXML2
if BUILD_LIBCURL

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_onemanga.la

activity_onemanga_la_SOURCES = $(SRCS_ACTIVITY_ONEMANGA)

activity_onemanga_la_LIBADD = @ENNA_LIBS@ @LIBXML2_LIBS@ @LIBCURL_LIBS@
activity_onemanga_la_LDFLAGS = -module -avoid-version
activity_onemanga_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
endif
endif
