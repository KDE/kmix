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
#include "core/mixer.h"

#include <QString>



#if defined(sun) || defined(__sun__)
#define SUN_MIXER
#endif

#ifdef __linux__

#define OSS_MIXER
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(_UNIXWARE)
#define OSS_MIXER
#endif

#if defined(hpux)
#error "The HP/UX port is not maintained anymore, an no official part of KMix / KDE at this point of time! Please contact the current KMix maintainer if you would like to maintain the port."
#endif // hpux

// PORTING: add #ifdef PLATFORM , commands , #endif, add your new mixer below

// Compiled by its own!
//#include "backends/mixer_mpris2.cpp"

#if defined(SUN_MIXER)
#include "backends/mixer_sun.cpp"
#endif

// OSS 3 / 4
#if defined(OSS_MIXER)
#include "backends/mixer_oss.cpp"

#if !defined(__NetBSD__) && !defined(__OpenBSD__)
#include <sys/soundcard.h>
#else
#include <soundcard.h>
#endif
#if !defined(__FreeBSD__) && (SOUND_VERSION >= 0x040000)
#define OSS4_MIXER
#endif
#endif

#if defined(OSS4_MIXER)
#include "backends/mixer_oss4.cpp"
#endif


// Possibly encapsualte by #ifdef HAVE_DBUS
Mixer_Backend* MPRIS2_getMixer(Mixer *mixer, int device );
QString MPRIS2_getDriverName();

Mixer_Backend* ALSA_getMixer(Mixer *mixer, int device );
QString ALSA_getDriverName();

Mixer_Backend* PULSE_getMixer(Mixer *mixer, int device );
QString PULSE_getDriverName();

MixerFactory g_mixerFactories[] = {

#if defined(SUN_MIXER)
    { SUN_getMixer, SUN_getDriverName },
#endif

#if defined(HAVE_PULSE)
    { PULSE_getMixer, PULSE_getDriverName },
#endif

#if defined(HAVE_LIBASOUND2)
    { ALSA_getMixer, ALSA_getDriverName },
#endif

#if defined(OSS_MIXER)
    { OSS_getMixer, OSS_getDriverName },
#endif

#if defined(OSS4_MIXER)
    { OSS4_getMixer, OSS4_getDriverName },
#endif

    // Make sure MPRIS2 is at the end. Implementation of SINGLE_PLUS_MPRIS2 in MixerToolBox is much easier.
    // And also we make sure, streams are always the last backend, which is important for the default KMix GUI layout.
    { MPRIS2_getMixer, MPRIS2_getDriverName },

    { 0, 0 }
};

