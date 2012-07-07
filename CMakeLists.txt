project(kmix)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

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

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${CMAKE_SOURCE_DIR}/src )
if(MSVC)
    include_directories( ${TAGLIB_INCLUDES} )
endif(MSVC)


configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )


include_directories( ${GLIB2_INCLUDE_DIR} /usr/lib/oss/include)

add_subdirectory( doc )
add_subdirectory( icons ) 
add_subdirectory( pics ) 
add_subdirectory( profiles ) 
#add_subdirectory( tests )

if (PULSEAUDIO_FOUND)
  add_definitions(-DHAVE_PULSE)

  include_directories(${PULSEAUDIO_INCLUDE_DIR})
endif (PULSEAUDIO_FOUND)

if (CANBERRA_FOUND)
  add_definitions(-DHAVE_CANBERRA)

  include_directories(${CANBERRA_INCLUDE_DIRS})
endif (CANBERRA_FOUND)


set(kmix_adaptor_SRCS
	src/dbus/dbusmixerwrapper.cpp
	src/dbus/dbusmixsetwrapper.cpp
	src/dbus/dbuscontrolwrapper.cpp)
qt4_add_dbus_adaptor( kmix_adaptor_SRCS src/dbus/org.kde.kmix.control.xml
	src/dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt4_add_dbus_adaptor( kmix_adaptor_SRCS src/dbus/org.kde.kmix.mixer.xml
	src/dbus/dbusmixerwrapper.h DBusMixerWrapper )
qt4_add_dbus_adaptor( kmix_adaptor_SRCS src/dbus/org.kde.kmix.mixset.xml
	src/dbus/dbusmixsetwrapper.h DBusMixSetWrapper )

set(kmix_backend_SRCS
   src/backends/mixer_backend.cpp
   src/backends/mixer_mpris2.cpp
   )

if (HAVE_LIBASOUND2)
  set(kmix_backend_SRCS ${kmix_backend_SRCS}
      src/backends/mixer_alsa9.cpp )
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
  set(kmix_backend_SRCS ${kmix_backend_SRCS}
      src/backends/mixer_pulse.cpp )
endif (PULSEAUDIO_FOUND)

set(kmix_KDEINIT_SRCS ${kmix_adaptor_SRCS} ${kmix_backend_SRCS}
   src/main.cpp 
   src/kmix.cpp 
   src/KMixApp.cpp
   src/gui/kmixdockwidget.cpp 
   src/gui/kmixprefdlg.cpp 
   src/gui/viewbase.cpp 
   src/gui/viewdockareapopup.cpp 
   src/gui/viewsliders.cpp 
   src/gui/mixdevicewidget.cpp 
   src/gui/mdwmoveaction.cpp
   src/gui/mdwslider.cpp 
   src/gui/mdwenum.cpp 
   src/gui/kmixerwidget.cpp 
   src/gui/ksmallslider.cpp
   src/gui/verticaltext.cpp
   src/gui/volumeslider.cpp 
   src/gui/kmixtoolbox.cpp 
   src/gui/dialogaddview.cpp 
   src/gui/dialogviewconfiguration.cpp 
   src/gui/dialogselectmaster.cpp 
   src/gui/guiprofile.cpp
   src/gui/osdwidget.cpp
   src/core/mixertoolbox.cpp
   src/core/kmixdevicemanager.cpp
   src/core/ControlPool.cpp
   src/core/MasterControl.cpp
   src/core/mixer.cpp
   src/core/mixset.cpp
   src/core/mixdevice.cpp
   src/core/mixdevicecomposite.cpp
   src/core/volume.cpp
   )

kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS} ${QT_QTXML_LIBRARY})
#target_link_libraries(kdeinit_kmix ${KDE4_KUTILS_LIBS} ${KDE4_PHONON_LIBS} /home/kde/workspace/kdelibs/build/lib/libsolid.so.4.7.0 ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS} ${QT_QTXML_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
    target_link_libraries(kdeinit_kmix ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (PULSEAUDIO_FOUND)

if (CANBERRA_FOUND)
  target_link_libraries(kdeinit_kmix ${CANBERRA_LIBRARIES})
endif (CANBERRA_FOUND)

install(TARGETS kdeinit_kmix  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### next target ###############
add_subdirectory( src/plasma )

install( PROGRAMS kmix.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kmix_autostart.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
install( FILES src/dbus/org.kde.kmix.control.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
install( FILES src/dbus/org.kde.kmix.mixer.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
install( FILES src/dbus/org.kde.kmix.mixset.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

macro_display_feature_log()
