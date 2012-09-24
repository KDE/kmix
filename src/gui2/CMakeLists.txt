set(kmix_KDEINIT_SRCS
   main.cpp
   KMixWindow.cpp
   KMixApp.cpp
   ControlSlider.cpp
   ControlGroupTab.cpp
)

qt4_add_dbus_interface(kmix_KDEINIT_SRCS ../dbus/org.kde.kmix.kmixd.xml
	kmixd_interface)
qt4_add_dbus_interface(kmix_KDEINIT_SRCS ../dbus/org.kde.kmix.controlgroup.xml
	controlgroup_interface)
qt4_add_dbus_interface(kmix_KDEINIT_SRCS ../dbus/org.kde.kmix.control.xml
	control_interface)

kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS} ${QT_QTXML_LIBRARY})

if (CANBERRA_FOUND)
  target_link_libraries(kdeinit_kmix ${CANBERRA_LIBRARIES})
endif (CANBERRA_FOUND)

install(TARGETS kdeinit_kmix  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY} ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix  ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( PROGRAMS kmix.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kmix_autostart.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
