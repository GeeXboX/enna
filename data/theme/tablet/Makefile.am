if ENNA_BUILD_THEME
if BUILD_THEME_TABLET

MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = edje_cc

EDJE_FLAGS = \
-id $(top_srcdir)/data/theme/tablet/images  -id $(top_srcdir)/data/theme/tablet/gadgets/date/images

EDJE_VERB = $(EDJE_VERB_$(V))
EDJE_VERB_ = $(EDJE_VERB_$(AM_DEFAULT_VERBOSITY))
EDJE_VERB_1 = -v

defaultdir = $(pkgdatadir)/theme

default_DATA = tablet.edj

EDC = \
box.edc \
elm_inwin.edc \
elm_label.edc \
exit.edc      \
tablet.edc

IMGS = \
images/inwin_bg.png

EXTRA_DIST = \
  $(EDC) \
  $(IMGS)

tablet.edj: Makefile $(EXTRA_DIST)
	$(AM_V_GEN)$(EDJE_CC) $(EDJE_VERB) $(EDJE_FLAGS) \
	$(top_srcdir)/data/theme/tablet/tablet.edc \
	$(top_builddir)/data/theme/tablet/tablet.edj

clean-local:
	rm -f *.edj

endif
endif
