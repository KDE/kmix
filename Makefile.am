SUBDIRS = pics

INCLUDES= $(all_includes)

bin_PROGRAMS = kmix2

kmix2_SOURCES = main.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp \
	kmixerwidget.cpp channel.cpp mixdevice.cpp mixer.cpp sets.cpp \
	volume.cpp kledbutton.cpp

noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h \
	channel.h mixdevice.h mixer.h sets.h volume.h kledbutton.h \
	version.h

kmix2_LDADD = $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)

kmix2_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = AUTO

$(srcdir)/version.h: VERSION
	cd $(srcdir) ;\
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h ;\
	echo \" >> version.h

EXTRA_DIST = kmix2.desktop VERSION

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmix2.desktop

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kmix.pot
