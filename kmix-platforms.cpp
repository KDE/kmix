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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This code is being #include'd from mixer.cpp */

#include <config.h>

#if defined(sun) || defined(__sun__)
#define SUN_MIXER
#endif

#ifdef sgi
#include <sys/fcntl.h>
#define IRIX_MIXER
#endif

#ifdef __linux__

#ifdef ALSA
#define ALSA_MIXER
#endif

#define OSS_MIXER
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__bsdi__) || defined(_UNIXWARE)
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

#if defined(ALSA_MIXER)
#include "mixer_alsa.cpp"
#endif

#if defined(OSS_MIXER)
#include "mixer_oss.cpp"
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

typedef Mixer *getMixerFunc( int device, int card );
typedef Mixer *getMixerSetFunc( MixSet set, int device, int card );

struct MixerFactory {
    getMixerFunc *getMixer;
    getMixerSetFunc *getMixerSet;
};

MixerFactory g_mixerFactories[] = {

#if defined(NAS_MIXER)
    { NAS_getMixer, 0 },
#endif

#if defined(SUN_MIXER)
    { SUN_getMixer, SUN_getMixerSet },
#endif

#if defined(IRIX_MIXER)
    { IRIX_getMixer, 0 },
#endif

#if defined(ALSA_MIXER)
    { ALSA_getMixer, ALSA_getMixerSet },
#endif

#if defined(OSS_MIXER)
    { OSS_getMixer, OSS_getMixerSet },
#endif

#if defined(HPUX_MIXER)
    { HPUX_getMixer, 0 },
#endif

    { 0, 0 }
};
