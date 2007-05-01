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
#include <QTimer>
#include <QObject>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/audiointerface.h>

#include "kmixd.h"
#include "version.h"

static const char description[] =
I18N_NOOP("kmixd - Soundcard Mixer Device Manager");


kdm::kdm()
{
  std::cerr << "--- before getting dm ---\n";
  connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), SLOT(plugged(const QString&)) );

    std::cerr << "--- before dm.allDevices() ---\n";
//  QList<Solid::Device> dl = Solid::Device::allDevices();
    QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

   foreach ( Solid::Device device, dl )
   {
      std::cout << "udi = '" << device.udi().toUtf8().data() << "'\n";
      //QMap<QString,QVariant> properties = device.allProperties();
      //std::cout << properties << "\n";
   }

  QTimer* tim = new QTimer();
  connect(tim, SIGNAL(timeout()), SLOT(tick()));
}

void kdm::plugged(const QString& udi) {
   std::cout << "Plugged udi='" <<  udi.toUtf8().data() << "'\n";
}

void kdm::tick()
{
  std::cout << "kdm::tick()\n";
}

extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
{
  KAboutData aboutData( "kmixd", I18N_NOOP("Soundcard Mixer Device Manager"),
                         APP_VERSION, description, KAboutData::License_GPL,
                         "(c) 2007 by Christian Esken");

  KCmdLineArgs::init( argc, argv, &aboutData );
  //KApplication app( false );

  KMixD *app = new KMixD();
  int ret = app->exec();
  delete app;

  return ret;
}

#include "kmixdevicemanager.moc"

