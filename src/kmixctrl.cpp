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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "core/mixertoolbox.h"
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdebug.h>

#include "gui/kmixtoolbox.h"
#include "core/mixer.h"
#include "core/version.h"

static const char description[] =
I18N_NOOP("kmixctrl - kmix volume save/restore utility");

extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
{
   KLocale::setMainCatalog("kmix");
   KAboutData aboutData( "kmixctrl", 0, ki18n("KMixCtrl"),
			 APP_VERSION, ki18n(description), KAboutData::License_GPL,
			 ki18n("(c) 2000 by Stefan Schimanski"));

   aboutData.addAuthor(ki18n("Stefan Schimanski"), KLocalizedString(), "1Stein@gmx.de");

   KCmdLineArgs::init( argc, argv, &aboutData );

   KCmdLineOptions options;
   options.add("s");
   options.add("save", ki18n("Save current volumes as default"));
   options.add("r");
   options.add("restore", ki18n("Restore default volumes"));
   KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
   KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
   KApplication app( false );

   // create mixers
   QString dummyStringHwinfo;
   MixerToolBox::instance()->initMixer(false, QList<QString>(), dummyStringHwinfo);

   // load volumes
   if ( args->isSet("restore") )
   {
      for (int i=0; i<Mixer::mixers().count(); ++i) {
         Mixer *mixer = (Mixer::mixers())[i];
         mixer->volumeLoad( KGlobal::config().data() );
      }
   }

   // save volumes
   if ( args->isSet("save") )
   {
      for (int i=0; i<Mixer::mixers().count(); ++i) {
         Mixer *mixer = (Mixer::mixers())[i];
         mixer->volumeSave( KGlobal::config().data() );
      }
   }

   MixerToolBox::instance()->deinitMixer();

   return 0;
}
