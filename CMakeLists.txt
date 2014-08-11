project(kmix)

# CMake build file for KMix. It supports both KDE4 and KF5.
# 
# KDE4 support: complete
# KF5 support is in work. A build will be triggered by adding "-DKMIX_KF5_BUILD=1" to the cmake call.
#       Status: Build fails on kmixdevicemanager.cpp , which does #include <solid/audiointerface.h>
#               Unfortunately Solid currently has no Audiointerface in KF5, thus it pulls the #include from
#               KDE4, which leads to error: ‘AudioInterface’ is not a member of ‘Solid::DeviceInterface’
#
# Next step: Talk to Alex Fiestas how to progress

if(POLICY CMP0046)
  cmake_policy (SET CMP0046 NEW)
endif()

# Your project should require at least CMake 2.8.12 to use FindKF5.cmake
if (KMIX_KF5_BUILD)
   cmake_minimum_required(VERSION 2.8.12)
else ()
   cmake_minimum_required(VERSION 2.8.11)
endif()

if(POLICY CMP0046)
  cmake_policy (SET CMP0046 NEW)
endif()




if (KMIX_KF5_BUILD)
find_package(ECM 0.0.10 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
#include(KF5Init/KF5InitMacros)
#include(MacroLibrary)
# I need KF5KDELibs4Support at least for find_package(GLIB2), for KDebug and eventually for Alsa (vs ALSA)
find_package(KF5 REQUIRED COMPONENTS I18n Init Config Solid KDELibs4Support)
# find_package(KF5Solid NO_MODULE) : added according to http://api.kde.org/frameworks-api/frameworks5-apidocs/solid/html/index.html
find_package(KF5Solid NO_MODULE)
find_package(Qt5 REQUIRED COMPONENTS Core DBus Gui Widgets)
#include(KF5InitMacros)

else()
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

endif()



# Do not yet REQUIRE Phonon. Hint: As long as we do not find_package(), ${KDE4_PHONON_LIBS} will be empty below, but that should not hurt.
#find_package(Phonon REQUIRED)



set(PA_VER "0.9.16")
if (KMIX_KF5_BUILD)
find_package(PulseAudio "${PA_VER}")
else()
macro_optional_find_package(PulseAudio "${PA_VER}")
macro_log_feature(PULSEAUDIO_FOUND "PulseAudio" "PulseAudio Audio Server"
    "http://www.pulseaudio.org/" FALSE "${PA_VER}" "libpulse is needed to let KMix control PulseAudio")
endif()

find_package(GLIB2)

if (KMIX_KF5_BUILD)
find_package(Canberra)
else()
macro_optional_find_package(Canberra)
macro_log_feature(CANBERRA_FOUND "libcanberra" "libcanberra audio library"
    "http://0pointer.de/lennart/projects/libcanberra/" FALSE "" "libcanberra is needed for kmix sound feedback")
endif()

#if (KMIX_KF5_BUILD)
#find_package(ALSA)
#else()
find_package(Alsa)
alsa_configure_file(${CMAKE_BINARY_DIR}/config-alsa.h)
#endif()


add_definitions (${QT_DEFINITIONS} ${QT_QTDBUS_DEFINITIONS} ${KDE4_DEFINITIONS} )
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=67100)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})
if(MSVC)
    include_directories( ${TAGLIB_INCLUDES} )
endif(MSVC)


include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <memory>
    int main() { std::shared_ptr<int> p; return 0; }
" HAVE_STD_SHARED_PTR)
check_cxx_source_compiles("
    #include <tr1/memory>
    int main() { std::tr1::shared_ptr<int> p; return 0; }
" HAVE_STD_TR1_SHARED_PTR)


configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )


include_directories( ${GLIB2_INCLUDE_DIR} /usr/lib/oss/include)

if (KMIX_KF5_BUILD)
   # TODO Add warning about porting kde4_create_handbook
   message( WARNING, "Skipping doc directory due to missing kde4_create_handbook" )
else()  
  add_subdirectory( doc )
endif()

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

#if (KMIX_KF5_BUILD)
#   message( WARNING, "Skipping kmix_adaptor_SRCS due to missing qt4_add_dbus_adaptor  ... KMix will NOT compile until this is fixed!!!" )

set(kmix_adaptor_SRCS
	dbus/dbusmixerwrapper.cpp
	dbus/dbusmixsetwrapper.cpp
	dbus/dbuscontrolwrapper.cpp)
if (KMIX_KF5_BUILD)
qt5_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.control.xml
	dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt5_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixer.xml
	dbus/dbusmixerwrapper.h DBusMixerWrapper )
qt5_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixset.xml
	dbus/dbusmixsetwrapper.h DBusMixSetWrapper )
else()  
qt4_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.control.xml
	dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt4_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixer.xml
	dbus/dbusmixerwrapper.h DBusMixerWrapper )
qt4_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixset.xml
	dbus/dbusmixsetwrapper.h DBusMixSetWrapper )
endif()


set(kmix_backend_SRCS
   backends/mixer_backend.cpp
   backends/mixer_mpris2.cpp
   )

if (HAVE_LIBASOUND2)
  set(kmix_backend_SRCS ${kmix_backend_SRCS}
      backends/mixer_alsa9.cpp )
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
  set(kmix_backend_SRCS ${kmix_backend_SRCS}
      backends/mixer_pulse.cpp )
endif (PULSEAUDIO_FOUND)

