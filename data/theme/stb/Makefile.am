if ENNA_BUILD_THEME
if BUILD_THEME_STB

MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = edje_cc

EDJE_FLAGS = \
-id $(top_srcdir)/data/theme/stb/images -id $(top_srcdir)/data/theme/stb/gadgets/date/images\
-fd $(top_srcdir)/data/theme/stb/fonts

stbdir = $(pkgdatadir)/theme

stb_DATA = stb.edj


FONTS = \
fonts/Comfortaa_Thin.ttf \
fonts/Comfortaa_Bold.ttf \
fonts/Comfortaa_Regular.ttf

EDC = \
stb.edc 

IMGS = \
images/bg.png

EXTRA_DIST = \
	$(FONTS) \
	$(EDC) \
	$(IMGS)

stb.edj: Makefile $(EXTRA_DIST)
	$(AM_V_GEN)$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/theme/stb/stb.edc \
	$(top_builddir)/data/theme/stb/stb.edj

clean-local:
	rm -f *.edj
endif
endif
