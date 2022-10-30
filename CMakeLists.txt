cmake_minimum_required(VERSION 3.16)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "22")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "3")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

set(KMIX_VERSION ${RELEASE_SERVICE_VERSION})
project(kmix VERSION ${KMIX_VERSION})
# SOVERSION of the kmixcore library
set(KMIX_VERSION_MAJOR 5)
# Version of the configuration, originally from core/version.h
set(KMIX_CONFIG_VERSION 3)

####################################################################################################
########### options ################################################################################
####################################################################################################

# Plasma dataengines are apparently deprecated and will be removed in Plasma 6
option(BUILD_DATAENGINE "Build the Plasma dataengine" OFF)

####################################################################################################
########### dependencies and tests #################################################################
####################################################################################################

set (QT_MIN_VERSION "5.15.0")
set (KF5_MIN_VERSION "5.90.0")
set (PA_MIN_VERSION "0.9.16")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} )
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(GenerateExportHeader)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS
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
    Notifications
    Solid
    WidgetsAddons
    WindowSystem
    XmlGui
)

if (BUILD_DATAENGINE)
  find_package(KF5Plasma ${KF5_MIN_VERSION} REQUIRED)
  set_package_properties(KF5Plasma PROPERTIES PURPOSE "Required to build the Plasma dataengine")
endif ()

# PulseAudio is an optional dependency
find_package(PulseAudio "${PA_MIN_VERSION}")

# Canberra is an optional dependency
find_package(Canberra)

# ALSA is an optional dependency
find_package(ALSA)

####################################################################################################
########### compile definitions ####################################################################
####################################################################################################

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

add_definitions(-DTRANSLATION_DOMAIN=\"kmix\")
add_definitions(-DKMIX_VERSION=\"${KMIX_VERSION}\")
add_definitions(-DKMIX_CONFIG_VERSION=${KMIX_CONFIG_VERSION})

if (ALSA_FOUND)
  set(HAVE_LIBASOUND2 TRUE)
  add_definitions(-DHAVE_LIBASOUND2)
endif ()

if (PulseAudio_FOUND)
  add_definitions(-DHAVE_PULSE)
endif ()

if (CANBERRA_FOUND)
  add_definitions(-DHAVE_CANBERRA)
endif ()

####################################################################################################
########### subdirectories #########################################################################
####################################################################################################

add_subdirectory(doc)
add_subdirectory(pics)
add_subdirectory(profiles)

if (BUILD_DATAENGINE)
  if (KF5Plasma_FOUND)
    add_subdirectory(plasma)
  endif ()
endif ()

####################################################################################################
########### definitions: logging ###################################################################
####################################################################################################

ecm_qt_declare_logging_category(kmix_debug_SRCS
                                HEADER kmix_debug.h
                                IDENTIFIER KMIX_LOG
                                EXPORT kmixlogging
                                DESCRIPTION "KMix sound mixer and volume control"
                                CATEGORY_NAME org.kde.kmix)

####################################################################################################
########### definitions: DBus adaptor ##############################################################
####################################################################################################

set(kmix_adaptor_SRCS
  dbus/dbusmixerwrapper.cpp
  dbus/dbusmixsetwrapper.cpp
  dbus/dbuscontrolwrapper.cpp
)

qt_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.control.xml
	dbus/dbuscontrolwrapper.h DBusControlWrapper )
qt_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixer.xml
	dbus/dbusmixerwrapper.h DBusMixerWrapper )
qt_add_dbus_adaptor( kmix_adaptor_SRCS dbus/org.kde.kmix.mixset.xml
	dbus/dbusmixsetwrapper.h DBusMixSetWrapper )

