INCLUDES= $(all_includes)

KDE_ICON=AUTO

libkcm_kmix_la_LIBADD = $(LIB_KDEUI)
libkcm_kmix_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
libkcm_kmix_la_METASOURCES = AUTO
libkcm_kmix_la_SOURCES = mixconfig.cpp

lib_LTLIBRARIES = libkcm_kmix.la

noinst_HEADERS = mixconfig.h

DISTCLEANFILES = $(libkcm_kmix_la_METASOURCES)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kcmkmix.pot

data_DATA = kmix.desktop
datadir = $(kde_appsdir)/Settings/Sound
