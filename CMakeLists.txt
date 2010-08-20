project(kmix)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
find_package(Alsa)

set(PULSEAUDIO_MINIMUM_VERSION "0.9.12")
macro_optional_find_package(PulseAudio)
macro_log_feature(PULSEAUDIO_FOUND "PulseAudio" "PulseAudio Audio Server" "http://www.pulseaudio.org/" FALSE "0.9.12" "libpulse is needed to let KMix control PulseAudio")
alsa_configure_file(${CMAKE_BINARY_DIR}/config-alsa.h)


add_definitions (${QT_DEFINITIONS} ${QT_QTDBUS_DEFINITIONS} ${KDE4_DEFINITIONS} )
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})
if(MSVC)
    include_directories( ${TAGLIB_INCLUDES} )
endif(MSVC)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

macro_display_feature_log()

include_directories( ${GLIB2_INCLUDE_DIR} /usr/lib/oss/include)

add_subdirectory( pics ) 
add_subdirectory( profiles ) 
add_subdirectory( tests )

set(kmix_adaptor_SRCS)
qt4_add_dbus_adaptor( kmix_adaptor_SRCS org.kde.KMix.xml core/mixer.h Mixer)

set(kmix_KDEINIT_SRCS ${kmix_adaptor_SRCS}
   apps/main.cpp 
   apps/kmix.cpp 
   apps/KMixApp.cpp
   gui/kmixdockwidget.cpp 
   gui/kmixprefdlg.cpp 
   gui/viewbase.cpp 
   gui/viewdockareapopup.cpp 
   gui/viewsliders.cpp 
   gui/mixdevicewidget.cpp 
   gui/mdwmoveaction.cpp
   gui/mdwslider.cpp 
   gui/mdwenum.cpp 
   gui/kmixerwidget.cpp 
   gui/ksmallslider.cpp
   gui/verticaltext.cpp
   gui/kmixtoolbox.cpp 
   gui/dialogviewconfiguration.cpp 
   gui/dialogselectmaster.cpp 
   gui/guiprofile.cpp
   gui/osdwidget.cpp
   core/mixertoolbox.cpp
   core/kmixdevicemanager.cpp
   core/mixer.cpp
   core/mixset.cpp
   core/mixdevice.cpp
   core/mixdevicecomposite.cpp
   core/volume.cpp
   backends/mixer_backend.cpp
   )

kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix ${KDE4_KUTILS_LIBS} ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS} ${QT_QTXML_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (HAVE_PULSE)
    target_link_libraries(kdeinit_kmix ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (HAVE_PULSE)


install(TARGETS kdeinit_kmix  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

 set(kded_kmixd_SRCS ${kmix_adaptor_SRCS}
    apps/kmixd.cpp 
    core/mixer.cpp 
    core/mixset.cpp 
    core/mixdevice.cpp 
    core/volume.cpp
    core/mixertoolbox.cpp 
    core/kmixdevicemanager.cpp
    backends/mixer_backend.cpp 
    )

#qt4_add_dbus_adaptor(kded_kmixd_SRCS org.kde.KMixD.xml kmixd.h Mixer) 
 
kde4_add_plugin(kded_kmixd ${kded_kmixd_SRCS})


target_link_libraries(kded_kmixd ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kded_kmixd ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (HAVE_PULSE)
    target_link_libraries(kded_kmixd ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (HAVE_PULSE)

install(TARGETS kded_kmixd DESTINATION ${PLUGIN_INSTALL_DIR})

#target_link_libraries( kmixd kded_kmixd )
#install(TARGETS kmixd DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES kmixd.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kded )

########### next target ###############

set(kmixctrl_KDEINIT_SRCS ${kmix_adaptor_SRCS}
   apps/kmixctrl.cpp 
   core/mixer.cpp 
   core/mixset.cpp 
   core/mixdevice.cpp 
   core/volume.cpp 
   core/mixertoolbox.cpp 
   core/kmixdevicemanager.cpp 
   backends/mixer_backend.cpp
   )

# gui/guiprofile.cpp


kde4_add_kdeinit_executable( kmixctrl ${kmixctrl_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmixctrl ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmixctrl ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (HAVE_PULSE)
    target_link_libraries(kdeinit_kmixctrl ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (HAVE_PULSE)

install(TARGETS kdeinit_kmixctrl  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install(TARGETS kmixctrl ${INSTALL_TARGETS_DEFAULT_ARGS} )


install( FILES kmix.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES restore_kmix_volumes.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmix_autostart.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
install( FILES kmixctrl_restore.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES org.kde.KMix.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
