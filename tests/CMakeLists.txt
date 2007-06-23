
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/.. )

########### next target ###############
set(profiletest_SRCS profiletest.cpp )

kde4_automoc(${profiletest_SRCS})

kde4_add_test_executable(profiletest ${profiletest_SRCS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../guiprofile.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixdevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../volume.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixer_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mixset.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../kmixadaptor.cpp
    )

target_link_libraries(profiletest  ${KDE4_KDECORE_LIBS} )

if (HAVE_LIBASOUND2)
    target_link_libraries(profiletest ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)




