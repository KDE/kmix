//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef MIXERTOOLBOX_H
#define MIXERTOOLBOX_H

#include <qstringlist.h>

#include "kmixcore_export.h"

#include "MasterControl.h"

class Mixer;
class MixDevice;
class MixerBackend;


/**
 * This toolbox contains various static methods that are shared throughout KMix.
 * It only contains no-GUI code, the shared with-GUI code is in KMixToolBox.
 * The reason, why it is not put in a common base class is, that the classes are
 * very different and cannot be changed (e.g. KPanelApplet) without major headache.
 */

namespace MixerToolBox
{
    // Probe for all supported mixers at application startup.
    KMIXCORE_EXPORT void initMixer(bool allowHotplug);
    // Close and remove all of the mixers at application shutdown.
    KMIXCORE_EXPORT void deinitMixer();

    // Manage the global list of mixers, either at initialisation time
    // or by hotplug.
    KMIXCORE_EXPORT bool possiblyAddMixer(Mixer *mixer);
    KMIXCORE_EXPORT void removeMixer(Mixer *mixer);

    // Matching of mixer names that should be ignored.
    // Note that this is not the same as setAllowedBackends()
    // which filters by backend name.
    KMIXCORE_EXPORT void setMixerIgnoreExpression(const QString &ignoreExpr);
    KMIXCORE_EXPORT QString mixerIgnoreExpression();

    // A read-only list of all the currently known mixers.
    KMIXCORE_EXPORT const QList<Mixer *> &mixers();

    // Find a mixer with the given 'mixerId'
    KMIXCORE_EXPORT Mixer *findMixer(const QString &mixerId);

    // Managing the global master mixer.
    KMIXCORE_EXPORT void setGlobalMaster(const QString &ref_card, const QString &ref_control, bool preferred);
    KMIXCORE_EXPORT Mixer *getGlobalMasterMixer(bool fallbackAllowed = true);
    KMIXCORE_EXPORT MasterControl &getGlobalMasterPreferred(bool fallbackAllowed = true);
    KMIXCORE_EXPORT shared_ptr<MixDevice> getGlobalMasterMD(bool fallbackAllowed = true);

    // Whether these apply somewhere among all known mixers.
    KMIXCORE_EXPORT bool dynamicBackendsPresent();
    KMIXCORE_EXPORT bool pulseaudioPresent();
    KMIXCORE_EXPORT bool backendPresent(const QString &driverName);
    KMIXCORE_EXPORT bool backendAvailable(const QString &driverName);
    KMIXCORE_EXPORT QString preferredBackend();

    // Creating a mixer backend using the list of available backends.
    KMIXCORE_EXPORT MixerBackend *getBackendFor(const QString &backendName, int deviceIndex, Mixer *mixer);

    // Set the mixer backends allowed to be used (for testing), formerly the
    // second parameter passed to initMixer().  If this list is not empty
    // then only those backends will be probed on startup or considered for
    // hotplugging.  This is a global setting that affects the entire
    // application, and is not the same as the ignored mixer names set by
    // setMixerIgnoreExpression() above.
    //
    // The PulseAudio backend reads the KMIX_PULSEAUDIO_DISABLE environment
    // variable which, if set, disables the PulseAudio mainloop connection.
    // However, this backend list acts before that variable is checked -
    // if it is not included in the list then the PulseAudio backend
    // is not even considered.
    KMIXCORE_EXPORT void setAllowedBackends(const QStringList &backends);

    // The experimental multiple driver mode, formerly the first parameter
    // passed to initMixer().  This is again a global setting that affects
    // the entire application.
    KMIXCORE_EXPORT void setMultiDriverMode(bool multiDriverMode);
    KMIXCORE_EXPORT bool isMultiDriverMode();
}

#endif
