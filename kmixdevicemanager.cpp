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

#include "kmixdevicemanager.h"

#include <iostream>

#include <QString>
#include <QObject>


#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/audiointerface.h>

/*
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "version.h"

static const char description[] =
I18N_NOOP("kmixd - Soundcard Mixer Device Manager");
*/
KMixDeviceManager* KMixDeviceManager::s_KMixDeviceManager = 0;

KMixDeviceManager::KMixDeviceManager()
{
}

KMixDeviceManager::~KMixDeviceManager()
{
}

KMixDeviceManager* KMixDeviceManager::instance()
{
    if ( s_KMixDeviceManager == 0 ) {
        s_KMixDeviceManager = new KMixDeviceManager();
    }
    return s_KMixDeviceManager;
}

void KMixDeviceManager::initHotplug()
{
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), SLOT(pluggedSlot(const QString&)) );
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString&)), SLOT(unpluggedSlot(const QString&)) );
}

QString KMixDeviceManager::getUDI_ALSA(int num)
{
    QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

    QString numString;
    numString.setNum(num);
    bool found = false;
    QString udi;
    QString devHandle;
    foreach ( Solid::Device device, dl )
    {
        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
        Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
        if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
            switch (audiohw->driver()) {
                case Solid::AudioInterface::Alsa:
                    devHandle = audiohw->driverHandle().toList().first().toString();
                    std::cout << ">>> Coldplugged ALSA ='" <<  devHandle.toUtf8().data() << "'\n";
                    if ( numString == devHandle ) {
                        found = true;
                        std::cout << ">>> Match!!! Coldplugged ALSA ='" <<  devHandle.toUtf8().data() << "'\n";
                        udi = device.udi();
                    }
                    break;
                default:
                    break;
            } // driver type
        } // is an audio control
        if ( found) break;
    } // foreach
    return udi;
}

QString KMixDeviceManager::getUDI_OSS(QString& devname)
{
    QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

    bool found = false;
    QString udi;
    QString devHandle;
    foreach ( Solid::Device device, dl )
    {
        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
        Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
        if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
            switch (audiohw->driver()) {
                case Solid::AudioInterface::OpenSoundSystem:
                    devHandle = audiohw->driverHandle().toString();
                    std::cout << ">>> Coldplugged OSS ='" <<  devHandle.toUtf8().data() << "'\n";
                    if ( devname == devHandle ) {
                        found = true;
                        std::cout << ">>> Match!!! Coldplugged OSS ='" <<  devHandle.toUtf8().data() << "'\n";
                        udi = device.udi();
                    }
                     break;
                default:
                    break;
            } // driver type
        } // is an audio control
        if ( found) break;
    } // foreach
    return udi;
}


void KMixDeviceManager::pluggedSlot(const QString& udi) {
   std::cout << "Plugged udi='" <<  udi.toUtf8().data() << "'\n";
   Solid::Device device(udi);
   Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
   if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
       QString dev;
               switch (audiohw->driver()) {
           case Solid::AudioInterface::Alsa:
               dev = audiohw->driverHandle().toList().first().toString();
               std::cout << ">>> Plugged ALSA ='" <<  udi.toUtf8().data() << "'\n";
               emit plugged("ALSA", udi, dev);
               break;
           case Solid::AudioInterface::OpenSoundSystem:
               dev = audiohw->driverHandle().toString();
               std::cout << ">>> Plugged OSS ='" <<  udi.toUtf8().data() << "'\n";
               emit plugged("OSS", udi, dev);
                break;
           default:
               std::cout << ">>> Plugged UNKNOWN (ignored)\n";
               break;
       }
    }
}


void KMixDeviceManager::unpluggedSlot(const QString& udi) {
    std::cout << "Unplugged udi='" <<  udi.toUtf8().data() << "'\n";
    Solid::Device device(udi);
    // At this point the device has already been unplugged by the user. Solid doesn't know anything about the
    // device except the UDI (not even device.as<Solid::AudioInterface>() is possible). Thus I'll forward any
    // unplugging action (could e.g. also be HID or mass storage). The receiver of the signal as to deal with it,
    // but a simple UDI matching is enough.
    emit unplugged(udi);

}


#include "kmixdevicemanager.moc"

