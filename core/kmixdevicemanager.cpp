/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken <esken@kde.org>
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

#include "core/kmixdevicemanager.h"

#include <QTimer>

#include "kmix_debug.h"
#include "core/mixertoolbox.h"
#include "core/mixer.h"

#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/block.h>


static const int HOTPLUG_DELAY = 1000;			// settling delay, milliseconds


KMixDeviceManager *KMixDeviceManager::instance()
{
    static KMixDeviceManager *instance = new KMixDeviceManager();
    return (instance);
}


void KMixDeviceManager::initHotplug()
{
    qCDebug(KMIX_LOG) << "Connecting to Solid";
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded,
            this, &KMixDeviceManager::pluggedSlot);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved,
            this, &KMixDeviceManager::unpluggedSlot);
}


void KMixDeviceManager::pluggedSlot(const QString &udi)
{
    if (!MixerToolBox::isSoundDevice(udi)) return;	// quickly ignore non-sound devices

    // Check whether a backend recognises the UDI
    const QPair<QString,int> dev = MixerToolBox::acceptsHotplugId(udi);
    const int devnum = dev.second;
    if (devnum==-1)
    {
        qCDebug(KMIX_LOG) << "Ignored unrecognised UDI" << udi;
        return;
    }

    const QString &backend = dev.first;
    qCDebug(KMIX_LOG) << "Plugged UDI" << udi << "->" << backend << "dev" << devnum;
    QTimer::singleShot(HOTPLUG_DELAY, this, [=]() { emit plugged(backend.toLatin1(), udi, devnum); });
}


void KMixDeviceManager::unpluggedSlot(const QString &udi)
{
    if (!MixerToolBox::isSoundDevice(udi)) return;	// quickly ignore non-sound devices

    // Check whether a backend recognises the UDI
    const QPair<QString,int> dev = MixerToolBox::acceptsHotplugId(udi);
    const int devnum = dev.second;
    if (devnum==-1)
    {
        qCDebug(KMIX_LOG) << "Ignored unrecognised UDI" << udi;
        return;
    }

    const QString &backend = dev.first;
    qCDebug(KMIX_LOG) << "Unplugged UDI" << udi << "->" << backend << "dev" << devnum;
    QTimer::singleShot(HOTPLUG_DELAY, this, [=]() { emit unplugged(udi); });
}
