project(kmix)

FIND_PATH( KICKER_INCLUDE_DIR plasma/kpanelapplet.h
  ${KDE4_INCLUDE_DIR}
)


add_subdirectory( pics ) 
add_subdirectory( tests ) 
add_subdirectory( profiles ) 



########### next target ###############

set(kmix_KDEINIT_SRCS
   mixeradaptor.cpp	
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

kde4_add_kdeinit_executable( kmix ${kmix_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmix  ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTXML_LIBRARY} kutils phononcore )

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmix ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kdeinit_kmix  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmix kdeinit_kmix )
install(TARGETS kmix  DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############

set(kmixctrl_KDEINIT_SRCS
   mixeradaptor.cpp
   kmixctrl.cpp 
   mixer.cpp 
   mixset.cpp 
   mixdevice.cpp 
   volume.cpp 
   mixertoolbox.cpp 
   mixer_backend.cpp 
   guiprofile.cpp )

kde4_automoc(${kmixctrl_KDEINIT_SRCS})


kde4_add_kdeinit_executable( kmixctrl ${kmixctrl_KDEINIT_SRCS})

target_link_libraries(kdeinit_kmixctrl  ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTXML_LIBRARY} )

if (HAVE_LIBASOUND2)
    target_link_libraries(kdeinit_kmixctrl ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kdeinit_kmixctrl  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kmixctrl kdeinit_kmixctrl )
install(TARGETS kmixctrl DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############
if(KICKER_INCLUDE_DIR)

set(kmix_panelapplet_PART_SRCS
   mixeradaptor.cpp
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

kde4_add_ui3_files(kmix_panelapplet_PART_SRCS colorwidget.ui )


kde4_add_plugin(kmix_panelapplet ${kmix_panelapplet_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kmix_panelapplet )

target_link_libraries(kmix_panelapplet  ${KDE4_KDE3SUPPORT_LIBS} kutils kickermain ${QT_QTXML_LIBRARY} )

if (HAVE_LIBASOUND2)
    target_link_libraries(kmix_panelapplet ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

install(TARGETS kmix_panelapplet  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES kmixapplet.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kicker/applets )
else(KICKER_INCLUDE_DIR)
MESSAGE(STATUS "kmix applet will not compile. Install kdebase before")
endif(KICKER_INCLUDE_DIR)

########### install files ###############

install( FILES kmix.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES restore_kmix_volumes.desktop  DESTINATION /share/autostart)
install( FILES kmixui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kmix )
install( FILES kmixctrl_restore.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




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