install(FILES dbus/org.kde.kmix.control.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
install(FILES dbus/org.kde.kmix.mixer.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
install(FILES dbus/org.kde.kmix.mixset.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

####################################################################################################
########### definitions: backends ##################################################################
####################################################################################################

set(kmix_backend_SRCS
  backends/mixer_backend.cpp
  backends/mixer_mpris2.cpp
)

if (HAVE_LIBASOUND2)
  set(kmix_backend_SRCS ${kmix_backend_SRCS} backends/mixer_alsa9.cpp backends/mixer_alsa9.h)
endif ()

if (PulseAudio_FOUND)
  set(kmix_backend_SRCS ${kmix_backend_SRCS} backends/mixer_pulse.cpp backends/mixer_pulse.h)
endif ()

####################################################################################################
########### target: kmixcore library ###############################################################
####################################################################################################

set(kmixcore_SRCS
  core/MediaController.cpp
  core/mixertoolbox.cpp
  core/kmixdevicemanager.cpp
  core/ControlManager.cpp
  core/MasterControl.cpp
  core/mixer.cpp
  core/mixset.cpp
  core/mixdevice.cpp
  core/mixdevicecomposite.cpp
  core/volume.cpp
  core/MediaController.h
  core/mixertoolbox.h
  core/kmixdevicemanager.h
  core/ControlManager.h
  core/MasterControl.h
  core/mixer.h
  core/mixset.h
  core/mixdevice.h
  core/mixdevicecomposite.h
  core/volume.h
)

kconfig_add_kcfg_files(kmixcore_SRCS core/settings.kcfgc)

add_library(kmixcore SHARED
  ${kmixcore_SRCS}
  ${kmix_adaptor_SRCS}
  ${kmix_backend_SRCS}
  ${kmix_debug_SRCS}
)

target_link_libraries(kmixcore
  PUBLIC
    Qt::Core
    Qt::Widgets
  PRIVATE
    Qt::Xml
    Qt::DBus
    KF5::I18n
    KF5::Solid
  PUBLIC
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::CoreAddons
)

set_target_properties(kmixcore PROPERTIES VERSION ${KMIX_VERSION} SOVERSION ${KMIX_VERSION_MAJOR})
generate_export_header(kmixcore BASE_NAME kmixcore EXPORT_FILE_NAME kmixcore_export.h)

if (HAVE_LIBASOUND2)
  target_link_libraries(kmixcore PRIVATE ALSA::ALSA)
endif ()

if (PulseAudio_FOUND)
  target_link_libraries(kmixcore PRIVATE PulseAudio::PulseAudio)
endif ()

if (CANBERRA_FOUND)
  # VolumeFeedback calls Canberra directly, so public linking is required
  target_link_libraries(kmixcore PUBLIC Canberra::Canberra)
endif ()

install(TARGETS kmixcore DESTINATION ${KDE_INSTALL_LIBDIR} LIBRARY NAMELINK_SKIP)
install(FILES core/settings.kcfg RENAME kmixsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

####################################################################################################
########### target: kmixgui library ################################################################
####################################################################################################

set(kmixgui_SRCS
  gui/dialogbase.cpp
  gui/dialogstatesaver.cpp
  gui/kmixdockwidget.cpp
  gui/kmixprefdlg.cpp
  gui/viewbase.cpp
  gui/viewdockareapopup.cpp
  gui/viewsliders.cpp
  gui/mixdevicewidget.cpp
  gui/mdwslider.cpp
  gui/mdwenum.cpp
  gui/kmixerwidget.cpp
  gui/verticaltext.cpp
  gui/volumeslider.cpp
  gui/kmixtoolbox.cpp
  gui/dialogaddview.cpp
  gui/dialogviewconfiguration.cpp
  gui/dialogselectmaster.cpp
  gui/dialogchoosebackends.cpp
  gui/guiprofile.cpp
  gui/toggletoolbutton.cpp
  gui/dialogbase.h
  gui/dialogstatesaver.h
  gui/kmixdockwidget.h
  gui/kmixprefdlg.h
  gui/viewbase.h
  gui/viewdockareapopup.h
  gui/viewsliders.h
  gui/mixdevicewidget.h
  gui/mdwslider.h
  gui/mdwenum.h
  gui/kmixerwidget.h
  gui/verticaltext.h
  gui/volumeslider.h
  gui/kmixtoolbox.h
  gui/dialogaddview.h
  gui/dialogviewconfiguration.h
  gui/dialogselectmaster.h
  gui/dialogchoosebackends.h
  gui/guiprofile.h
  gui/toggletoolbutton.h
)

add_library(kmixgui STATIC
  ${kmixgui_SRCS}
  ${kmix_debug_SRCS}
)

target_link_libraries(kmixgui
  kmixcore
  Qt::Core
  Qt::Widgets
  KF5::I18n
  KF5::ConfigCore
  KF5::ConfigGui
  KF5::GlobalAccel
  KF5::Notifications
  KF5::XmlGui
  KF5::WindowSystem
)

####################################################################################################
########### target: kmix ###########################################################################
####################################################################################################

set(kmix_SRCS
  apps/main.cpp
  apps/kmixwindow.cpp
  apps/kmixapp.cpp
  apps/kmixwindow.h
  apps/kmixapp.h
  ${kmix_debug_SRCS}
)

if (CANBERRA_FOUND)
  set(kmix_SRCS ${kmix_SRCS} apps/volumefeedback.cpp apps/volumefeedback.h)
endif ()

add_executable(kmix ${kmix_SRCS})
target_link_libraries(kmix
  kmixcore
  kmixgui
  KF5::I18n
  KF5::CoreAddons
  KF5::DBusAddons
  KF5::GlobalAccel
  KF5::XmlGui
  KF5::Notifications
  KF5::WindowSystem
)

install(TARGETS kmix ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES desktop/kmixui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kmix)
install(PROGRAMS desktop/org.kde.kmix.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES desktop/org.kde.kmix.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES desktop/kmix_autostart.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install(FILES desktop/kmix.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )

####################################################################################################
########### target: kded_kmixd #####################################################################
####################################################################################################

set(kmixd_SRCS
  apps/kmixd.cpp
  ${kmix_debug_SRCS}
)

add_library(kded_kmixd MODULE ${kmixd_SRCS})
set_target_properties(kded_kmixd PROPERTIES OUTPUT_NAME kmixd)
kcoreaddons_desktop_to_json(kded_kmixd desktop/kmixd.desktop)

target_link_libraries(kded_kmixd
  kmixcore
  KF5::I18n
  KF5::CoreAddons
  KF5::DBusAddons
)

install(TARGETS kded_kmixd DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)

####################################################################################################
########### target: kmixctrl #######################################################################
####################################################################################################

set(kmixctrl_SRCS
  apps/kmixctrl.cpp
  ${kmix_debug_SRCS}
)

add_executable(kmixctrl ${kmixctrl_SRCS})
target_link_libraries(kmixctrl
  kmixcore
  KF5::I18n
  KF5::CoreAddons
)

install(TARGETS kmixctrl ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES desktop/kmixctrl_restore.desktop DESTINATION ${KDE_INSTALL_KSERVICESDIR})

####################################################################################################
########### tests ##################################################################################
####################################################################################################

if(BUILD_TESTING)
  find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)
  add_subdirectory(tests)
endif()

####################################################################################################
########### other installs #########################################################################
####################################################################################################

install(PROGRAMS apps/kmixremote DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES desktop/restore_kmix_volumes.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

ecm_qt_install_logging_categories(EXPORT kmixlogging
                                  FILE kmix.categories
                                  DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}")

ki18n_install(po)
kdoctools_install(po)

####################################################################################################
########### end ####################################################################################
####################################################################################################

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
