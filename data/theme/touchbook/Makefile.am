if ENNA_BUILD_THEME

MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = edje_cc

EDJE_FLAGS = \
-id $(top_srcdir)/data/theme/touchbook/images

EDJE_VERB = $(EDJE_VERB_$(V))
EDJE_VERB_ = $(EDJE_VERB_$(AM_DEFAULT_VERBOSITY))
EDJE_VERB_1 = -v

defaultdir = $(pkgdatadir)/theme

default_DATA = touchbook.edj

EDC = \
touchbook.edc


EXTRA_DIST = \
  $(EDC)

touchbook.edj: Makefile $(EXTRA_DIST)
	$(AM_V_GEN)$(EDJE_CC) $(EDJE_VERB) $(EDJE_FLAGS) \
	$(top_srcdir)/data/theme/touchbook/touchbook.edc \
	$(top_builddir)/data/theme/touchbook/touchbook.edj

clean-local:
	rm -f *.edj

endif
