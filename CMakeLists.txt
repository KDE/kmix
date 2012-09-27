project(kmix)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

set(KMIX_VERSION_MAJOR 4)
set(KMIX_VERSION_MINOR 2)
set(KMIX_VERSION_PATCH 0)
set(KMIX_VERSION "${KMIX_VERSION_MAJOR}.${KMIX_VERSION_MINOR}.${KMIX_VERSION_PATCH}")

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

find_package(Alsa)

macro_optional_find_package(PulseAudio "0.9.12")
macro_log_feature(PULSEAUDIO_FOUND "PulseAudio" "PulseAudio Audio Server" "http://www.pulseaudio.org/" FALSE "0.9.12" "libpulse is needed to let KMix control PulseAudio")
find_package(GLIB2)
macro_optional_find_package(Canberra)
macro_log_feature(CANBERRA_FOUND "libcanberra" "libcanberra audio library" "http://0pointer.de/lennart/projects/libcanberra/" FALSE "" "libcanberra is needed for kmix sound feedback")

alsa_configure_file(${CMAKE_BINARY_DIR}/config-alsa.h)

add_definitions (${QT_DEFINITIONS} ${QT_QTDBUS_DEFINITIONS} ${KDE4_DEFINITIONS} )
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=67100)

include_directories (${KDE4_INCLUDES})

if(MSVC)
    include_directories( ${TAGLIB_INCLUDES} )
endif(MSVC)


configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

include_directories( ${GLIB2_INCLUDE_DIR} /usr/lib/oss/include)

if (PULSEAUDIO_FOUND)
  add_definitions(-DHAVE_PULSE)

  include_directories(${PULSEAUDIO_INCLUDE_DIR})
endif (PULSEAUDIO_FOUND)

if (CANBERRA_FOUND)
  add_definitions(-DHAVE_CANBERRA)

  include_directories(${CANBERRA_INCLUDE_DIRS})
endif (CANBERRA_FOUND)

add_subdirectory( doc )
add_subdirectory( icons ) 
add_subdirectory( pics ) 
add_subdirectory( profiles ) 
add_subdirectory( src )
#add_subdirectory( tests )


macro_display_feature_log()
