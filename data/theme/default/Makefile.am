if ENNA_BUILD_THEME

MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = edje_cc

EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/theme/default/images \
-fd $(top_srcdir)/data/theme/default/fonts

defaultdir = $(pkgdatadir)/theme

default_DATA = default.edj

EXTRA_DIST = \
images/filebrowser_tile.png \
images/bt_base_menuitem.png \
images/transition_vswipe_gap.png \
images/icon_freeboxtv.png \
images/weather_rain_storm.png \
images/icon_genre.png \
images/dev_hdd.png \
images/circle_shade.png \
images/bt_sm_shine.png \
images/weather_light_snow.png \
images/icon_rss.png \
images/icon_playlist.png \
images/weather_sun_rain.png \
images/icon_music_mini.png \
images/rating4.png \
images/mp_pause.png \
images/dev_cdda2.png \
images/dev_bluetooth.png \
images/video_info_panel.png \
images/icon_hd.png \
images/volume_bg.png \
images/volume_level_shadow.png \
images/ctrl_restart.png \
images/ctrl_ok.png \
images/rating3.png \
images/icon_video.png \
images/weather_foggy.png \
images/bt_glow.png \
images/mp_next.png \
images/loading-4.png \
images/popup.png \
images/rating1.png \
images/volume_level_glow.png \
images/dev_photo.png \
images/weather_unknown.png \
images/volume_icon.png \
images/mp_stop.png \
images/icon_artist.png \
images/unknown_cover.png \
images/icon_directory.png \
images/default_backdrop.png \
images/icon_weather.png \
images/ctrl_back.png \
images/pointer.png \
images/flags/video/576i.png \
images/flags/video/sd.png \
images/flags/video/540i.png \
images/flags/video/480i.png \
images/flags/video/720i.png \
images/flags/video/1080p.png \
images/flags/video/1080i.png \
images/flags/video/720p.png \
images/flags/video/576p.png \
images/flags/video/540p.png \
images/flags/video/480p.png \
images/flags/media/bluray.png \
images/flags/media/hddvd.png \
images/flags/media/divx.png \
images/flags/media/dvd.png \
images/flags/media/hdtv.png \
images/flags/studio/dreamworks.png \
images/flags/studio/lionsgate.png \
images/flags/studio/screengems.png \
images/flags/studio/20thcenturyfox.png \
images/flags/studio/gaumont.png \
images/flags/studio/dimension.png \
images/flags/studio/miramax.png \
images/flags/studio/tristar.png \
images/flags/studio/mgm.png \
images/flags/studio/columbia.png \
images/flags/studio/warner.png \
images/flags/studio/carolco.png \
images/flags/studio/castlerock.png \
images/flags/studio/hollywood.png \
images/flags/studio/unitedartists.png \
images/flags/studio/sonypictures.png \
images/flags/studio/studiocanal.png \
images/flags/studio/paramount.png \
images/flags/studio/disney.png \
images/flags/studio/lucasfilm.png \
images/flags/studio/revolution.png \
images/flags/studio/universal.png \
images/flags/studio/newlinecinema.png \
images/flags/studio/touchstone.png \
images/flags/studio/filmfour.png \
images/flags/studio/artisan.png \
images/flags/audio/dd51.png \
images/flags/audio/dts71.png \
images/flags/audio/dolby20.png \
images/flags/audio/dolbydigital.png \
images/flags/audio/dd71.png \
images/flags/audio/dd20.png \
images/flags/audio/mono.png \
images/flags/audio/dts51.png \
images/flags/audio/dts.png \
images/background.png \
images/mp_forward.png \
images/mp_rewind.png \
images/dev_ipod.png \
images/mainmenu_background.png \
images/mainmenu_border_left.png \
images/mainmenu_border_right.png \
images/ctrl_hibernate.png \
images/dev_cdrw.png \
images/dev_tv.png \
images/weather_sun_clouds.png \
images/weather_sun_snow.png \
images/menu_sel_bg.png \
images/loading-7.png \
images/icon_enna.png \
images/icon_games.png \
images/rating2.png \
images/loading-0.png \
images/bt_sm_base2.png \
images/icon_home_mini.png \
images/weather_sun_mostly_cloudy.png \
images/cd_case.png \
images/folder_favorites.png \
images/loading-1.png \
images/weather_heavy_rain.png \
images/weather_windy.png \
images/title_bar.png \
images/loading-5.png \
images/volume_icon_reflection.png \
images/progress_bar_drag.png \
images/bt_shine_menuitem.png \
images/weather_sun_storm.png \
images/progress_bar_content.png \
images/weather_sunny.png \
images/bt_shine.png \
images/icon_library.png \
images/rating5.png \
images/letter_bg.png \
images/loading-6.png \
images/weather_summer.jpeg \
images/weather_sun_light_clouds.png \
images/bt_hilight.png \
images/dev_camera.png \
images/icon_infos.png \
images/mp_prev.png \
images/panel_bottom.png \
images/loading-3.png \
images/icon_upnp.png \
images/cd_case_reflect.png \
images/weather_cloudy.png \
images/dev_mem_card.png \
images/browser_cover_bg.png \
images/icon_album.png \
images/dev_samba.png \
images/arrow_left.png \
images/sel_fg.png \
images/bt_sm_hilight.png \
images/rating0.png \
images/weather_showers.png \
images/progress_bar.png \
images/ctrl_warning.png \
images/bt_base1.png \
images/dev_usb_stick.png \
images/weather_fall.jpeg \
images/volume_bg_shadow.png \
images/sb_runnerv.png \
images/bt_sm_base1.png \
images/icon_tv.png \
images/dev_cdda.png \
images/icon_nofile.png \
images/dev_dvd.png \
images/weather_rain.png \
images/weather_winter.jpeg \
images/panel_left.png \
images/panel_right.png \
images/text_border.png \
images/volume_level.png \
images/bt_base2.png \
images/icon_shoutcast.png \
images/dev_cdrom.png \
images/panel_info.png \
images/weather_ice.png \
images/ctrl_shutdown.png \
images/mp_play.png \
images/weather_spring.jpeg \
images/icon_photo.png \
images/menu_sel_fg.png \
images/sb_runnerh.png \
images/weather_snow.png \
images/shadow.png \
images/icon_song.png \
images/frame.png \
images/icon_music.png \
images/clock_base.png \
images/dev_nfs.png \
images/icon_config.png \
images/loading-2.png \
images/go-first.png \
images/go-last.png \
images/icon_rotate_cw.png \
images/icon_rotate_ccw.png \
images/picture_bar.png \
images/icon_module.png \
images/check.png \
fonts/content.ttf \
fonts/content_bold.ttf \
default.edc \
music_video.edc \
video_info.edc \
mediaplayer.edc \
mainmenu.edc \
mediacontrol.edc \
picture.edc \
weather.edc \
browser.edc \
exit.edc \
elm.edc \
infos.edc \
slideshow.edc \
list.edc \
video.edc \
popup.edc \
volume.edc \
games.edc \
icons.edc \
cover_video.edc \
photo.edc \
configuration.edc



default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/theme/default/default.edc \
	$(top_builddir)/data/theme/default/default.edj

clean-local:
	rm -f *.edj

endif
