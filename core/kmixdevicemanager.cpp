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

#include <QRegExp>
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

    mHotplugTimer = new QTimer(this);
    mHotplugTimer->setInterval(HOTPLUG_DELAY);
    mHotplugTimer->setSingleShot(true);
    connect(mHotplugTimer, &QTimer::timeout, this, &KMixDeviceManager::slotTimer);
}


QString KMixDeviceManager::getUDI(int num) const
{
    // This is intended to be used for both ALSA and OSS (no other
    // backends currently support hotplugging).  It is in the format
    // "hw:<devnum>" which may or may not correspond to the name of
    // the device that needs to be provided to the backend API or
    // Canberra.  It does for ALSA.
    return (QString("hw:%1").arg(num));
}


static bool isSoundDevice(const QString &udi)
{
    QRegExp rx("/sound/");				// any UDI mentioning sound
    return (udi.contains(rx));
}


static int matchDevice(const QString &udi)
{
    QRegExp rx("/sound/card(\\d+)/");			// match sound card and number
    if (!udi.contains(rx)) return (-1);			// UDI not recognised
    return (rx.cap(1).toInt());				// assume conversion succeeds
}


void KMixDeviceManager::pluggedSlot(const QString &udi)
{
    if (!isSoundDevice(udi)) return;			// ignore non-sound devices

    Solid::Device device(udi);
    if (!device.isValid())
    {
        qCWarning(KMIX_LOG) << "Invalid device for UDI" << udi;
        return;
    }

    // Solid device UDIs for a plugged sound card are of the form:
    //
    // ALSA:
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:13.2/usb2/2-4/2-4.1/2-4.1:1.0/sound/card3
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:13.2/usb2/2-4/2-4.1/2-4.1:1.0/sound/card3/pcmC3D0p
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:13.2/usb2/2-1/2-1.3/2-1.3:1.0/sound/card3/controlC3
    //
    // OSS:
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:12.1/usb4/4-2/4-2:1.0/sound/card4
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:12.1/usb4/4-2/4-2:1.0/sound/card4/mixer4
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:12.1/usb4/4-2/4-2:1.0/sound/card4/pcm4
    //
    // However, there are a number of complications with the delivery of these
    // UDIs, for both hotplug and unplug events:
    //
    //   1.  Multiple UDIs as above may be delivered for a single device,
    //       in a random order.
    //   2.  UDIs may be delivered for more than one device, for example if
    //       a card that supports multiple devices is hotplugged.
    //   3.  If both ALSA and OSS drivers are supported on the system,
    //       then UDIs for both ALSA and OSS may be delivered.
    //   4.  For OSS, the canonical form (the first example above) may
    //       not be delivered.
    //   5.  Again for OSS, the device type (as in the second and third
    //       examples above) may vary.
    //
    // There is also the possibility that a hotplug may soon be followed by
    // an unplug for the same device, or vice versa.
    //
    // To correctly handle all of the above possibilities, the hotplug or
    // unplug events are not actioned immediately.  Pending events are noted
    // in the 'mPendingMap' map, indexed by device number.  By observation,
    // if both ALSA and OSS UDIs are delivered for a single device then the
    // device numbers for both are the same, so that is noted by separate
    // flags.  ALSA and OSS can be reliably distinguished by the physical
    // device name provided by Solid.
    //
    // Unplug events are also noted by a pending flag, with an additional flag
    // noting whether this unplug closely (within the HOTPLUG_DELAy time) follows
    // a hotplug for the same device.  In this case, there is no need to
    // action either event because the device will not have actually been added.
    //
    // The converse - a hotplug closely following an unplug for the same device -
    // cannot be ignored in the same way because the device will probably need
    // to be opened again.  Therefore the unplug and then the hotplug are both
    // actioned in that order.
    //
    // When things have settled down (after the HOTPLUG_DELAY time from the last
    // event), the complete situation, on a device by device basis, is evaluated
    // and appropriate actions taken.
    //
    // Note that the Solid device UDI is not used anywhere else within KMix,
    // what is referred to as a "UDI" elsewhere is as described in the
    // comment for Mixer::udi() in core/mixer.h and generated by getUDI()
    // above.  Solid only ever supported ALSA and OSS anyway, even in KDE4,
    // so we can assume the driver is one of those here.  See
    // https://community.kde.org/Frameworks/Porting_Notes#Solid_Changes

    const int devnum = matchDevice(udi);
    if (devnum!=-1 && device.is<Solid::Block>())
    {
        qCDebug(KMIX_LOG) << "Plugged UDI" << udi;

        const Solid::Block *blk = device.as<Solid::Block>();
        const QString blkdev = blk->device();
        qCDebug(KMIX_LOG) << "devnum" << devnum << "device" << blkdev;

        // Do not override a pending unplug for that same device,
        // because it will probably have to be opened again
        PendingFlags orig = mPendingMap[devnum];
        mPendingMap[devnum] = orig|(blkdev.contains("/snd/") ? PluggedALSA : PluggedOSS);
        //qCDebug(KMIX_LOG) << "  updated map [" << devnum << "]" << orig << "->" << mPendingMap[devnum];
        mHotplugTimer->start();
    }
    else qCDebug(KMIX_LOG) << "Ignored unrecognised UDI" << udi;
}


