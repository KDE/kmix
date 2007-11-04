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
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), SLOT(plugged(const QString&)) );
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString&)), SLOT(unplugged(const QString&)) );
}

QString KMixDeviceManager::getUDI_ALSA(int num)
{
    QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

    QString numString;
    numString.setNum(num);
    bool found = false;
    QString udi;
    foreach ( Solid::Device device, dl )
    {
        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
        Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
        if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
            switch (audiohw->driver()) {
                case Solid::AudioInterface::Alsa:
                    udi = audiohw->driverHandle().toList().first().toString();
                    std::cout << ">>> Coldplugged ALSA ='" <<  udi.toUtf8().data() << "'\n";
                    if ( numString == udi ) {
                        found = true;
                        std::cout << ">>> Match!!! Coldplugged ALSA ='" <<  udi.toUtf8().data() << "'\n";
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
    foreach ( Solid::Device device, dl )
    {
        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
        Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
        if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
            switch (audiohw->driver()) {
                case Solid::AudioInterface::OpenSoundSystem:
                    udi = audiohw->driverHandle().toString();
                    std::cout << ">>> Coldplugged OSS ='" <<  udi.toUtf8().data() << "'\n";
                    if ( devname == udi ) {
                        found = true;
                        std::cout << ">>> Match!!! Coldplugged OSS ='" <<  udi.toUtf8().data() << "'\n";
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

void KMixDeviceManager::plugged(const QString& udi) {
   std::cout << "Plugged udi='" <<  udi.toUtf8().data() << "'\n";
   Solid::Device device(udi);
   Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
   if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
       switch (audiohw->driver()) {
           case Solid::AudioInterface::Alsa:
               std::cout << ">>> Plugged ALSA ='" <<  audiohw->driverHandle().toList().first().toString().toUtf8().data() << "'\n";
               break;
           case Solid::AudioInterface::OpenSoundSystem:
               std::cout << ">>> Plugged OSS ='" <<  audiohw->driverHandle().toString().toUtf8().data() << "'\n";
               break;
           default:
               std::cout << ">>> Plugged UNKNOWN \n";
               break;
       }
    }
}

void KMixDeviceManager::unplugged(const QString& udi) {
   std::cout << "Unplugged udi='" <<  udi.toUtf8().data() << "'\n";
   Solid::Device device(udi);
   Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
   if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
       switch (audiohw->driver()) {
           case Solid::AudioInterface::Alsa:
               std::cout << ">>> Unplugged ALSA ='" <<  audiohw->driverHandle().toList().first().toString().toUtf8().data() << "'\n";
               break;
           case Solid::AudioInterface::OpenSoundSystem:
               std::cout << ">>> Unplugged OSS ='" <<  audiohw->driverHandle().toString().toUtf8().data() << "'\n";
               break;
           default:
               std::cout << ">>> Unplugged UNKNOWN \n";
               break;
       }
    }
}


/*
extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
{
  KAboutData aboutData( "kmixd", 0, ki18n("Soundcard Mixer Device Manager"),
                         APP_VERSION, ki18n(description), KAboutData::License_GPL,
                         ki18n("(c) 2007 by Christian Esken"));

  KCmdLineArgs::init( argc, argv, &aboutData );
  //KApplication app( false );

  KMixD *app = new KMixD();
  int ret = app->exec();
  delete app;

  return ret;
}
*/

#include "kmixdevicemanager.moc"

