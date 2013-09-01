add_subdirectory( engine )
install(DIRECTORY kmix-applet-qml/ DESTINATION ${DATA_INSTALL_DIR}/plasma/plasmoids/org.kde.plasma.plasma-applet-kmix)
install(FILES kmix-applet-qml/metadata.desktop DESTINATION ${SERVICES_INSTALL_DIR} RENAME org.kde.plasma.plasma-applet-kmix.desktop)
