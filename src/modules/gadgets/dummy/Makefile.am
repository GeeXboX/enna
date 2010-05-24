include ./config.mak

MAINTAINERCLEANFILES = Makefile.in

MODULE = dummy

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

if BUILD_GADGET_DUMMY

pkgdir = $(libdir)/enna/modules

pkg_LTLIBRARIES = gadget_dummy.la

gadget_dummy_la_SOURCES = $(SRCS_GADGET_DUMMY)

gadget_dummy_la_LIBADD = @ENNA_LIBS@
gadget_dummy_la_LDFLAGS = -module -avoid-version
gadget_dummy_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enna/modules/input_$(MODULE).*

endif