void KMixDeviceManager::unpluggedSlot(const QString &udi)
{
    if (!isSoundDevice(udi)) return;			// ignore non-sound devices

    // At this point the device has already been unplugged by the user.
    // Solid doesn't know anything about the device except the UDI, but
    // that can be matched using the same logic as for plugging.
    //
    // This may not work properly in multi driver mode, because UDIs may not
    // be unique among backends.  Since the format for ALSA and OSS is the
    // same (generated by getUDI() above), it is conceivable that ALSA and OSS
    // may each have had a device with the same number, so the loop in
    // KMixWindow::unplugged() would unplug the first one found regardless
    // of which driver caused the unplug event.  However, this configuration
    // is not supported in KMix and is not very useful for either KMix or the
    // system, so there is no need to consider which backend applies here.

    const int devnum = matchDevice(udi);
    if (devnum!=-1)
    {
        qCDebug(KMIX_LOG) << "Unplugged UDI" << udi;

        PendingFlags orig = mPendingMap[devnum];
        mPendingMap[devnum] = orig|Unplugged;
        // If there is a pending hotplug event (for the same device),
        // then note this unplug as overriding it.
        if (orig & (PluggedALSA|PluggedOSS)) mPendingMap[devnum] |= UnplugOverride;
        //qCDebug(KMIX_LOG) << "  updated map [" << devnum << "]" << orig << "->" << mPendingMap[devnum];
        mHotplugTimer->start();
    }
    else qCDebug(KMIX_LOG) << "Ignored unrecognised UDI" << udi;
}


void KMixDeviceManager::slotTimer()
{
    qCDebug(KMIX_LOG) << mPendingMap.count()  << "pending events";
    for (const int devnum : mPendingMap.keys())		// process each device by number
    {
        const PendingFlags ops = mPendingMap[devnum];	// pending operations for that device
        qCDebug(KMIX_LOG) << "  " << devnum << "=>" << ops;

        const QString ourUDI = getUDI(devnum);
        qCDebug(KMIX_LOG) << "  our UDI" << ourUDI;

        if (ops & Unplugged)				// device was unplugged
        {
            if (ops & UnplugOverride)			// soon after a hotplug,
            {						// nothing more to do
                qCDebug(KMIX_LOG) << "  unplug overrides hotplug, do nothing";
                continue;
            }

            emit unplugged(ourUDI);			// action the unplug event
        }

        bool plugALSA = (ops & PluggedALSA);		// an ALSA device was hotplugged
        bool plugOSS = (ops & PluggedOSS);		// an OSS device was hotplugged
        if (plugALSA || plugOSS)			// either of them happened
        {
            QString backend;				// backend that will be used
            if (plugALSA && plugOSS)
            {
                qCDebug(KMIX_LOG) << "  hotplug both ALSA and OSS";
                // Look to see which backends are already in use, and choose
                // one that is already being used with ALSA having priority.
                if (MixerToolBox::backendPresent("ALSA")) plugOSS = false;
                else if (MixerToolBox::backendPresent("OSS")) plugALSA = false;
                else if (MixerToolBox::backendPresent("OSS4")) plugALSA = false;
            }

            // If there are existing backends in use, then ALSA or OSS should now
            // have been resolved to match.  Even if that was not resolved above,
            // select ALSA or OSS (with ALSA having priority) if the backend is
            // available (that means supported by KMix, even if it is not currently
            // in use).
            if (plugALSA)
            {
                if (MixerToolBox::backendAvailable("ALSA")) backend = "ALSA";
            }
            else if (plugOSS)
            {
                if (MixerToolBox::backendAvailable("OSS")) backend = "OSS";
                else if (MixerToolBox::backendAvailable("OSS4")) backend = "OSS4";
            }

            // If the backend has still not been resolved, then use the default
            // preferred backend - which is the first "regular" backend which
            // is supported.
            if (backend.isEmpty()) backend = MixerToolBox::preferredBackend();

            qCDebug(KMIX_LOG) << "  -> using backend" << backend;
            emit plugged(backend.toLatin1(), ourUDI, devnum);
        }
    }

    mPendingMap.clear();				// nothing more is pending
}


void KMixDeviceManager::setHotpluggingBackends(const QString &backendName)
{
    qCDebug(KMIX_LOG) << "using" << backendName;
    // Note: this setting is ignored, it is assumed above that
    // Solid only delivers sound card hotplug events for either
    // ALSA or OSS.
    _hotpluggingBackend = backendName;
}
