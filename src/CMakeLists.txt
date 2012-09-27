configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

add_subdirectory( daemon )
add_subdirectory( gui )
add_subdirectory( dbus )
