include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = photo

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

EXTRA_DIST = \
photo.h \
photo_infos.h \
photo_slideshow_view.h

if BUILD_ACTIVITY_PHOTO

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_photo.la

activity_photo_la_SOURCES = $(SRCS_ACTIVITY_PHOTO)

activity_photo_la_LIBADD = @ENNA_LIBS@
activity_photo_la_LDFLAGS = -module -avoid-version
activity_photo_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
