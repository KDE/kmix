#INCLUDES = -I$(top_srcdir)/kmix $(all_includes)
# esken: At my box I need to add -lqt-mt  . I thought it would be included in $(QT_LDFLAGS) ?!?
#AM_LDFLAGS = -lqt-mt $(QT_LDFLAGS) $(X_LDFLAGS) $(KDE_RPATH)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
AM_CPPFLAGS = -I$(top_srcdir)/kmix $(all_includes)

check_PROGRAMS = profiletest

METASOURCES = AUTO

profiletest_SOURCES = profiletest.cpp
# ../guiprofile.cpp
profiletest_LDFLAGS = $(all_libraries) $(KDE_RPATH)
profiletest_LDADD = ../guiprofile.o

