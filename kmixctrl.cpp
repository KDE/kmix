/*
 * kmixctrl - kmix volume save/restore utility
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <qptrlist.h>

#include "mixer.h"
#include "version.h"

static const char description[] =
I18N_NOOP("kmixctrl - kmix volume save/restore utility");

static KCmdLineOptions options[] =
{
   { "s", 0, 0 },
   { "save", I18N_NOOP("Save current volumes as default"), 0 },
   { "r", 0, 0 },
   { "restore", I18N_NOOP("Restore default volumes"), 0 },
   KCmdLineLastOption
   // INSERT YOUR COMMANDLINE OPTIONS HERE
};

extern "C" int kdemain(int argc, char *argv[])
{
   KLocale::setMainCatalogue("kmix");
   KAboutData aboutData( "kmixctrl", I18N_NOOP("KMixCtrl"),
			 APP_VERSION, description, KAboutData::License_GPL,
			 "(c) 2000 by Stefan Schimanski");

   aboutData.addAuthor("Stefan Schimanski", 0, "1Stein@gmx.de");

   KCmdLineArgs::init( argc, argv, &aboutData );
   KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
   KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
   KApplication app( false, false );

   // get maximum values
   KConfig *config= new KConfig("kmixrc", true, false);
   config->setGroup("Misc");
   int maxDevices = 32;
   delete config;

   // create mixers
   QPtrList<Mixer> mixers;
   int drvNum = Mixer::getDriverNum();

   // Following loop iterates over all soundcard drivers (e.g. OSS, ALSA).
   // As soon as a card is found by one driver (mixers.count()==0) the detection stops.
   // Even if there are more drivers, they are not queried (due to "mixers.count()==0" check).
   for( int drv=0; drv<drvNum && mixers.count()==0; drv++ )
   {
       for ( int dev=0; dev<maxDevices; dev++ )
       {
	   Mixer *mixer = Mixer::getMixer( drv, dev, 0 );
	   int mixerError = mixer->grab();
	   if ( mixerError!=0 ) {
	       delete mixer;
               if ( mixerError == Mixer::ERR_MIXEROPEN )
                   continue; // there was a soundcard, but no mixer => continue autodetection
               else
	           break;  // no more soundcards in this driver => skip the other devices
	   }
	   else {
	       mixers.append( mixer );
	   }
       } // for all devices (Soundcards) of this driver
   } // for all drivers

   // load volumes
   if ( args->isSet("restore") )
   {
       for (Mixer *mixer=mixers.first(); mixer!=0; mixer=mixers.next()) {
	   mixer->volumeLoad( KGlobal::config() );
       }
   }

   // save volumes
   if ( args->isSet("save") )
   {
      for (Mixer *mixer=mixers.first(); mixer!=0; mixer=mixers.next())
	 mixer->volumeSave( KGlobal::config() );
   }

   return 0;
}
