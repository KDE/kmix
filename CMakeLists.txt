project(kmix)

FIND_PATH( KICKER_INCLUDE_DIR plasma/kpanelapplet.h
  ${KDE4_INCLUDE_DIR}
)


add_subdirectory( pics ) 
add_subdirectory( profiles ) 



########### next target ###############

set(kmix_KDEINIT_SRCS
   mixeradaptor.cpp	
   main.cpp 
   kmix.cpp 
   kmixdockwidget.cpp 
   kmixprefdlg.cpp 
   viewbase.cpp 
   viewoutput.cpp 
   viewinput.cpp 
   viewswitches.cpp 
   viewsurround.cpp 
   viewdockareapopup.cpp 
   viewsliders.cpp 
   viewsliderset.cpp 
   mixdevicewidget.cpp 
   mdwslider.cpp 
   mdwswitch.cpp 
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

kde4_automoc(${kmix_KDEINIT_SRCS})

kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix  ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTXML_LIBRARY} kutils phononcore )

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kdeinit_kmix  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix  DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############

set(kmixctrl_KDEINIT_SRCS
   mixeradaptor.cpp
   kmixctrl.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   volume.cpp 
   mixertoolbox.cpp 
   mixer_backend.cpp 
   guiprofile.cpp )

kde4_automoc(${kmixctrl_KDEINIT_SRCS})


kde4_add_kdeinit_executable( kmixctrl ${kmixctrl_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmixctrl  ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTXML_LIBRARY} )

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmixctrl ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kdeinit_kmixctrl  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install(TARGETS kmixctrl DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############
if(KICKER_INCLUDE_DIR)

set(kmix_panelapplet_PART_SRCS
   mixeradaptor.cpp
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

kde4_automoc(${kmix_panelapplet_PART_SRCS})

kde4_add_ui3_files(kmix_panelapplet_PART_SRCS colorwidget.ui )


kde4_add_plugin(kmix_panelapplet ${kmix_panelapplet_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kmix_panelapplet )

target_link_libraries(kmix_panelapplet  ${KDE4_KDE3SUPPORT_LIBS} kutils kickermain ${QT_QTXML_LIBRARY} )

if (HAVE_LIBASOUND2)
    target_link_libraries(kmix_panelapplet ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kmix_panelapplet  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES kmixapplet.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kicker/applets )
else(KICKER_INCLUDE_DIR)
MESSAGE(STATUS "kmix applet will not compile. Install kdebase before")
endif(KICKER_INCLUDE_DIR)

########### install files ###############

install( FILES kmix.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES restore_kmix_volumes.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
install( FILES kmixctrl_restore.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


########### next target ###############

set(profiletest_SRCS tests/profiletest.cpp )

kde4_automoc(${profiletest_SRCS})

if(KDE4_BUILD_TESTS_KMIX)

kde4_add_executable(profiletest ${profiletest_SRCS} guiprofile.cpp mixer.cpp)

target_link_libraries(profiletest  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

