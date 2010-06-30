include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckStructMember)
include(MacroBoolTo01)

# The FindKDE4.cmake module sets _KDE4_PLATFORM_DEFINITIONS with
# definitions like _GNU_SOURCE that are needed on each platform.
set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})

macro_bool_to_01(CARBON_FOUND HAVE_CARBON)

macro_bool_to_01(AKODE_FOUND HAVE_AKODE)

macro_bool_to_01(OGGVORBIS_FOUND HAVE_VORBIS)

macro_bool_to_01(X11_XShm_FOUND  HAVE_XSHMGETEVENTBASE)

macro_bool_to_01(PULSEAUDIO_FOUND HAVE_PULSE)

#now check for dlfcn.h using the cmake supplied CHECK_include_FILE() macro
# If definitions like -D_GNU_SOURCE are needed for these checks they
# should be added to _KDE4_PLATFORM_DEFINITIONS when it is originally
# defined outside this file.  Here we include these definitions in
# CMAKE_REQUIRED_DEFINITIONS so they will be included in the build of
# checks below.
set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})
if (WIN32)
   set(CMAKE_REQUIRED_LIBRARIES ${KDEWIN32_LIBRARIES} )
   set(CMAKE_REQUIRED_INCLUDES  ${KDEWIN32_INCLUDES} )
endif (WIN32)

check_include_files(machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H)
check_include_files(soundcard.h HAVE_SOUNDCARD_H)
check_include_files(sys/soundcard.h HAVE_SYS_SOUNDCARD_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(linux/cdrom.h HAVE_LINUX_CDROM_H)
check_include_files(linux/ucdrom.h HAVE_LINUX_UCDROM_H)
check_include_files(machine/endian.h HAVE_MACHINE_ENDIAN_H)
check_include_files(sys/audioio.h HAVE_SYS_AUDIOIO_H)
check_include_files(Alib.h HAVE_ALIB_H)
check_include_files(alloca.h HAVE_ALLOCA_H)
# Linux has <endian.h>, FreeBSD has <sys/endian.h> and Solaris has neither.
check_include_files(endian.h HAVE_ENDIAN_H)
check_include_files(sys/endian.h HAVE_SYS_ENDIAN_H)
check_include_files(unistd.h HAVE_UNISTD_H)

check_type_size("long" SIZEOF_LONG)

