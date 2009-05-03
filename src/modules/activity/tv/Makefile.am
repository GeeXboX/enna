MAINTAINERCLEANFILES = Makefile.in

MODULE = tv

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

if BUILD_LIBSVDRP
INCLUDES += @LIBSVDRP_CFLAGS@
endif

if BUILD_ACTIVITY_TV

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = activity_tv.la

activity_tv_la_SOURCES= \
tv.c

activity_tv_la_LIBADD = @ENNA_LIBS@
activity_tv_la_LDFLAGS = -module -avoid-version
activity_tv_la_DEPENDENCIES = $(top_builddir)/config.h

if BUILD_LIBSVDRP
activity_tv_la_LIBADD += @LIBSVDRP_LIBS@
endif

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/activity_$(MODULE).*

endif
