if ENNA_BUILD_THEME
if BUILD_THEME_N900

MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = edje_cc

EDJE_FLAGS = \
-id $(top_srcdir)/data/theme/n900/images -id $(top_srcdir)/data/theme/default/gadgets/date/images  \
-fd $(top_srcdir)/data/theme/default/fonts

EDJE_VERB = $(EDJE_VERB_$(V))
EDJE_VERB_ = $(EDJE_VERB_$(AM_DEFAULT_VERBOSITY))
EDJE_VERB_1 = -v

defaultdir = $(pkgdatadir)/theme

default_DATA = n900.edj

IMG_ICONS = \
images/arrow_left.png \
images/arrow_right.png \
images/icon_album.png \
images/icon_artist.png \
images/icon_bookstore.png \
images/icon_config.png \
images/icon_directory.png \
images/icon_enna.png \
images/icon_freeboxtv.png \
images/icon_games.png \
images/icon_genre.png \
images/icon_gocomics.png \
images/icon_hd.png \
images/icon_home_mini.png \
images/icon_infos.png \
images/icon_library.png \
images/icon_module.png \
images/icon_music.png \
images/icon_music_mini.png \
images/icon_nofile.png \
images/icon_photo.png \
images/icon_rss.png \
images/icon_shoutcast.png \
images/icon_song.png \
images/icon_upnp.png \
images/icon_video.png \
images/icon_weather.png \
images/unknown_cover.png

IMG_ACTIVITIES = \
images/bg_bookstore.png \
images/bg_configuration.jpg \
images/bg_games.png \
images/bg_gocomics.png \
images/bg_music.jpg \
images/bg_photo.jpg \
images/bg_tv.jpg \
images/bg_video.jpg \
images/bg_weather.jpg

IMG_DEVICES = \
images/dev_bluetooth.png \
images/dev_camera.png \
images/dev_cdda2.png \
images/dev_cdda.png \
images/dev_cdrom.png \
images/dev_cdrw.png \
images/dev_dvd.png \
images/dev_hdd.png \
images/dev_ipod.png \
images/dev_mem_card.png \
images/dev_tv.png \
images/dev_nfs.png \
images/dev_photo.png \
images/dev_samba.png \
images/dev_usb_stick.png

IMG_CONTROLS = \
images/ctrl_back.png \
images/ctrl_hibernate.png \
images/ctrl_ok.png \
images/ctrl_restart.png \
images/ctrl_shutdown.png \
images/ctrl_warning.png

IMG_FILETYPES = \
images/folder_favorites.png \
images/icon_music.png \
images/icon_photo.png \
images/icon_video.png

IMG_MEDIACONTROLS = \
images/go-first.png \
images/go-last.png \
images/icon_rotate_ccw.png \
images/icon_rotate_cw.png \
images/mp_forward.png \
images/mp_info.png \
images/mp_next.png \
images/mp_pause.png \
images/mp_play.png \
images/mp_prev.png \
images/mp_rewind.png \
images/mp_stop.png

IMG_WEATHER = \
images/weather/weather_fall.png \
images/weather/weather_spring.png \
images/weather/weather_summer.png \
images/weather/weather_unknown.png \
images/weather/weather_winter.png

IMG_WEATHER_ICONS = \
images/weather/weather_cloudy.png \
images/weather/weather_foggy.png \
images/weather/weather_heavy_rain.png \
images/weather/weather_ice.png \
images/weather/weather_light_snow.png \
images/weather/weather_rain.png \
images/weather/weather_rain_storm.png \
images/weather/weather_showers.png \
images/weather/weather_snow.png \
images/weather/weather_sun_clouds.png \
images/weather/weather_sun_light_clouds.png \
images/weather/weather_sun_mostly_cloudy.png \
images/weather/weather_sunny.png \
images/weather/weather_sun_rain.png \
images/weather/weather_sun_snow.png \
images/weather/weather_sun_storm.png \
images/weather/weather_windy.png

IMG_RATINGS = \
images/rating0.png \
images/rating1.png \
images/rating2.png \
images/rating3.png \
images/rating4.png \
images/rating5.png

IMG_MOVIE_FLAGS = \
images/flags/audio/dd20.png \
images/flags/audio/dd51.png \
images/flags/audio/dd71.png \
images/flags/audio/dts51.png \
images/flags/audio/dts71.png \
images/flags/audio/mono.png \
images/flags/audio/defaultsound.png \
images/flags/media/bluray.png \
images/flags/media/divx.png \
images/flags/media/dvd.png \
images/flags/media/hdtv.png \
images/flags/media/defaultsource.png \
images/flags/studio/20thcenturyfox.png \
images/flags/studio/artisan.png \
images/flags/studio/carolco.png \
images/flags/studio/castlerock.png \
images/flags/studio/columbia.png \
images/flags/studio/default.png \
images/flags/studio/dimension.png \
images/flags/studio/disney.png \
images/flags/studio/dreamworks.png \
images/flags/studio/filmfour.png \
images/flags/studio/gaumont.png \
images/flags/studio/hollywood.png \
images/flags/studio/lionsgate.png \
images/flags/studio/lucasfilm.png \
images/flags/studio/mgm.png \
images/flags/studio/miramax.png \
images/flags/studio/newlinecinema.png \
images/flags/studio/paramount.png \
images/flags/studio/revolution.png \
images/flags/studio/screengems.png \
images/flags/studio/sonypictures.png \
images/flags/studio/studiocanal.png \
images/flags/studio/touchstone.png \
images/flags/studio/tristar.png \
images/flags/studio/unitedartists.png \
images/flags/studio/universal.png \
images/flags/studio/warner.png \
images/flags/network/abc.png \
images/flags/network/bbc.png \
images/flags/network/cartoon_network.png \
images/flags/network/cbs.png \
images/flags/network/channel4.png \
images/flags/network/citytv.png \
images/flags/network/comedy_central.png \
images/flags/network/fox.png \
images/flags/network/hbo.png \
images/flags/network/lifetime.png \
images/flags/network/living.png \
images/flags/network/nbc.png \
images/flags/network/showtime.png \
images/flags/video/1080i.png \
images/flags/video/1080p.png \
images/flags/video/720i.png \
images/flags/video/720p.png \
images/flags/video/576i.png \
images/flags/video/576p.png \
images/flags/video/540i.png \
images/flags/video/540p.png \
images/flags/video/480i.png \
images/flags/video/480p.png \
images/flags/video/defaultscreen.png \
images/flags/video/sd.png