set(kmix_KDEINIT_SRCS ${kmix_adaptor_SRCS} ${kmix_backend_SRCS}
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
   gui/volumeslider.cpp 
   gui/kmixtoolbox.cpp 
   gui/dialogaddview.cpp 
   gui/dialogviewconfiguration.cpp 
   gui/dialogselectmaster.cpp 
   gui/dialogchoosebackends.cpp 
   gui/guiprofile.cpp
   gui/osdwidget.cpp
   core/MediaController.cpp
   core/mixertoolbox.cpp
   core/kmixdevicemanager.cpp
   core/ControlManager.cpp
#   core/ControlPool.cpp
   core/GlobalConfig.cpp
   core/MasterControl.cpp
   core/mixer.cpp
   core/mixset.cpp
   core/mixdevice.cpp
   core/mixdevicecomposite.cpp
   core/volume.cpp
   )

if (KMIX_KF5_BUILD)
#   message( WARNING, "Skipping kdeinit_executable due to missing KF5_ADD_KDEINIT_EXECUTABLE  ... KMix will NOT compile until this is fixed!!!" )
   kf5_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})
  #target_link_libraries( KF5::UI KF5::Plasma  Qt5:XML KF5::Phonon)
   target_link_libraries(kdeinit_kmix KF5::I18n KF5::ConfigCore KF5::ConfigGui KF5::Solid KF5::KDELibs4Support Qt5::Core Qt5::Widgets Qt5::DBus Qt5::Xml)
else()
  kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})
  target_link_libraries(kdeinit_kmix ${KDE4_SOLID_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_PHONON_LIBS})
endif()


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

 set(kded_kmixd_SRCS ${kmix_adaptor_SRCS} ${kmix_backend_SRCS}
    apps/kmixd.cpp 
    core/ControlManager.cpp
#    core/ControlPool.cpp
    core/GlobalConfig.cpp
    core/MasterControl.cpp
    core/MediaController.cpp
    core/mixer.cpp 
    core/mixset.cpp 
    core/mixdevice.cpp 
    core/volume.cpp
    core/mixertoolbox.cpp 
    core/kmixdevicemanager.cpp
    )

#qt4_add_dbus_adaptor(kded_kmixd_SRCS org.kde.KMixD.xml kmixd.h Mixer) 

if (KMIX_KF5_BUILD)
  add_library(kded_kmixd ${kded_kmixd_SRCS})
else()
  kde4_add_plugin(kded_kmixd ${kded_kmixd_SRCS})
endif()


if (KMIX_KF5_BUILD)
	target_link_libraries(kded_kmixd KF5::I18n KF5::ConfigCore KF5::ConfigGui KF5::Solid KF5::KDELibs4Support Qt5::Core Qt5::Widgets Qt5::DBus Qt5::Xml ${KDEx_KDEUI_LIBS} ${KDEx_PHONON_LIBS})
else()
	target_link_libraries(kded_kmixd ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_PHONON_LIBS})
endif()


if (HAVE_LIBASOUND2)
    target_link_libraries(kded_kmixd ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
    target_link_libraries(kded_kmixd ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (PULSEAUDIO_FOUND)

if (CANBERRA_FOUND)
  target_link_libraries(kded_kmixd ${CANBERRA_LIBRARIES})
endif (CANBERRA_FOUND)

install(TARGETS kded_kmixd DESTINATION ${PLUGIN_INSTALL_DIR})

#target_link_libraries( kmixd kded_kmixd )
#install(TARGETS kmixd DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES kmixd.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kded )

########### next target ###############

set(kmixctrl_KDEINIT_SRCS ${kmix_adaptor_SRCS} ${kmix_backend_SRCS}
   apps/kmixctrl.cpp 
   core/ControlManager.cpp
#   core/ControlPool.cpp
   core/GlobalConfig.cpp
   core/MasterControl.cpp
   core/MediaController.cpp
   core/mixer.cpp 
   core/mixset.cpp 
   core/mixdevice.cpp 
   core/volume.cpp 
   core/mixertoolbox.cpp 
   core/kmixdevicemanager.cpp
   )

# gui/guiprofile.cpp


if (KMIX_KF5_BUILD)
  KF5_ADD_KDEINIT_EXECUTABLE( kmixctrl ${kmixctrl_KDEINIT_SRCS})
  target_link_libraries(kdeinit_kmixctrl QtCore ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_PHONON_LIBS})
else()
  kde4_add_kdeinit_executable( kmixctrl ${kmixctrl_KDEINIT_SRCS})
  target_link_libraries(kdeinit_kmixctrl ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_PHONON_LIBS})
endif()

target_link_libraries(kdeinit_kmixctrl ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_PHONON_LIBS})

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmixctrl ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
    target_link_libraries(kdeinit_kmixctrl ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (PULSEAUDIO_FOUND)

if (CANBERRA_FOUND)
  target_link_libraries(kdeinit_kmixctrl ${CANBERRA_LIBRARIES})
endif (CANBERRA_FOUND)

########### next target ###############
if (!KMIX_KF5_BUILD)
  add_subdirectory( plasma )
endif()

install( TARGETS kdeinit_kmixctrl  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install( TARGETS kmixctrl ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( PROGRAMS kmix.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( PROGRAMS apps/kmixremote DESTINATION ${BIN_INSTALL_DIR} )
install( FILES restore_kmix_volumes.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmix_autostart.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
install( FILES kmixctrl_restore.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES dbus/org.kde.kmix.control.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
install( FILES dbus/org.kde.kmix.mixer.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
install( FILES dbus/org.kde.kmix.mixset.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

if (KMIX_KF5_BUILD)
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES) 
else()
  macro_display_feature_log()
endif()
