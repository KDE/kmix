INCLUDES= $(all_includes)

KDE_ICON=AUTO

kcm_kmix_la_LIBADD = $(LIB_KDEUI)
kcm_kmix_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
kcm_kmix_la_METASOURCES = AUTO
kcm_kmix_la_SOURCES = mixconfig.cpp

kde_module_LTLIBRARIES = kcm_kmix.la

noinst_HEADERS = mixconfig.h

DISTCLEANFILES = $(kcm_kmix_la_METASOURCES)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kcmkmix.pot

data_DATA = kmixcfg.desktop
datadir = $(kde_appsdir)/.hidden
