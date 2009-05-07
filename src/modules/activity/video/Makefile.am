MAINTAINERCLEANFILES = Makefile.in

MODULE = video

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

if BUILD_ACTIVITY_VIDEO

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_video.la

activity_video_la_SOURCES= \
video.c\
backdrop.c\
backdrop.h\
panel_infos.c\
panel_infos.h

activity_video_la_LIBADD = @ENNA_LIBS@
activity_video_la_LDFLAGS = -module -avoid-version
activity_video_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
