 
APPSDIR = $(kde_appsdir)/Multimedia
INCLUDES= $(all_includes)

####### Files

SUBDIRS = pics docs
bin_PROGRAMS = 	kmix
kmix_SOURCES	= kcontextmenu.cpp kmix.cpp mixer.cpp prefs.cpp docking.cpp sets.cpp channel.cpp
noinst_HEADERS = kcontextmenu.h kmix.h mixer.h prefs.h version.h docking.h sets.h channel.h
kmix_LDADD = $(all_libraries) -lkdeui -lkdecore -lqt -lXext -lX11 $(LIBSOCKET)
kmix_LDFLAGS = $(LDFLAGS) $(KDE_RPATH)

BUILTSOURCES = kmix.moc mixer.moc prefs.moc docking.moc
DISTCLEANFILES = kmix.moc mixer.moc prefs.moc docking.moc



kmix.cpp: kmix.moc
kmix.moc: kmix.h
	$(MOC) kmix.h -o kmix.moc

mixer.cpp: mixer.moc
mixer.moc: mixer.h
	$(MOC) mixer.h -o mixer.moc

prefs.cpp: prefs.moc
prefs.moc: prefs.h
	$(MOC) prefs.h -o prefs.moc

docking.cpp: docking.moc
docking.moc: docking.h
	$(MOC) docking.h -o docking.moc

version.h: VERSION
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h
	echo \" >> version.h

EXTRA_DIST = KMix.kdelnk VERSION

install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) KMix.kdelnk $(APPSDIR)

uninstall-local:
	rm -f $(APPSDIR)/KMix.kdelnk

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kmix_SOURCES) && mv messages.po ../po/kmix.pot

edit:
	xemacs $(kmix_SOURCES) $(noinst_HEADERS) &
