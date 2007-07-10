project(kmix)

find_package(Plasma)

add_subdirectory( pics ) 
add_subdirectory( profiles ) 
add_subdirectory( tests ) 

set(kmix_adaptor_SRCS)
qt4_add_dbus_adaptor( kmix_adaptor_SRCS org.kde.KMix.xml mixer.h Mixer)
########### next target ###############

# Cleanup
#   viewsurround.cpp for now
#   mdwswitch.cpp 
#   viewoutput.cpp 
#   viewinput.cpp 
#   viewswitches.cpp 
#   viewsliderset.cpp 

set(kmix_KDEINIT_SRCS ${kmix_adaptor_SRCS}
   main.cpp 
   kmix.cpp 
   kmixdockwidget.cpp 
   kmixprefdlg.cpp 
   viewbase.cpp 
   viewdockareapopup.cpp 
   viewsliders.cpp 
   mixdevicewidget.cpp 
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
   guiprofile.cpp )



kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix  ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_KUTILS_LIBS} ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS} ${QT_QT3SUPPORT_LIBRARY} )

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kdeinit_kmix  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix  DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############

set(kmixctrl_KDEINIT_SRCS ${kmix_adaptor_SRCS}
   kmixctrl.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   volume.cpp 
   mixertoolbox.cpp 
   mixer_backend.cpp 
   guiprofile.cpp )



kde4_add_kdeinit_executable( kmixctrl ${kmixctrl_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmixctrl  ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_SOLID_LIBS} ${QT_QT3SUPPORT_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmixctrl ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kdeinit_kmixctrl  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install(TARGETS kmixctrl DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############

set(kmixd_KMIXD_SRCS
   kmixd.cpp
   kmixdevicemanager.cpp )



kde4_add_kdeinit_executable( kmixd ${kmixd_KMIXD_SRCS})

target_link_libraries(kdeinit_kmixd  ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_SOLID_LIBS} ${QT_QT3SUPPORT_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmixd ${ASOUND_LIBRARY} ${KDE4_SOLID_LIBS})
endif (HAVE_LIBASOUND2)

install(TARGETS kdeinit_kmixd DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmixd kdeinit_kmixd )
install(TARGETS kmixd DESTINATION ${BIN_INSTALL_DIR} )


########### next target ###############

if(Q_WS_X11)
if(PLASMA_FOUND)

set(kmix_panelapplet_PART_SRCS ${kmix_adaptor_SRCS}
   kmixapplet.cpp 
   viewbase.cpp 
   viewapplet.cpp 
   mixdevicewidget.cpp 
   mdwslider.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   mixer_backend.cpp 
   ksmallslider.cpp 
   volume.cpp 
   kledbutton.cpp 
   verticaltext.cpp 
   dialogviewconfiguration.cpp 
   kmixtoolbox.cpp 
   mixertoolbox.cpp 
   dialogselectmaster.cpp 
   guiprofile.cpp )


kde4_add_ui_files(kmix_panelapplet_PART_SRCS colorwidget.ui )


kde4_add_plugin(kmix_panelapplet ${kmix_panelapplet_PART_SRCS})



target_link_libraries(kmix_panelapplet  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} kickermain ${QT_QTXML_LIBRARY} )

if (HAVE_LIBASOUND2)
    target_link_libraries(kmix_panelapplet ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kmix_panelapplet  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES kmixapplet.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kicker/applets )
else(KICKER_INCLUDE_DIR)
MESSAGE(STATUS "kmix applet will not compile. Install kdebase before")
endif(PLASMA_FOUND)
endif(Q_WS_X11)

########### install files ###############

install( FILES kmix.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES restore_kmix_volumes.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
install( FILES kmixctrl_restore.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES org.kde.KMix.xml DESTINATION ${DBUS_INTERFACES_DIR} )





