set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/.. )

set(kmix_adaptor_SRCS)
qt4_add_dbus_adaptor( kmix_adaptor_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/../dbus/org.kde.KMix.Mixer.xml
	${CMAKE_CURRENT_SOURCE_DIR}/../dbus/dbusmixerwrapper.cpp DBusMixerWrapper)


########### next target ###############
set(profiletest_SRCS profiletest.cpp )


kde4_add_executable(profiletest ${profiletest_SRCS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/guiprofile.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/MasterControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/mixer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/mixdevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/volume.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/mixer_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/mixset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/kmixdevicemanager.cpp 
    ${kmix_adaptor_SRCS}
    )

target_link_libraries(profiletest  ${KDE4_KDECORE_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} )

if (HAVE_LIBASOUND2)
    target_link_libraries(profiletest ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
    target_link_libraries(profiletest ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (PULSEAUDIO_FOUND)

########### next target ###############
set(dialogtest_KDEINIT_SRCS dialogtest.cpp )

kde4_add_kdeinit_executable( dialogtest ${dialogtest_KDEINIT_SRCS}
    )
    
target_link_libraries(kdeinit_dialogtest  ${KDE4_KDEUI_LIBS} )


