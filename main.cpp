/*
 * KMix -- KDE's full featured mini mixer
 *
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>

#include "kmix.h"
#include "version.h"

static const char *description =
I18N_NOOP("KMix - KDE's full featured mini mixer");
	
static KCmdLineOptions options[] =
{
   { 0, 0, 0 }
   // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{
   KAboutData aboutData( "kmix", I18N_NOOP("KMix"),
			 APP_VERSION, description, KAboutData::License_GPL,
			 "(c) 2000 by Stefan Schimanski");

   aboutData.addAuthor("Stefan Schimanski", "GUI", "1Stein@gmx.de");
   aboutData.addAuthor("Christian Esken", 0, "esken@kde.org");
   aboutData.addAuthor("Paul Kendall", "SGI Port", "paul@orion.co.nz");
   aboutData.addAuthor("Sebestyen Zoltan", "*BSD fixes", "szoli@digo.inf.elte.hu");
   aboutData.addAuthor("Lennart Augustsson", "*BSD fixes", "augustss@cs.chalmers.se");
   aboutData.addAuthor("Nick Lopez", "ALSA port", "kimo_sabe@usa.net");
   aboutData.addAuthor("Helge Deller", "HP/UX port", "deller@gmx.de");
   	
   KCmdLineArgs::init( argc, argv, &aboutData );
   KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.	

   KApplication app;
   KGlobal::dirs()->addResourceType("icon", KStandardDirs::kde_default("data") + "kmix/pics");

   if (app.isRestored())
   {
      RESTORE(KMixApp);
   }
   else 
   {
      KMixApp *kmix = new KMixApp();
      kmix->show();

      KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		
      args->clear();
   }

   return app.exec();
}  
