include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = weather

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

if BUILD_ACTIVITY_WEATHER

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_weather.la

activity_weather_la_SOURCES = $(SRCS_ACTIVITY_WEATHER)

activity_weather_la_LIBADD = @ENNA_LIBS@
activity_weather_la_LDFLAGS = -module -avoid-version
activity_weather_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
