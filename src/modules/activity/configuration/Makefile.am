MAINTAINERCLEANFILES = Makefile.in

MODULE = configuration

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

if BUILD_BROWSER_VALHALLA
INCLUDES += @VALHALLA_CFLAGS@
endif

if BUILD_LIBSVDRP
INCLUDES += @LIBSVDRP_CFLAGS@
endif

if BUILD_LIBXRANDR
INCLUDES += @LIBXRANDR_CFLAGS@
endif

if BUILD_ACTIVITY_CONFIGURATION

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_configuration.la

activity_configuration_la_SOURCES= \
configuration.c \
infos.c \
infos.h

activity_configuration_la_LIBADD = @ENNA_LIBS@
activity_configuration_la_LDFLAGS = -module -avoid-version
activity_configuration_la_DEPENDENCIES = $(top_builddir)/config.h

if BUILD_LIBSVDRP
activity_configuration_la_LIBADD += @LIBSVDRP_LIBS@
endif

if BUILD_LIBXRANDR
activity_configuration_la_LIBADD += @LIBXRANDR_LIBS@
endif

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
