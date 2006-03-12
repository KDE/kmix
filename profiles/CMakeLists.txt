kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kmix/profiles FILES OSS.default.xml ALSA.default.xml )

kde4_footer()



#original Makefile.am contents follow:

#
#kmixprofiledir = $(kde_datadir)/kmix/profiles
#kmixprofile_DATA = OSS.default.xml ALSA.default.xml
#
## Read all the following instructions carefully.
## If you want to try the profile thing NOW, please backup your ~/.kde/share/config/kmix*rc* before!
#
## If you want to try the profile thing with ***OSS***, add OSS.default.xml to kmixprofile_DATA above
## OSS.default.xml
#
## If you want to try the profile thing with ***ALSA***, do two things:
## (1) #define KMIX_ALSA_NEW_PK    in mixer_alsa9.cpp
## (2) Add ALSA.default.xml to kmixprofile_DATA above
## ALSA.default.xml
#
## Example configuration for the "Sound Fusion CS46xx" card
## ALSA.Sound_Fusion_CS46xx.xml
#
#
