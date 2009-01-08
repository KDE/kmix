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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* This code is being #include'd from mixer.cpp */

#include <config.h>
#include <config-alsa.h>

#include "mixer_backend.h"
#include "mixer.h"

#include <QString>



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

#ifdef HAVE_PULSE
#define PULSE_MIXER
#endif

#define OSS_MIXER
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(_UNIXWARE)
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

// Pulse API
#if defined(PULSE_MIXER)
#include "mixer_pulse.cpp"
#endif

#if defined(OSS_MIXER)
#include "mixer_oss.cpp"
#endif

#if defined(HPUX_MIXER)
#include "mixer_hpux.cpp"
#endif


typedef Mixer_Backend *getMixerFunc( Mixer* mixer, int device );
typedef QString getDriverNameFunc( );

struct MixerFactory {
    getMixerFunc *getMixer;
    getDriverNameFunc *getDriverName;
};

MixerFactory g_mixerFactories[] = {

#if defined(SUN_MIXER)
    { SUN_getMixer, SUN_getDriverName },
#endif

#if defined(IRIX_MIXER)
    { IRIX_getMixer, IRIX_getDriverName },
#endif

#if defined(ALSA_MIXER)
    { ALSA_getMixer, ALSA_getDriverName },
#endif

#if defined(OSS_MIXER)
    { OSS_getMixer, OSS_getDriverName },
#endif

#if defined(HPUX_MIXER)
    { HPUX_getMixer, HPUX_getDriverName },
#endif

#if defined(PULSE_MIXER)
    { PULSE_getMixer, PULSE_getDriverName },
#endif

    { 0, 0 }
};

