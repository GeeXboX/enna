## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 

AM_CFLAGS =\
	@ECORE_CFLAGS@ \
	@EVAS_CFLAGS@ \
	@EDJE_CFLAGS@ \
	@EMOTION_CFLAGS@ \
	@EDBUS_CFLAGS@\
	@SQLITE3_CFLAGS@ \
	@LIGHTMEDIASCANNER_CFLAGS@\
	@MUSICBRAINZ_CFLAGS@

bin_PROGRAMS = enna

enna_SOURCES = \
	enna.h\
	enna.c\
	enna_inc.h\
	enna_config.c\
	enna_config.h\
	enna_util.c\
	enna_util.h\
	enna_scanner.c\
	enna_scanner.h\
	enna_event.c\
	enna_event.h\
	enna_widget.c\
	enna_widget.h\
	enna_box.c\
	enna_box.h\
	enna_mainmenu.c\
	enna_mainmenu.h\
	enna_background.c\
	enna_background.h\
	enna_module.c\
	enna_module.h

enna_LDADD =  	@EVAS_LIBS@ \
		@ECORE_LIBS@ \
		@EDJE_LIBS@ \
		@EDBUS_LIBS@ \
		@EMOTION_LIBS@\
		@LIGHTMEDIASCANNER_LIBS@\
		@SQLITE3_LIBS@\
		@MUSICBRAINZ_LIBS@

