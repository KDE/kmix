 
INCLUDES= $(all_includes)

####### Files

SUBDIRS = pics
bin_PROGRAMS = 	kmix kmixclient
kmix_SOURCES	= kmix.cpp mixer.cpp prefs.cpp kmix-docking.cpp sets.cpp channel.cpp docking.cpp QceStateLED.cpp
noinst_HEADERS = kmix.h mixer.h prefs.h version.h kmix-docking.h sets.h channel.h docking.h QceStateLED.h
kmix_LDADD =   $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmix_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kmixclient_SOURCES	= kmixclient.cpp
kmixclient_LDADD	= $(LIB_KDECORE)
kmixclient_LDFLAGS	= $(all_libraries) $(KDE_RPATH)


METASOURCES = kmix.moc mixer.moc prefs.moc kmix-docking.moc docking.moc QceStateLED.moc

$(srcdir)/version.h: VERSION
	cd $(srcdir) ;\
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h ;\
	echo \" >> version.h

EXTRA_DIST = kmix.desktop VERSION

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmix.desktop

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kmix_SOURCES) && mv messages.po ../po/kmix.pot

edit:
	xemacs $(kmix_SOURCES) $(noinst_HEADERS) &

