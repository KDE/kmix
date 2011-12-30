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
#include <kdebug.h>

#include <iostream>

#include <QRegExp>
#include <QString>


#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/audiointerface.h>

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
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), SLOT(pluggedSlot(QString)) );
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), SLOT(unpluggedSlot(QString)) );
}

QString KMixDeviceManager::getUDI_ALSA(int num)
{
    QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

    QString numString;
    numString.setNum(num);
    bool found = false;
    QString udi;
    QString devHandle;
    foreach ( const Solid::Device &device, dl )
    {
//        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
        const Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
        if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
            switch (audiohw->driver()) {
                case Solid::AudioInterface::Alsa:
                    devHandle = audiohw->driverHandle().toList().first().toString();
//                    std::cout << ">>> Coldplugged ALSA ='" <<  devHandle.toUtf8().data() << "'\n";
                    if ( numString == devHandle ) {
                        found = true;
//                        std::cout << ">>> Match!!! Coldplugged ALSA ='" <<  devHandle.toUtf8().data() << "'\n";
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

QString KMixDeviceManager::getUDI_OSS(const QString& devname)
{
    QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

    bool found = false;
    QString udi;
    QString devHandle;
    foreach ( const Solid::Device &device, dl )
    {
//        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
        const Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
        if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
            switch (audiohw->driver()) {
                case Solid::AudioInterface::OpenSoundSystem:
                    devHandle = audiohw->driverHandle().toString();
//                    std::cout << ">>> Coldplugged OSS ='" <<  devHandle.toUtf8().data() << "'\n";
                    if ( devname == devHandle ) {
                        found = true;
//                        std::cout << ">>> Match!!! Coldplugged OSS ='" <<  devHandle.toUtf8().data() << "'\n";
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
//   std::cout << "Plugged udi='" <<  udi.toUtf8().data() << "'\n";
   Solid::Device device(udi);
   Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
   if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
       QString dev;
       QRegExp devExpr( QLatin1String( "^\\D+(\\d+)$" ));
        switch (audiohw->driver()) {
           case Solid::AudioInterface::Alsa:
               if ( _hotpluggingBackend == "ALSA" || _hotpluggingBackend == "*" ) {
                    dev = audiohw->driverHandle().toList().first().toString();
                    emit plugged("ALSA", udi, dev);
               }
               break;
           case Solid::AudioInterface::OpenSoundSystem:
                if ( _hotpluggingBackend == "OSS" || _hotpluggingBackend == "*" ) {
                    dev = audiohw->driverHandle().toString();
                    if ( devExpr.indexIn(dev) > -1 ) {
                        dev = devExpr.cap(1); // Get device number from device name (e.g "/dev/mixer1" or "/dev/sound/mixer2")
                    }
                    else {
                        dev = '0'; // "/dev/mixer" or "/dev/sound/mixer"
                    }
                    emit plugged("OSS", udi, dev);
                }
                break;
           default:
               kError(67100) <<  "Plugged UNKNOWN Audio device (ignored)";
               break;
       }
    }
}


void KMixDeviceManager::unpluggedSlot(const QString& udi) {
//    std::cout << "Unplugged udi='" <<  udi.toUtf8().data() << "'\n";
    Solid::Device device(udi);
    // At this point the device has already been unplugged by the user. Solid doesn't know anything about the
    // device except the UDI (not even device.as<Solid::AudioInterface>() is possible). Thus I'll forward any
    // unplugging action (could e.g. also be HID or mass storage). The receiver of the signal as to deal with it,
    // but a simple UDI matching is enough.
    emit unplugged(udi);

}


#include "kmixdevicemanager.moc"

