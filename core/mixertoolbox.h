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

#include <qobject.h>
#include <qlist.h>
#include <QMap>
#include <QRegExp>
#include <QString>

#include "kmixcore_export.h"

class Mixer;

/**
 * This toolbox contains various static methods that are shared throughout KMix.
 * It only contains no-GUI code. The shared with-GUI code is in KMixToolBox
 * The reason, why it is not put in a common base class is, that the classes are
 * very different and cannot be changed (e.g. KPanelApplet) without major headache.
 */

namespace MixerToolBox
{
    KMIXCORE_EXPORT void initMixer(bool multiDriverFlag, const QStringList &backendList, bool hotplug);

    KMIXCORE_EXPORT void deinitMixer();
    KMIXCORE_EXPORT bool possiblyAddMixer(Mixer *mixer);
    KMIXCORE_EXPORT void removeMixer(Mixer *mixer);
    KMIXCORE_EXPORT void setMixerIgnoreExpression(const QString &ignoreExpr);
    KMIXCORE_EXPORT QString mixerIgnoreExpression();
}

#endif
