 
APPSDIR = $(kde_appsdir)/Multimedia
INCLUDES= $(all_includes)

####### Files

SUBDIRS = pics docs
bin_PROGRAMS = 	kmix
kmix_SOURCES	= kmix.cpp mixer.cpp prefs.cpp docking.cpp sets.cpp channel.cpp
noinst_HEADERS = kmix.h mixer.h prefs.h version.h docking.h sets.h channel.h
kmix_LDADD =       $(LIB_KDEUI)
kmix_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = kmix.moc mixer.moc prefs.moc docking.moc

$(srcdir)/version.h: VERSION
	cd $(srcdir) ;\
	sed "s/1/#define APP_VERSION \"1/" <VERSION >version.h ;\
	echo \" >> version.h

EXTRA_DIST = KMix.kdelnk VERSION

install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) $(srcdir)/KMix.kdelnk $(APPSDIR)

uninstall-local:
	rm -f $(APPSDIR)/KMix.kdelnk

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kmix_SOURCES) && mv messages.po ../po/kmix.pot

edit:
	xemacs $(kmix_SOURCES) $(noinst_HEADERS) &
