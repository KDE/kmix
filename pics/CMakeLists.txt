kde4_header()




########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kmix/pics FILES mix_audio.png mix_bass.png mix_cd.png mix_ext.png  	mix_microphone.png mix_midi.png	mix_recmon.png mix_treble.png  	mix_unknown.png mix_volume.png mix_surround.png mix_video.png  	mix_headphone.png mix_digital.png mix_ac97.png  	kmixdocked.png kmixdocked_mute.png kmixdocked_error.png  	mix_record.png  	SpeakerFrontLeft.png SpeakerRearLeft.png SpeakerFrontRight.png  SpeakerRearRight.png Listener.png )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

kde4_footer()



#original Makefile.am contents follow:

#
#picsdir = $(kde_datadir)/kmix/pics
#pics_DATA = mix_audio.png mix_bass.png mix_cd.png mix_ext.png \
#	mix_microphone.png mix_midi.png	mix_recmon.png mix_treble.png \
#	mix_unknown.png mix_volume.png mix_surround.png mix_video.png \
#	mix_headphone.png mix_digital.png mix_ac97.png \
#	kmixdocked.png kmixdocked_mute.png kmixdocked_error.png \
#	mix_record.png \
#	SpeakerFrontLeft.png SpeakerRearLeft.png SpeakerFrontRight.png  SpeakerRearRight.png Listener.png
#
#
#EXTRA_DIST = $(pics_DATA) 
#
#KDE_ICON=kmix
