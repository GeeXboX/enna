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

if BUILD_GADGET_WEATHER

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = gadget_weather.la

gadget_weather_la_SOURCES = $(SRCS_GADGET_WEATHER)

gadget_weather_la_LIBADD = @ENNA_LIBS@
gadget_weather_la_LDFLAGS = -module -avoid-version
gadget_weather_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/input_$(MODULE).*

endif
