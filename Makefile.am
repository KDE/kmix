SUBDIRS = pics
INCLUDES= $(all_includes)

bin_PROGRAMS = 
lib_LTLIBRARIES = 
kdeinit_LTLIBRARIES = kmix.la kmixctrl.la
kde_module_LTLIBRARIES = kmix_panelapplet.la


noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h \
	viewbase.h viewoutput.h viewinput.h viewsliders.h viewswitches.h viewsurround.h viewdockareapopup.h \
	mixdevicewidget.h mdwslider.h mdwswitch.h \
	mixer.h mixset.h mixdevice.h volume.h kledbutton.h version.h kmixtoolbox.h \
	ksmallslider.h kmixapplet.h mixerIface.h verticaltext.h \
	KMixApp.h dialogviewconfiguration.h kmixtoolbox.h


METASOURCES = AUTO

kmix_la_SOURCES = main.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp \
	viewbase.cpp viewoutput.cpp viewinput.cpp viewswitches.cpp viewsurround.cpp viewdockareapopup.cpp \
	viewsliders.cpp \
	mixdevicewidget.cpp mdwslider.cpp mdwswitch.cpp \
	kmixerwidget.cpp mixer.cpp mixset.cpp mixdevice.cpp ksmallslider.cpp \
	volume.cpp kledbutton.cpp verticaltext.cpp mixerIface.skel \
	kmixtoolbox.cpp dialogviewconfiguration.cpp KMixApp.cpp

kmix_la_LIBADD = $(LIB_KDEUI) $(LIB_KUTILS) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmix_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -avoid-version

kmixctrl_la_SOURCES = kmixctrl.cpp mixer.cpp mixset.cpp mixdevice.cpp volume.cpp mixerIface.skel
kmixctrl_la_LIBADD = $(LIB_KDECORE) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmixctrl_la_LDFLAGS = $(all_libraries) -module -avoid-version

# !!! todo: remove mdwslider.cpp mdwswitch.cpp from kmix_panelapplet_la_SOURCE

kmix_panelapplet_la_SOURCES = kmixapplet.cpp \
	viewbase.cpp viewapplet.cpp \
	mixdevicewidget.cpp mdwslider.cpp mdwswitch.cpp \
	mixer.cpp mixset.cpp mixdevice.cpp ksmallslider.cpp volume.cpp kledbutton.cpp \
	verticaltext.cpp mixerIface.skel colorwidget.ui dialogviewconfiguration.cpp kmixtoolbox.cpp

kmix_panelapplet_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kmix_panelapplet_la_LIBADD = $(LIB_KDEUI) $(LIB_KUTILS) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)

#FOO kmixerwidget.cpp viewoutput.cpp viewinput.cpp viewsliders.cpp viewsurround.cpp viewswitches.cpp

xdg_apps_DATA = kmix.desktop

rcdir = $(kde_datadir)/kmix
rc_DATA = kmixui.rc

autostart_DATA = restore_kmix_volumes.desktop
autostartdir = $(datadir)/autostart

lnkdir = $(kde_datadir)/kicker/applets
lnk_DATA = kmixapplet.desktop

servicesdir = $(kde_servicesdir)
services_DATA = kmixctrl_restore.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kmix.pot

