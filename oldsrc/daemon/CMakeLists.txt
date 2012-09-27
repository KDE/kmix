
set(kmixd_backend_SRCS
   ../backends/mixer_backend.cpp
   ../backends/mixer_mpris2.cpp
   )

if (HAVE_LIBASOUND2)
  set(kmixd_backend_SRCS ${kmixd_backend_SRCS}
      ../backends/mixer_alsa9.cpp )
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
  set(kmixd_backend_SRCS ${kmixd_backend_SRCS}
      ../backends/mixer_pulse.cpp )
endif (PULSEAUDIO_FOUND)

set(kmixd_SRCS
    main.cpp
    KMixDApp.cpp
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
)

qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.mixset.xml
	../dbus/dbusmixsetwrapper.h DBusMixSetWrapper )
qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.control.xml
	../dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.mixer.xml
	../dbus/dbusmixerwrapper.h DBusMixerWrapper )

kde4_add_executable(kmixd ${kmixd_SRCS} ${kmixd_backend_SRCS})

target_link_libraries(kmixd
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_SOLID_LIBS}
)

if (HAVE_LIBASOUND2)
    target_link_libraries(kmixd ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
    target_link_libraries(kmixd ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (PULSEAUDIO_FOUND)

install( TARGETS kmixd ${INSTALL_TARGETS_DEFAULT_ARGS} )

configure_file(org.kde.kmix.service.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmix.service)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmix.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR} )
