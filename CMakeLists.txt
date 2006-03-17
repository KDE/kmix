kde4_header()
project(kmix)

add_subdirectory( pics ) 
add_subdirectory( tests ) 
add_subdirectory( profiles ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kmix_KDEINIT_SRCS 
   main.cpp 
   kmix.cpp 
   kmixdockwidget.cpp 
   kmixprefdlg.cpp 
   viewbase.cpp 
   viewoutput.cpp 
   viewinput.cpp 
   viewswitches.cpp 
   viewsurround.cpp 
   viewdockareapopup.cpp 
   viewsliders.cpp 
   viewsliderset.cpp 
   mixdevicewidget.cpp 
   mdwslider.cpp 
   mdwswitch.cpp 
   mdwenum.cpp 
   kmixerwidget.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   mixer_backend.cpp 
   ksmallslider.cpp 
   volume.cpp 
   kledbutton.cpp 
   verticaltext.cpp 
   kmixtoolbox.cpp 
   mixertoolbox.cpp 
   dialogviewconfiguration.cpp 
   KMixApp.cpp 
   dialogselectmaster.cpp 
   guiprofile.cpp )

kde4_automoc(${kmix_KDEINIT_SRCS})

kde4_add_dcop_skels(kmix_KDEINIT_SRCS mixerIface.h )

kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix  ${LIB_KDE3SUPPORT} ${QT_QTXML_LIBRARY} kutils )

install_targets(${LIB_INSTALL_DIR} kdeinit_kmix )

target_link_libraries( kmix kdeinit_kmix )
install_targets(/bin kmix )

########### next target ###############

set(kmixctrl_KDEINIT_SRCS 
   kmixctrl.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   volume.cpp 
   mixertoolbox.cpp 
   mixer_backend.cpp 
   guiprofile.cpp )

kde4_automoc(${kmixctrl_KDEINIT_SRCS})

kde4_add_dcop_skels(kmixctrl_KDEINIT_SRCS mixerIface.h )

kde4_add_kdeinit_executable( kmixctrl ${kmixctrl_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmixctrl  ${LIB_KDE3SUPPORT} ${QT_QTXML_LIBRARY} )

install_targets(${LIB_INSTALL_DIR} kdeinit_kmixctrl )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install_targets(/bin kmixctrl )

########### next target ###############

set(kmix_panelapplet_PART_SRCS 
   kmixapplet.cpp 
   viewbase.cpp 
   viewapplet.cpp 
   mixdevicewidget.cpp 
   mdwslider.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   mixer_backend.cpp 
   ksmallslider.cpp 
   volume.cpp 
   kledbutton.cpp 
   verticaltext.cpp 
   dialogviewconfiguration.cpp 
   kmixtoolbox.cpp 
   mixertoolbox.cpp 
   dialogselectmaster.cpp 
   guiprofile.cpp )

kde4_automoc(${kmix_panelapplet_PART_SRCS})

kde4_add_ui_files(kmix_panelapplet_PART_SRCS colorwidget.ui )

kde4_add_dcop_skels(kmix_panelapplet_PART_SRCS mixerIface.h )

kde4_add_plugin(kmix_panelapplet ${kmix_panelapplet_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kmix_panelapplet )

target_link_libraries(kmix_panelapplet  ${LIB_KDE3SUPPORT} kutils kickermain ${QT_QTXML_LIBRARY} )

install_targets(${LIB_INSTALL_DIR}/kde4 kmix_panelapplet )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kmix.desktop )
install_files( /share/autostart FILES restore_kmix_volumes.desktop )
install_files( ${DATA_INSTALL_DIR}/kmix FILES kmixui.rc )
install_files( ${SERVICES_INSTALL_DIR} FILES kmixctrl_restore.desktop )
install_files( ${DATA_INSTALL_DIR}/kicker/applets FILES kmixapplet.desktop )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = pics tests profiles
#INCLUDES= $(all_includes)
#
#bin_PROGRAMS = 
#lib_LTLIBRARIES = 
#kdeinit_LTLIBRARIES = kmix.la kmixctrl.la
#kde_module_LTLIBRARIES = kmix_panelapplet.la
#
#
#noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h \
#	viewbase.h viewoutput.h viewinput.h viewsliders.h viewswitches.h viewsurround.h viewdockareapopup.h \
#	viewsliderset.h \
#	mixdevicewidget.h mdwslider.h mdwswitch.h mdwenum.h \
#	mixer.h mixset.h mixdevice.h mixer_backend.h volume.h kledbutton.h version.h kmixtoolbox.h \
#	ksmallslider.h kmixapplet.h mixerIface.h verticaltext.h \
#	KMixApp.h dialogviewconfiguration.h kmixtoolbox.h mixertoolbox.h dialogselectmaster.h \
#	guiprofile.h
#
#
#METASOURCES = AUTO
#
#
#kmix_la_SOURCES = main.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp \
#	viewbase.cpp viewoutput.cpp viewinput.cpp viewswitches.cpp viewsurround.cpp viewdockareapopup.cpp \
#	viewsliders.cpp viewsliderset.cpp \
#	mixdevicewidget.cpp mdwslider.cpp mdwswitch.cpp mdwenum.cpp \
#	kmixerwidget.cpp mixer.cpp mixset.cpp mixdevice.cpp mixer_backend.cpp ksmallslider.cpp \
#	volume.cpp kledbutton.cpp verticaltext.cpp mixerIface.skel \
#	kmixtoolbox.cpp mixertoolbox.cpp dialogviewconfiguration.cpp KMixApp.cpp dialogselectmaster.cpp \
#	guiprofile.cpp
#
#kmix_la_LIBADD = $(LIB_KDEUI) $(LIB_KUTILS) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
#kmix_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -avoid-version
#
#
#kmixctrl_la_SOURCES = kmixctrl.cpp mixer.cpp mixset.cpp mixdevice.cpp volume.cpp mixerIface.skel \
#	mixertoolbox.cpp mixer_backend.cpp guiprofile.cpp
#
#kmixctrl_la_LIBADD = $(LIB_KDECORE) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
#kmixctrl_la_LDFLAGS = $(all_libraries) -module -avoid-version
#
#
#kmix_panelapplet_la_SOURCES = kmixapplet.cpp \
#	viewbase.cpp viewapplet.cpp \
#	mixdevicewidget.cpp mdwslider.cpp \
#	mixer.cpp mixset.cpp mixdevice.cpp mixer_backend.cpp ksmallslider.cpp volume.cpp kledbutton.cpp \
#	verticaltext.cpp mixerIface.skel colorwidget.ui dialogviewconfiguration.cpp \
#	kmixtoolbox.cpp mixertoolbox.cpp dialogselectmaster.cpp \
#	guiprofile.cpp
#
#kmix_panelapplet_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
#kmix_panelapplet_la_LIBADD = $(LIB_KDEUI) $(LIB_KUTILS) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND) -lkickermain
#
#xdg_apps_DATA = kmix.desktop
#
#rcdir = $(kde_datadir)/kmix
#rc_DATA = kmixui.rc
#
#autostart_DATA = restore_kmix_volumes.desktop
#autostartdir = $(datadir)/autostart
#
#lnkdir = $(kde_datadir)/kicker/applets
#lnk_DATA = kmixapplet.desktop
#
#servicesdir = $(kde_servicesdir)
#services_DATA = kmixctrl_restore.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kmix.pot
#
