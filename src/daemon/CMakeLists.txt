set(kmixd_SRCS
    main.cpp
    KMixDApp.cpp
    Backend.cpp
    BackendManager.cpp
    ControlGroup.cpp
    Control.cpp
)

if (HAVE_LIBASOUND2)
    set(kmixd_SRCS ${kmixd_SRCS}
        backends/ALSA.cpp
        backends/ALSAControl.cpp
    )
endif()

if (PULSEAUDIO_FOUND)
    set(kmixd_SRCS ${kmixd_SRCS}
        backends/PulseAudio.cpp
        backends/PulseControl.cpp
        backends/PulseSourceOutputControl.cpp
        backends/PulseSinkInputControl.cpp
        backends/PulseSinkControl.cpp
        backends/PulseSourceControl.cpp
    )
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.kmixd.xml
    KMixDApp.h KMixDApp )
qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.control.xml
    Control.h Control )
qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.controlgroup.xml
    ControlGroup.h ControlGroup )
qt4_add_dbus_interface(kmixd_SRCS ../dbus/org.kde.kmix.controlmonitor.xml
	controlmonitor_interface)

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

configure_file(org.kde.kmixd.service.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmixd.service)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmixd.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR} )
