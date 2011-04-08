include_directories( ../../ )

set(mixer_engine_SRCS
	mixerengine.cpp
	mixerservice.cpp
)

qt4_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.mixset.xml
	mixset_interface)
qt4_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.mixer.xml
	mixer_interface)
qt4_add_dbus_interface(mixer_engine_SRCS ../../dbus/org.kde.kmix.control.xml
	control_interface)

kde4_add_plugin(plasma_engine_mixer ${mixer_engine_SRCS})
target_link_libraries(plasma_engine_mixer ${KDE4_KDECORE_LIBS}
	${KDE4_PLASMA_LIBS})

install(TARGETS plasma_engine_mixer DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-engine-mixer.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES mixer.operations DESTINATION
	${DATA_INSTALL_DIR}/plasma/services)
