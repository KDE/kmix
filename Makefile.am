 
APPSDIR = $(kde_appsdir)/Multimedia
INCLUDES= $(all_includes)

####### Files

SUBDIRS = pics
bin_PROGRAMS = 	kmix
kmix_SOURCES	= kcontextmenu.cpp kmix.cpp mixer.cpp prefs.cpp
noinst_HEADERS = kcontextmenu.h kmix.h mixer.h prefs.h version.h
kmix_LDADD = $(all_libraries) -lkdeui -lkdecore -lqt -lXext -lX11
kmix_LDFLAGS = $(LDFLAGS) $(KDE_RPATH)

BUILTSOURCES = kmix.moc mixer.moc prefs.moc
DISTCLEANFILES = kmix.moc mixer.moc prefs.moc



kmix.cpp: kmix.moc
kmix.moc: kmix.h
	$(MOC) kmix.h -o kmix.moc

mixer.cpp: mixer.moc
mixer.moc: mixer.h
	$(MOC) mixer.h -o mixer.moc

prefs.cpp: prefs.moc
prefs.moc: prefs.h
	$(MOC) prefs.h -o prefs.moc

version.h: VERSION
	sed "s/0/#define APP_VERSION 0/" <VERSION >version.h

install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) KMix.kdelnk $(APPSDIR)

uninstall-local:
	rm -f $(APPSDIR)/KMix.kdelnk

messages:
	@XGETTEXT@ -C -ki18n *.cpp -x../../kdelibs/po/kde.pot && mv messages.po ../po/kmix.pot

edit:
	xemacs $(kmix_SOURCES) $(noinst_HEADERS) &
