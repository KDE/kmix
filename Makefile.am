INCLUDES= $(all_includes)

####### Files

SUBDIRS = pics
bin_PROGRAMS = 	kmix
kmix_SOURCES	= main.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp kmixerwidget.cpp channel.cpp mixdevice.cpp mixer.cpp sets.cpp volume.cpp kledbutton.cpp
noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h channel.h mixdevice.h mixer.h sets.h volume.h kledbutton.h version.h
kmix_LDADD =   $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmix_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = AUTO

$(srcdir)/version.h: VERSION
	cd $(srcdir) ;\
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h ;\
	echo \" >> version.h

EXTRA_DIST = kmix.desktop VERSION

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmix.desktop

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kmix.pot

edit:
	xemacs $(kmix_SOURCES) $(noinst_HEADERS) &

