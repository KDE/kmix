SUBDIRS = control pics
INCLUDES= $(all_includes)

bin_PROGRAMS = 
lib_LTLIBRARIES = 
kdeinit_LTLIBRARIES = kmix.la kmixctrl.la
kde_module_LTLIBRARIES = kmix_panelapplet.la


noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h \
	channel.h mixer.h volume.h kledbutton.h version.h \
	mixdevicewidget.h ksmallslider.h kmixapplet.h mixerIface.h verticaltext.h \
	mixerselector.h mixerselectioninfo.h KMixApp.h


METASOURCES = AUTO

kmix_la_SOURCES = main.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp \
	kmixerwidget.cpp channel.cpp mixer.cpp ksmallslider.cpp \
	volume.cpp kledbutton.cpp mixdevicewidget.cpp verticaltext.cpp mixerIface.skel \
	mixerselector.cpp mixerselectioninfo.cpp KMixApp.cpp

kmix_la_LIBADD = $(LIB_KDEUI) $(LIB_KUTILS) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmix_la_LDFLAGS = $(all_libraries) -module -avoid-version

kmixctrl_la_SOURCES = kmixctrl.cpp channel.cpp mixer.cpp volume.cpp mixerIface.skel
kmixctrl_la_LIBADD = $(LIB_KDEUI) $(LIB_KUTILS) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmixctrl_la_LDFLAGS = $(all_libraries) -module -avoid-version

kmix_panelapplet_la_SOURCES = kmixapplet.cpp kmixerwidget.cpp channel.cpp \
	mixer.cpp ksmallslider.cpp volume.cpp kledbutton.cpp \
	mixdevicewidget.cpp verticaltext.cpp mixerIface.skel colorwidget.ui
kmix_panelapplet_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kmix_panelapplet_la_LIBADD = $(LIB_KDEUI) $(LIB_KUTILS) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)

EXTRA_DIST = kmix.desktop

xdg_apps_DATA = kmix.desktop

rcdir = $(kde_datadir)/kmix
rc_DATA = kmixui.rc

lnkdir = $(kde_datadir)/kicker/applets
lnk_DATA = kmixapplet.desktop

servicesdir = $(kde_servicesdir)
services_DATA = kmixctrl_restore.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kmix.pot

dummy.cpp:
	echo > dummy.cpp
