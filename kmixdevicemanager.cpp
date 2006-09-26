#include "kmixdevicemanager.h"

#include <iostream>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "solid/device.h"
#include "solid/devicemanager.h"
#include "solid/capability.h"
#include "solid/audioiface.h"

#include "version.h"

static const char description[] =
I18N_NOOP("kmixd - Soundcard Mixer Device Manager");

extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
{
   KAboutData aboutData( "kmixd", I18N_NOOP("Soundcard Mixer Device Manager"),
                         APP_VERSION, description, KAboutData::License_GPL,
                         "(c) 2007 by Christian Esken");

   KCmdLineArgs::init( argc, argv, &aboutData );
   //KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
   KApplication app( false );

  std::cerr << "--- before getting dm ---\n";
  Solid::DeviceManager& dm = Solid::DeviceManager::self();
  std::cerr << "--- before dm.allDevices() ---\n";
//  Solid::DeviceList dl = dm.allDevices();
  Solid::DeviceList dl = dm.findDevicesFromQuery("/org/kde/solid/fake/pc", Solid::Capability::AudioIface );
  

  std::cerr << "--- before dl.first() ---\n";
  //Solid::Device dev = dl.first();

  return 0;
}

