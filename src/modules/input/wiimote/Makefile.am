include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = wiimote

# data files for the module
filesdir = $(libdir)/enna/modules/

# the module .so file
INCLUDES = \
-I. \
-I$(top_srcdir) \
-I$(top_srcdir)/src/modules/$(MODULE) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/modules \
@ENNA_CFLAGS@ @LIBBLUETOOTH_CFLAGS@

if BUILD_INPUT_WIIMOTE


pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = input_wiimote.la

input_wiimote_la_SOURCES = $(SRCS_INPUT_WIIMOTE)

input_wiimote_la_LIBADD = @ENNA_LIBS@ @LIBBLUETOOTH_LIBS@ @LIBCWIID_LIBS@
input_wiimote_la_LDFLAGS = -module -avoid-version
input_wiimote_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/input_$(MODULE).*

endif
