SUBDIRS = pics
INCLUDES= $(all_includes)

bin_PROGRAMS = kmix
lib_LTLIBRARIES = libkmixapplet.la


noinst_HEADERS = kmix.h kmixdockwidget.h kmixprefdlg.h kmixerwidget.h \
	channel.h mixer.h volume.h kledbutton.h version.h \
	mixdevicewidget.h ksmallslider.h kmixapplet.h


METASOURCES = AUTO
kmix_SOURCES = main.cpp kmix.cpp kmixdockwidget.cpp kmixprefdlg.cpp \
	kmixerwidget.cpp channel.cpp mixer.cpp ksmallslider.cpp \
	volume.cpp kledbutton.cpp mixdevicewidget.cpp
kmix_LDADD = $(LIB_KDEUI) $(LIBALIB) $(LIBOSSAUDIO) $(LIBASOUND)
kmix_LDFLAGS = $(all_libraries) $(KDE_RPATH)


libkmixapplet_la_METASOURCES=AUTO
libkmixapplet_la_SOURCES = kmixapplet.cpp kmixerwidget.cpp channel.cpp mixer.cpp \
	ksmallslider.cpp volume.cpp kledbutton.cpp mixdevicewidget.cpp
libkmixapplet_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module 
libkmixapplet_la_LIBADD = $(LIB_KDEUI)

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

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kmix.pot

