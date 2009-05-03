MAINTAINERCLEANFILES = Makefile.in

MODULE = infos

# data files for the module
filesdir = $(libdir)/enna/modules/

# the module .so file
INCLUDES = \
-I. \
-I$(top_srcdir) \
-I$(top_srcdir)/src/modules/$(MODULE) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/modules \
@ENNA_CFLAGS@ @LIBPLAYER_CFLAGS@

if BUILD_BROWSER_VALHALLA
INCLUDES += @VALHALLA_CFLAGS@
endif

if BUILD_LIBSVDRP
INCLUDES += @LIBSVDRP_CFLAGS@
endif

if BUILD_LIBXRANDR
INCLUDES += @LIBXRANDR_CFLAGS@
endif

if BUILD_ACTIVITY_INFOS

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_infos.la

activity_infos_la_SOURCES= \
infos.c

activity_infos_la_LIBADD = @ENNA_LIBS@
activity_infos_la_LDFLAGS = -module -avoid-version
activity_infos_la_DEPENDENCIES = $(top_builddir)/config.h

if BUILD_LIBSVDRP
activity_infos_la_LIBADD += @LIBSVDRP_LIBS@
endif

if BUILD_LIBXRANDR
activity_infos_la_LIBADD += @LIBXRANDR_LIBS@
endif

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
