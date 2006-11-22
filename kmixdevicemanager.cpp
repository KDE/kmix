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
#include <solid/devicemanager.h>
#include <solid/capability.h>
#include <solid/audiohw.h>

#include "kmixd.h"
#include "version.h"

static const char description[] =
I18N_NOOP("kmixd - Soundcard Mixer Device Manager");


kdm::kdm()
{
  std::cerr << "--- before getting dm ---\n";
  Solid::DeviceManager& dm = Solid::DeviceManager::self();
  connect (&dm, SIGNAL(deviceAdded(const QString&)), SLOT(plugged(const QString&)) );

    std::cerr << "--- before dm.allDevices() ---\n";
//  Solid::DeviceList dl = dm.allDevices();
  Solid::DeviceList dl = dm.findDevicesFromQuery(QString(), Solid::Capability::AudioHw );

   foreach ( Solid::Device device, dl )
   {
      std::cout << "udi = '" << device.udi().toUtf8().data() << "'\n";
      QMap<QString,QVariant> properties = device.allProperties();
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

