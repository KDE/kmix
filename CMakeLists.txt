cmake_minimum_required(VERSION 2.8.12)
project(kmix)

add_definitions( -DTRANSLATION_DOMAIN=\"kmix\" )

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") 

set (QT_MIN_VERSION "5.4.0")
set (KF5_MIN_VERSION "5.14.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMPackageConfigHelpers)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS
    Core
    DBus
    Gui
    Widgets
    Xml
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Completion
    Config
    ConfigWidgets
    Crash
    DBusAddons
    DocTools
    GlobalAccel
    I18n
    IconThemes
    KDELibs4Support
    Notifications
    Plasma
    WidgetsAddons
    WindowSystem
    XmlGui
)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})


# Do not yet REQUIRE Phonon. Hint: As long as we do not find_package(), ${KDE4_PHONON_LIBS} will be empty below, but that should not hurt.
#find_package(Phonon REQUIRED)

# PulseAudio is an optional dependency
set(PA_VER "0.9.16")
find_package(PulseAudio "${PA_VER}")

find_package(GLIB2)

# Canberra is an optional dependency
find_package(Canberra)

find_package(Alsa)
if(ALSA_FOUND)
	alsa_configure_file(${CMAKE_BINARY_DIR}/config-alsa.h)
	add_definitions(-DHAVE_LIBASOUND2)
endif(ALSA_FOUND)

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

add_subdirectory(doc)

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


ecm_qt_declare_logging_category(kmix_debug_SRCS
                                HEADER kmix_debug.h
                                IDENTIFIER KMIX_LOG
                                CATEGORY_NAME org.kde.kmix)

####################################################################################################
########### definitions: DBUSADAPTOR ###############################################################
####################################################################################################
set(kmix_adaptor_SRCS
	dbus/dbusmixerwrapper.cpp
	dbus/dbusmixsetwrapper.cpp
	dbus/dbuscontrolwrapper.cpp)
qt5_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.control.xml
	dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt5_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixer.xml
	dbus/dbusmixerwrapper.h DBusMixerWrapper )
qt5_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixset.xml
	dbus/dbusmixsetwrapper.h DBusMixSetWrapper )


####################################################################################################
########### definitions: backends ##################################################################
####################################################################################################
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



####################################################################################################
########### next target : kmix #####################################################################
####################################################################################################

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
   ${kmix_OSD_SRCS}
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
   ${kmix_debug_SRCS}
   )

kf5_add_kdeinit_executable(kmix ${kmix_KDEINIT_SRCS})
target_link_libraries(kdeinit_kmix
    KF5::I18n
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::ConfigWidgets
    KF5::IconThemes
    KF5::DBusAddons
    KF5::GlobalAccel
    KF5::KDELibs4Support

    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    Qt5::Xml
)


if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
    target_link_libraries(kdeinit_kmix ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (PULSEAUDIO_FOUND)

if (CANBERRA_FOUND)
  target_link_libraries(kdeinit_kmix ${CANBERRA_LIBRARIES})
endif (CANBERRA_FOUND)

install(TARGETS kdeinit_kmix DESTINATION ${KDE_INSTALL_LIBDIR} )

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})




####################################################################################################
########### next target : kmixd ####################################################################
####################################################################################################

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
    ${kmix_debug_SRCS}
    )

#qt4_add_dbus_adaptor(kded_kmixd_SRCS org.kde.KMixD.xml kmixd.h Mixer) 

add_library(kded_kmixd ${kded_kmixd_SRCS})


target_link_libraries(kded_kmixd
    KF5::I18n
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::Solid
    KF5::KDELibs4Support
    KF5::DBusAddons

    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    Qt5::Xml

    ${KDEx_KDEUI_LIBS}
    ${KDEx_PHONON_LIBS}
)


if (HAVE_LIBASOUND2)
    target_link_libraries(kded_kmixd ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (PULSEAUDIO_FOUND)
    target_link_libraries(kded_kmixd ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${GLIB2_LIBRARIES})
endif (PULSEAUDIO_FOUND)

if (CANBERRA_FOUND)
  target_link_libraries(kded_kmixd ${CANBERRA_LIBRARIES})
endif (CANBERRA_FOUND)

install(TARGETS kded_kmixd DESTINATION ${KDE_INSTALL_PLUGINDIR})

#target_link_libraries( kmixd kded_kmixd )
#install(TARGETS kmixd DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES kmixd.desktop  DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kded )



####################################################################################################
########### next target : kmixctrl #################################################################
####################################################################################################

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
   ${kmix_debug_SRCS}
   # There is no actual need for kmixdevicemanager.cpp (hotplug makes no sense in kmixctrl), but
   # the dependency comes in via mixertoolbox.cpp
   core/kmixdevicemanager.cpp
   )

# gui/guiprofile.cpp


KF5_ADD_KDEINIT_EXECUTABLE(kmixctrl ${kmixctrl_KDEINIT_SRCS})
target_link_libraries(kdeinit_kmixctrl
    KF5::I18n
    KF5::ConfigCore
    KF5::KDELibs4Support

    Qt5::Core
#Qt5::DBus Qt5::Xml
)

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
add_subdirectory( plasma )


####################################################################################################
########### INSTALL RULES ##########################################################################
####################################################################################################
install( TARGETS kdeinit_kmixctrl  DESTINATION ${KDE_INSTALL_LIBDIR} )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install( TARGETS kmixctrl ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install( PROGRAMS org.kde.kmix.desktop  DESTINATION ${KDE_INSTALL_APPDIR} )
install( PROGRAMS apps/kmixremote DESTINATION ${KDE_INSTALL_BINDIR} )
install( FILES restore_kmix_volumes.desktop  DESTINATION ${KDE_INSTALL_AUTOSTARTDIR} )
install( FILES kmix_autostart.desktop  DESTINATION ${KDE_INSTALL_AUTOSTARTDIR} )
install( FILES kmixui.rc  DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/kmix )
install( FILES kmixctrl_restore.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES dbus/org.kde.kmix.control.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
install( FILES dbus/org.kde.kmix.mixer.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
install( FILES dbus/org.kde.kmix.mixset.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
