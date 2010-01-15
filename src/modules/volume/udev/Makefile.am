include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = udev

# data files for the module
filesdir = $(libdir)/enna/modules/

# the module .so file
INCLUDES = \
-I. \
-I$(top_srcdir) \
-I$(top_srcdir)/src/modules/$(MODULE) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/modules \
@ENNA_CFLAGS@ @UDEV_CFLAGS@

EXTRA_DIST =

if BUILD_VOLUME_UDEV

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = volume_udev.la

volume_udev_la_SOURCES = $(SRCS_VOLUME_UDEV)

volume_udev_la_LIBADD = @ENNA_LIBS@ @UDEV_LIBS@
volume_udev_la_LDFLAGS = -module -avoid-version
volume_udev_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/volume_$(MODULE).*

endif
