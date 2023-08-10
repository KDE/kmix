cmake_minimum_required(VERSION 3.16)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "07")
set(RELEASE_SERVICE_VERSION_MICRO "90")
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

# For testing OSS4 build on Linux with 4Front OSS4 installed
#
# The OSS4_INCLUDES variable must point to the prototype/usr/lib/oss/include
# directory in a built (but does not have to be installed) OSS4 source tree.
set(OSS4_INCLUDES "" CACHE PATH "Include path for OSS4 include files")

####################################################################################################
########### dependencies and tests #################################################################
####################################################################################################

set(QT_MIN_VERSION "5.15.2")
set(KF5_MIN_VERSION "5.101.0")
set(PA_MIN_VERSION "0.9.16")
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
include(CheckIncludeFile)
include(CheckSourceRuns)
include(CheckStructHasMember)

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

####################################################################################################
########### sound backend support ##################################################################
####################################################################################################

# Sun Audio, only for that platform
check_include_file("sys/audioio.h" HAVE_SYS_AUDIOIO_H)
if (HAVE_SYS_AUDIOIO_H)
  message(STATUS "Building with Sun audio support")
  set(HAVE_SUN_MIXER true)
  add_definitions(-DHAVE_SUN_MIXER)
  set(SUPPORTED_BACKENDS ${SUPPORTED_BACKENDS} "Sun")
endif ()

# OSS, Linux or BSD
if (NOT (OSS4_INCLUDES STREQUAL ""))
  set(CMAKE_REQUIRED_INCLUDES ${OSS4_INCLUDES})
  set(TEST_COMPILE_FLAGS "-I${OSS4_INCLUDES}")
  message(STATUS "OSS4 includes: ${OSS4_INCLUDES}")
endif ()

check_include_file("soundcard.h" HAVE_SOUNDCARD_H)
check_include_file("sys/soundcard.h" HAVE_SYS_SOUNDCARD_H)
if (HAVE_SOUNDCARD_H OR HAVE_SYS_SOUNDCARD_H)
  add_definitions(-DHAVE_OSS_MIXER)
  set(HAVE_OSS_MIXER true)
  if (HAVE_SOUNDCARD_H)
    set(TEST_COMPILE_FLAGS "${TEST_COMPILE_FLAGS} -DHAVE_SOUNDCARD_H")
    add_definitions(-DHAVE_SOUNDCARD_H)
  endif ()
  if (HAVE_SYS_SOUNDCARD_H)
    set(TEST_COMPILE_FLAGS "${TEST_COMPILE_FLAGS} -DHAVE_SYS_SOUNDCARD_H")
    add_definitions(-DHAVE_SYS_SOUNDCARD_H)
  endif ()

  # Running a program is hopefully more reliable than trying to
  # grep through header files to find a definition.  Need try_run()
  # here so as to be able to read the output result.
  try_run(TEST_RUN_RESULT TEST_COMPILE_RESULT
          ${CMAKE_CURRENT_BINARY_DIR}
          SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ossversion.c
          COMPILE_DEFINITIONS "${TEST_COMPILE_FLAGS}"
          RUN_OUTPUT_VARIABLE TEST_RESULT_VERSION)
  #message(STATUS "TEST_RUN_RESULT= ${TEST_RUN_RESULT}")
  #message(STATUS "TEST_COMPILE_RESULT= ${TEST_COMPILE_RESULT}")
  #message(STATUS "TEST_RESULT_VERSION= ${TEST_RESULT_VERSION}")

  if (${TEST_COMPILE_RESULT} AND (${TEST_RUN_RESULT} EQUAL 0))
    message(STATUS "Detected OSS version ${TEST_RESULT_VERSION}")
    if (${TEST_RESULT_VERSION} GREATER_EQUAL 0x040000)
      message(STATUS "Building with OSS 4 support")
      set(HAVE_OSS_4 true)
      add_definitions(-DHAVE_OSS_4)
      set(SUPPORTED_BACKENDS ${SUPPORTED_BACKENDS} "OSS4")

      # This is included in 4Front OSS 4 but not in standard BSD.
      # Not possible to test at compile time.  Both of those appear
      # to have sys/soundcard.h and not soundcard.h installed.
      check_struct_has_member(oss_mixerinfo devnode sys/soundcard.h HAVE_MIXERINFO_DEVNODE)
      if (HAVE_MIXERINFO_DEVNODE)
	add_definitions(-DHAVE_MIXERINFO_DEVNODE)
      endif ()
    else ()
      message(STATUS "Building with OSS 3 support")
      set(HAVE_OSS_3 true)
      add_definitions(-DHAVE_OSS_3)
      set(SUPPORTED_BACKENDS ${SUPPORTED_BACKENDS} "OSS")
    endif ()
  else ()
    message(STATUS "Unable to get OSS version, assuming 3")
    set(HAVE_OSS_3 true)
    add_definitions(-DHAVE_OSS_3)
    set(SUPPORTED_BACKENDS ${SUPPORTED_BACKENDS} "OSS")
  endif ()
