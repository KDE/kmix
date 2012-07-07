set(kmixd_SRCS
    main.cpp
    KMixDApp.cpp
	../dbus/dbusmixsetwrapper.cpp
	../dbus/dbusmixerwrapper.cpp
	../dbus/dbuscontrolwrapper.cpp
    ../backends/mixer_backend.cpp
    ../core/mixertoolbox.cpp
    ../core/kmixdevicemanager.cpp
    ../core/ControlPool.cpp
    ../core/MasterControl.cpp
    ../core/mixer.cpp
    ../core/mixset.cpp
    ../core/mixdevice.cpp
    ../core/mixdevicecomposite.cpp
    ../core/volume.cpp
    ../backends/mixer_alsa9.cpp
    ../backends/mixer_mpris2.cpp
)

qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.mixset.xml
	../dbus/dbusmixsetwrapper.h DBusMixSetWrapper )
qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.control.xml
	../dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt4_add_dbus_adaptor( kmixd_SRCS ../dbus/org.kde.kmix.mixer.xml
	../dbus/dbusmixerwrapper.h DBusMixerWrapper )

kde4_add_executable(kmixd ${kmixd_SRCS})

target_link_libraries(kmixd ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${ASOUND_LIBRARY})
