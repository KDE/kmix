SUBDIRS = pics

INCLUDES= $(all_includes)

bin_PROGRAMS = kmix

kmix_SOURCES = main.cpp kmixapplet.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp \
	kmixerwidget.cpp channel.cpp mixer.cpp ksmallslider.cpp \
	volume.cpp kledbutton.cpp mixdevicewidget.cpp

noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h \
	channel.h mixer.h volume.h kledbutton.h version.h \
	mixdevicewidget.h ksmallslider.h kmixapplet.h

kmix_LDADD = $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)

kmix_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = AUTO

$(srcdir)/version.h: VERSION
	cd $(srcdir) ;\
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h ;\
	echo \" >> version.h

EXTRA_DIST = kmix.desktop VERSION

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmix.desktop

rcdir = $(kde_datadir)/kmix
rc_DATA = kmixui.rc

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kmix.pot
