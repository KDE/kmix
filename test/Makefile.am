SUBDIRS =
INCLUDES = -I$(top_srcdir)/kmix $(all_includes)
# esken: At my box I need to add -lqt-mt  . I thought it would be included in $(QT_LDFLAGS) ?!?
AM_LDFLAGS = -lqt-mt $(QT_LDFLAGS) $(X_LDFLAGS) $(KDE_RPATH)
#AM_LDFLAGS = $(QT_LDFLAGS) $(X_LDFLAGS) $(KDE_RPATH)


check_PROGRAMS = profiletest

METASOURCES = AUTO

profiletest_SOURCES = profiletest.cpp ../guiprofile.cpp
