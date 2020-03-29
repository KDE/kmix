



########### install files ###############

install( FILES
    mixer-pcm.png
    mixer-lfe.png
    mixer-cd.png
    mixer-line.png
    mixer-microphone.png
    mixer-midi.png
    mixer-capture.png
    mixer-pcm-default.png
    mixer-front.png
    mixer-master.png
    mixer-surround.png
    mixer-video.png
    mixer-headset.png
    mixer-digital.png
    mixer-ac97.png
    show-mixer.png
    DESTINATION  ${KDE_INSTALL_DATADIR}/kmix/pics )

ecm_install_icons(ICONS
    16-actions-kmix.png
    32-actions-kmix.png
    48-actions-kmix.png
    64-actions-kmix.png
    128-actions-kmix.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
