
picsdir = $(kde_datadir)/kmix/pics
pics_DATA = mix_audio.xpm mix_bass.xpm mix_cd.xpm mix_ext.xpm \
	mix_microphone.xpm mix_midi.xpm	mix_recmon.xpm mix_treble.xpm \
	mix_unknown.xpm mix_volume.xpm kmixdocked.xpm

install-data-local:
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) kmix_mini.xpm $(kde_minidir)/kmix.xpm

uninstall-local:
	-rm -f $(kde_minidir)/kmix.xpm

icon_DATA = kmix.xpm
icondir = $(kde_icondir)

EXTRA_DIST = $(icon_DATA) $(pics_DATA) kmix_mini.xpm

