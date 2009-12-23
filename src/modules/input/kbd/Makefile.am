include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = kbd

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

if BUILD_INPUT_KBD

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = input_kbd.la

input_kbd_la_SOURCES = $(SRCS_INPUT_KBD)

input_kbd_la_LIBADD = @ENNA_LIBS@
input_kbd_la_LDFLAGS = -module -avoid-version
input_kbd_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/input_$(MODULE).*

endif
