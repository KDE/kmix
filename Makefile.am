SUBDIRS = control pics
INCLUDES= $(all_includes)

bin_PROGRAMS = kmix kmixctrl
lib_LTLIBRARIES = kdeinit_kmix.la kdeinit_kmixctrl.la
kde_module_LTLIBRARIES = kmix_panelapplet.la


noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h \
	channel.h mixer.h volume.h kledbutton.h version.h \
	mixdevicewidget.h ksmallslider.h kmixapplet.h


METASOURCES = AUTO
kdeinit_kmix_la_SOURCES = main.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp \
	kmixerwidget.cpp channel.cpp mixer.cpp ksmallslider.cpp \
	volume.cpp kledbutton.cpp mixdevicewidget.cpp
kdeinit_kmix_la_LIBADD = $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kdeinit_kmix_la_LDFLAGS = $(all_libraries) -module -avoid-version

kmix_SOURCES = dummy.cpp
kmix_LDADD = kdeinit_kmix.la
kmix_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kdeinit_kmixctrl_la_SOURCES = kmixctrl.cpp channel.cpp mixer.cpp volume.cpp 
kdeinit_kmixctrl_la_LIBADD = $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kdeinit_kmixctrl_la_LDFLAGS = $(all_libraries) -module -avoid-version

kmixctrl_SOURCES = dummy.cpp
kmixctrl_LDADD = kdeinit_kmixctrl.la
kmixctrl_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kmix_panelapplet_la_SOURCES = kmixapplet.cpp kmixerwidget.cpp channel.cpp \
	mixer.cpp ksmallslider.cpp volume.cpp kledbutton.cpp \
	mixdevicewidget.cpp colorwidget.ui
kmix_panelapplet_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kmix_panelapplet_la_LIBADD = $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)

$(srcdir)/version.h: VERSION
	cd $(srcdir) ;\
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h ;\
	echo \" >> version.h

EXTRA_DIST = kmix.desktop VERSION

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmix.desktop

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
