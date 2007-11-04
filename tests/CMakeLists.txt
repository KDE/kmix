set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/.. )

set(kmix_adaptor_SRCS)
qt4_add_dbus_adaptor( kmix_adaptor_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../org.kde.KMix.xml ${CMAKE_CURRENT_SOURCE_DIR}/../mixer.h Mixer)


########### next target ###############
set(profiletest_SRCS profiletest.cpp )


kde4_add_executable(profiletest ${profiletest_SRCS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../guiprofile.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixdevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../volume.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixer_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kmixdevicemanager.cpp 
    ${kmix_adaptor_SRCS}
    )

target_link_libraries(profiletest  ${KDE4_KDECORE_LIBS} ${KDE4_SOLID_LIBS})

if (HAVE_LIBASOUND2)
    target_link_libraries(profiletest ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)



########### next target ###############
set(dialogtest_KDEINIT_SRCS dialogtest.cpp )

kde4_add_kdeinit_executable( dialogtest ${dialogtest_KDEINIT_SRCS}
    )
    
target_link_libraries(kdeinit_dialogtest  ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} )


