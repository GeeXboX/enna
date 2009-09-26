MAINTAINERCLEANFILES = Makefile.in

MODULE = keyb

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

if BUILD_INPUT_KEYB

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = input_keyb.la

input_keyb_la_SOURCES= \
keyb.c

input_keyb_la_LIBADD = @ENNA_LIBS@
input_keyb_la_LDFLAGS = -module -avoid-version
input_keyb_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/input_$(MODULE).*

endif
