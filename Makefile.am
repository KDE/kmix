 
INCLUDES= $(all_includes)

####### Files

SUBDIRS = pics
bin_PROGRAMS = 	kmix kmixclient
kmix_SOURCES	= volume.cpp mixer.cpp kmix.cpp prefs.cpp kmix-docking.cpp  \
	mixdevice.cpp channel.cpp docking.cpp
noinst_HEADERS = volume.h kmix.h mixer.h mixdevice.h prefs.h version.h kmix-docking.h  channel.h docking.h
kmix_LDADD =   $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmix_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kmixclient_SOURCES	= kmixclient.cpp
kmixclient_LDADD	= $(LIB_KDECORE) $(LIBAUDIO)
kmixclient_LDFLAGS	= $(all_libraries) $(KDE_RPATH)


METASOURCES = kmix.moc mixer.moc prefs.moc kmix-docking.moc docking.moc mixdevice.moc

$(srcdir)/version.h: VERSION
	cd $(srcdir) ;\
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h ;\
	echo \" >> version.h

EXTRA_DIST = kmix.desktop VERSION

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmix.desktop

messages:
	$(XGETTEXT) $(kmix_SOURCES) -o $(podir)/kmix.pot

edit:
	xemacs $(kmix_SOURCES) $(noinst_HEADERS) &

