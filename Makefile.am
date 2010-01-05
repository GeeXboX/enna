## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = po src data

EXTRA_DIST = config.rpath enna.cfg.sample $(ennadoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

update-pot:
	$(MAKE) -C po $(AM_MAKEFLAGS) @PACKAGE@.pot-update

update-po:
	$(MAKE) -C po $(AM_MAKEFLAGS) update-po

update-gmo:
	$(MAKE) -C po $(AM_MAKEFLAGS) update-gmo

force-update-gmo:
	touch po/*.po
	$(MAKE) -C po $(AM_MAKEFLAGS) update-gmo

force-update-gmo-%:
	touch po/`echo $@ | sed s/force-update-gmo-//`.po
	$(MAKE) -C po $(AM_MAKEFLAGS) update-gmo

ACLOCAL_AMFLAGS = -I m4
.PHONY: update-pot update-po update-gmo force-update-gmo
