project(kmix)

include_directories( ${GLIB2_INCLUDE_DIR} /usr/lib/oss/include)

add_subdirectory( pics ) 
add_subdirectory( profiles ) 
add_subdirectory( tests )

set(kmix_adaptor_SRCS)
qt4_add_dbus_adaptor( kmix_adaptor_SRCS org.kde.KMix.xml mixer.h Mixer)

set(kmix_KDEINIT_SRCS ${kmix_adaptor_SRCS}
   main.cpp 
   kmix.cpp 
   kmixdockwidget.cpp 
   kmixprefdlg.cpp 
   viewbase.cpp 
   viewdockareapopup.cpp 
   viewsliders.cpp 
   mixdevicewidget.cpp 
   mdwmoveaction.cpp
   mdwslider.cpp 
   mdwenum.cpp 
   kmixerwidget.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   mixer_backend.cpp
   ksmallslider.cpp 
   volume.cpp 
   kledbutton.cpp 
   verticaltext.cpp 
   kmixtoolbox.cpp 
   mixertoolbox.cpp 
   dialogviewconfiguration.cpp 
   KMixApp.cpp 
   dialogselectmaster.cpp 
   kmixdevicemanager.cpp 
   guiprofile.cpp
   osdwidget.cpp)

kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix ${KDE4_KUTILS_LIBS} ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS} ${QT_QTXML_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (HAVE_PULSE)
    target_link_libraries(kdeinit_kmix ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (HAVE_PULSE)


install(TARGETS kdeinit_kmix  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

 set(kded_kmixd_SRCS ${kmix_adaptor_SRCS}
    kmixd.cpp 
    mixer.cpp 
    mixset.cpp 
    mixdevice.cpp 
    volume.cpp
    mixertoolbox.cpp 
    mixer_backend.cpp 
    kmixdevicemanager.cpp
#    guiprofile.cpp
    )
# 
# # guiprofile.cpp should NOT be in kmixd. !!!
# # guiprofile stuff must be moved from mixertoolbox to kmixtoolbox!!! 

#qt4_add_dbus_adaptor(kded_kmixd_SRCS org.kde.KMixD.xml kmixd.h Mixer) 
 
kde4_add_plugin(kded_kmixd ${kded_kmixd_SRCS})


target_link_libraries(kded_kmixd ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kded_kmixd ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (HAVE_PULSE)
    target_link_libraries(kded_kmixd ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (HAVE_PULSE)

install(TARGETS kded_kmixd DESTINATION ${PLUGIN_INSTALL_DIR})

#target_link_libraries( kmixd kded_kmixd )
#install(TARGETS kmixd DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES kmixd.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kded )

########### next target ###############

set(kmixctrl_KDEINIT_SRCS ${kmix_adaptor_SRCS}
   kmixctrl.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   volume.cpp 
   mixertoolbox.cpp 
   mixer_backend.cpp 
   kmixdevicemanager.cpp 
   guiprofile.cpp )



kde4_add_kdeinit_executable( kmixctrl ${kmixctrl_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmixctrl ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmixctrl ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (HAVE_PULSE)
    target_link_libraries(kdeinit_kmixctrl ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (HAVE_PULSE)

install(TARGETS kdeinit_kmixctrl  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install(TARGETS kmixctrl ${INSTALL_TARGETS_DEFAULT_ARGS} )


install( FILES kmix.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES restore_kmix_volumes.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmix_autostart.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
install( FILES kmixctrl_restore.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES org.kde.KMix.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
