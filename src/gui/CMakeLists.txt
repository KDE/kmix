set(kmix_KDEINIT_SRCS
   main.cpp
   kmix.cpp
   KMixApp.cpp
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
   ksmallslider.cpp
   verticaltext.cpp
   volumeslider.cpp 
   kmixtoolbox.cpp 
   dialogaddview.cpp 
   dialogviewconfiguration.cpp 
   dialogselectmaster.cpp 
   guiprofile.cpp
   osdwidget.cpp
	../dbus/dbusmixsetwrapper.cpp
	../dbus/dbusmixerwrapper.cpp
	../dbus/dbuscontrolwrapper.cpp
   ../core/mixertoolbox.cpp
   ../core/kmixdevicemanager.cpp
   ../core/ControlPool.cpp
   ../core/MasterControl.cpp
   ../core/mixer.cpp
   ../core/mixset.cpp
   ../core/mixdevice.cpp
   ../core/mixdevicecomposite.cpp
   ../core/volume.cpp
   ../backends/mixer_backend.cpp
   ../backends/mixer_mpris2.cpp
   ../backends/mixer_alsa9.cpp
   ../backends/mixer_pulse.cpp
)

qt4_add_dbus_adaptor( kmix_KDEINIT_SRCS ../dbus/org.kde.kmix.control.xml
	../dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt4_add_dbus_adaptor( kmix_KDEINIT_SRCS ../dbus/org.kde.kmix.mixer.xml
	../dbus/dbusmixerwrapper.h DBusMixerWrapper )
qt4_add_dbus_adaptor( kmix_KDEINIT_SRCS ../dbus/org.kde.kmix.mixset.xml
	../dbus/dbusmixsetwrapper.h DBusMixSetWrapper )

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
