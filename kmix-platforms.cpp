/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* This code is being #include'd from mixer.cpp */

#include <config.h>

#include <qstring.h>

#include "mixer_backend.h"

#if defined(sun) || defined(__sun__)
#define SUN_MIXER
#endif

#ifdef sgi
#include <sys/fcntl.h>
#define IRIX_MIXER
#endif

#ifdef __linux__

#ifdef HAVE_LIBASOUND2
#define ALSA_MIXER
#endif

#define OSS_MIXER
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(_UNIXWARE) || defined(__DragonFly__)
#define OSS_MIXER
#endif

#if defined(hpux)
# if defined(HAVE_ALIB_H)
#  define HPUX_MIXER
# else
#  warning ** YOU NEED to have libAlib installed to use the HP-UX-Mixer **
# endif // HAVE_ALIB_H
#endif // hpux

// PORTING: add #ifdef PLATFORM , commands , #endif, add your new mixer below
#if defined(NAS_MIXER)
#include "mixer_nas.cpp"
#endif

#if defined(SUN_MIXER)
#include "mixer_sun.cpp"
#endif

#if defined(IRIX_MIXER)
#include "mixer_irix.cpp"
#endif

// Alsa API's 
#if defined(ALSA_MIXER)
#include "mixer_alsa9.cpp"
#endif

#if defined(OSS_MIXER)
#include "mixer_oss.cpp"

#if !defined(__NetBSD__) && !defined(__OpenBSD__)
#include <sys/soundcard.h>
#else
#include <soundcard.h>
#endif
#if SOUND_VERSION >= 0x040000
#define OSS4_MIXER
#endif

#endif

#if defined(OSS4_MIXER)
#include "mixer_oss4.cpp"
#endif

#if defined(HPUX_MIXER)
#include "mixer_hpux.cpp"
#endif

/*
#else
// We cannot handle this! I install a dummy mixer instead.
#define NO_MIXER
#include "mixer_none.cpp"
#endif
*/

typedef Mixer_Backend *getMixerFunc( int device );
typedef QString getDriverNameFunc( );

struct MixerFactory {
    getMixerFunc *getMixer;
    getDriverNameFunc *getDriverName;
};

MixerFactory g_mixerFactories[] = {

#if defined(NAS_MIXER)
    { NAS_getMixer, 0 },
#endif

#if defined(SUN_MIXER)
    { SUN_getMixer, SUN_getDriverName },
#endif

#if defined(IRIX_MIXER)
    { IRIX_getMixer, IRIX_getDriverName },
#endif

#if defined(ALSA_MIXER)
    { ALSA_getMixer, ALSA_getDriverName },
#endif

#if defined(OSS4_MIXER)
    { OSS4_getMixer, OSS4_getDriverName },
#endif

#if defined(OSS_MIXER)
    { OSS_getMixer, OSS_getDriverName },
#endif

#if defined(HPUX_MIXER)
    { HPUX_getMixer, HPUX_getDriverName },
#endif

    { 0, 0 }
};