endif ()

# PulseAudio, optional
find_package(PulseAudio "${PA_MIN_VERSION}")
if (PulseAudio_FOUND)
  message(STATUS "Building with PulseAudio support")
  set(HAVE_PULSEAUDIO true)
  add_definitions(-DHAVE_PULSEAUDIO)
  set(SUPPORTED_BACKENDS ${SUPPORTED_BACKENDS} "PulseAudio")
endif ()

# ALSA, optional
find_package(ALSA)
if (ALSA_FOUND)
  message(STATUS "Building with ALSA support")
  set(HAVE_ALSA_MIXER true)
  add_definitions(-DHAVE_ALSA_MIXER)
  set(SUPPORTED_BACKENDS ${SUPPORTED_BACKENDS} "ALSA")
endif ()

message(STATUS "Supported backends: ${SUPPORTED_BACKENDS}")

# Canberra, optional for volume feedback
find_package(Canberra)
if (CANBERRA_FOUND)
  add_definitions(-DHAVE_CANBERRA)
endif ()

####################################################################################################
########### compile definitions ####################################################################
####################################################################################################

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

if (NOT (OSS4_INCLUDES STREQUAL ""))
  include_directories(BEFORE ${OSS4_INCLUDES})
endif ()

add_definitions(-DTRANSLATION_DOMAIN=\"kmix\")
add_definitions(-DKMIX_VERSION=\"${KMIX_VERSION}\")
add_definitions(-DKMIX_CONFIG_VERSION=${KMIX_CONFIG_VERSION})

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

set(kmix_backend_SRCS backends/mixerbackend.cpp backends/mixer_mpris2.cpp)

if (HAVE_ALSA_MIXER)
  set(kmix_backend_SRCS ${kmix_backend_SRCS} backends/mixer_alsa9.cpp)
endif ()

if (HAVE_PULSEAUDIO)
  set(kmix_backend_SRCS ${kmix_backend_SRCS} backends/mixer_pulse.cpp)
endif ()

if (HAVE_SUN_MIXER)
  set(kmix_backend_SRCS ${kmix_backend_SRCS} backends/mixer_sun.cpp)
endif ()

if (HAVE_OSS_3)
  set(kmix_backend_SRCS ${kmix_backend_SRCS} backends/mixer_oss.cpp)
endif ()

if (HAVE_OSS_4)
  set(kmix_backend_SRCS ${kmix_backend_SRCS} backends/mixer_oss4.cpp)
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

if (HAVE_ALSA_MIXER)
  target_link_libraries(kmixcore PRIVATE ALSA::ALSA)
endif ()

if (HAVE_PULSEAUDIO)
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
  gui/guiprofile.cpp
  gui/toggletoolbutton.cpp
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
  apps/kmixmain.cpp
  apps/kmixwindow.cpp
  apps/kmixapp.cpp
  ${kmix_debug_SRCS}
)

if (CANBERRA_FOUND)
  set(kmix_SRCS ${kmix_SRCS} apps/volumefeedback.cpp)
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