IMG_COVERS = \
images/cover_movie.png \
images/cover_music.png \
images/cover_music2.png \
images/cover_video.png \
images/cover_video2.png

# Check which of these files really are still relevant
IMG_MISC = \
images/black_frame.png \
images/bottom_pane.png \
images/browser_cover_bg.png \
images/bt_base1.png \
images/bt_base2.png \
images/bt_base_menuitem.png \
images/bt_glow.png \
images/bt_hilight.png \
images/bt_shine_menuitem.png \
images/bt_shine.png \
images/bt_sm_base1.png \
images/bt_sm_base2.png \
images/bt_sm_hilight.png \
images/bt_sm_shine.png \
images/cd_case.png \
images/cd_case_reflect.png \
images/check.png \
images/circle_shade.png \
images/clock_base.png \
images/cover_border.png \
images/credits.jpg \
images/cur_box.png\
images/cur_glow.png\
images/cur_hi.png\
images/cur_shad.png\
images/cur_shine.png\
images/edit-redo.png \
images/edit-undo.png \
images/filebrowser_tile.png \
images/frame.png \
images/icon_playlist.png \
images/left_pane.png \
images/letter_bg.png \
images/mainmenu_border_left.png \
images/mainmenu_border_right.png \
images/menu_sel_bg.png \
images/menu_sel_fg.png \
images/mp_bt_hilight.png \
images/notifier_bg.png \
images/panel_bottom.png \
images/panel_info.png \
images/panel_left.png \
images/panel_right.png \
images/picture_bar.png \
images/pointer.png \
images/popup.png \
images/progress_bar_content.png \
images/progress_bar_drag.png \
images/progress_bar.png \
images/sb_runnerh.png \
images/sb_runnerv.png \
images/sel_bg.png \
images/sel_fg.png \
images/sel_hilight.png \
images/sel_shad.png \
images/sel_shine_black.png \
images/sel_shine_white.png \
images/separator_h.png \
images/separator_v.png \
images/shad_circ.png \
images/shadow.png \
images/shad_square.png \
images/sl_bg_over.png \
images/sl_bg.png \
images/sl_bt_0.png \
images/sl_bt_1.png \
images/sl_bt2_0_0.png \
images/sl_bt2_0_1.png \
images/sl_bt2_0_2.png \
images/sl_bt2_1.png \
images/sl_bt2_2.png \
images/sl_bt_2.png \
images/sl_units.png \
images/text_border.png \
images/toggle_bg.png \
images/toggle_button.png \
images/transition_vswipe_gap.png \
images/vertical_gradient.png \
images/video_info_panel.png \
images/volume_bg.png \
images/volume_bg_shadow.png \
images/volume_icon.png \
images/volume_icon_reflection.png \
images/volume_level_glow.png \
images/volume_level.png \
images/volume_level_shadow.png

FONTS = \
../default/fonts/content.ttf \
../default/fonts/content_bold.ttf

EDC = \
../default/activity_bookstore.edc \
../default/activity_configuration_credits.edc \
../default/activity_configuration.edc \
../default/activity_configuration_sysinfo.edc \
../default/activity_games.edc \
../default/activity_music.edc \
../default/activity_photo.edc \
../default/activity_tv.edc \
../default/activity_video.edc \
../default/activity_video_controls.edc \
../default/activity_video_flags.edc \
../default/activity_video_infos.edc \
../default/activity_weather.edc \
../default/browser.edc \
../default/cover_video.edc \
../default/default.edc \
../default/elm.edc \
../default/exit.edc \
../default/icons.edc \
../default/list.edc \
../default/mainmenu.edc \
../default/box.edc \
../default/mediacontrol.edc \
../default/mediaplayer.edc \
../default/notification_volume.edc \
../default/notification_weather.edc \
../default/picture.edc \
../default/popup.edc \
../default/slideshow.edc

EXTRA_DIST = \
  $(IMG_ICONS) \
  $(IMG_ACTIVITIES) \
  $(IMG_DEVICES) \
  $(IMG_CONTROLS) \
  $(IMG_FILETYPES) \
  $(IMG_MEDIACONTROLS) \
  $(IMG_WEATHER) \
  $(IMG_WEATHER_ICONS) \
  $(IMG_RATINGS) \
  $(IMG_MOVIE_FLAGS) \
  $(IMG_COVERS) \
  $(IMG_MISC) \
  $(FONTS) \
  $(EDC)

n900.edj: Makefile $(EXTRA_DIST)
	$(AM_V_GEN)$(EDJE_CC) $(EDJE_VERB) $(EDJE_FLAGS) \
	$(top_srcdir)/data/theme/default/default.edc \
	$(top_builddir)/data/theme/n900/n900.edj

clean-local:
	rm -f *.edj

endif
endif
