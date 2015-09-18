include_directories( ../../ )

set(mixer_engine_SRCS
	mixerengine.cpp
	mixerservice.cpp
)

if (KMIX_KF5_BUILD)
	qt5_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.mixset.xml
		mixset_interface)
	qt5_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.mixer.xml
		mixer_interface)
	qt5_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.control.xml
		control_interface)

	add_library(plasma_engine_mixer MODULE ${mixer_engine_SRCS})
    target_link_libraries(plasma_engine_mixer KF5::Plasma Qt5::DBus)
	kcoreaddons_desktop_to_json(plasma_engine_mixer plasma-dataengine-mixer.desktop)

	install(TARGETS plasma_engine_mixer DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
	install(FILES plasma-dataengine-mixer.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
	install(FILES mixer.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
else()
	qt4_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.mixset.xml
		mixset_interface)
	qt4_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.mixer.xml
		mixer_interface)
	qt4_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.control.xml
		control_interface)
	kde4_add_plugin(plasma_engine_mixer ${mixer_engine_SRCS})

	target_link_libraries(plasma_engine_mixer ${KDE4_KDECORE_LIBS} ${KDE4_PLASMA_LIBS})

	install(TARGETS plasma_engine_mixer DESTINATION ${PLUGIN_INSTALL_DIR})
	install(FILES plasma-dataengine-mixer.desktop DESTINATION ${SERVICES_INSTALL_DIR})
	install(FILES mixer.operations DESTINATION ${DATA_INSTALL_DIR}/plasma/services)
endif()

