/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
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

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include "KMixApp.h"
#include "version.h"

static const char description[] =
I18N_NOOP("KMix - KDE's full featured mini mixer");

extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
{
   KAboutData aboutData( "kmix", 0, ki18n("KMix"),
                         APP_VERSION, ki18n(description), KAboutData::License_GPL,
                         ki18n("(c) 1996-2000 Christian Esken\n(c) 2000-2003 Christian Esken, Stefan Schimanski\n(c) 2002-2005 Christian Esken, Helio Chissini de Castro"));

   aboutData.addAuthor(ki18n("Christian Esken"), ki18n("Current maintainer"), "esken@kde.org");
   aboutData.addAuthor(ki18n("Helio Chissini de Castro"), ki18n("Co-maintainer, Alsa 0.9x port"), "helio@kde.org" );
   aboutData.addAuthor(ki18n("Brian Hanson")      , ki18n("Solaris port"), "bhanson@hotmail.com");
/* The SGI and HP/UX ports are not maintained anymore, and no official part of KMix anymore
   aboutData.addAuthor(ki18n("Paul Kendall")      , ki18n("SGI Port"), "paul@orion.co.nz");
   aboutData.addAuthor(ki18n("Helge Deller")      , ki18n("HP/UX port"), "deller@gmx.de");
*/
   aboutData.addCredit(ki18n("Stefan Schimanski") , ki18n("Temporary maintainer"), "schimmi@kde.org");
   aboutData.addCredit(ki18n("Erwin Mascher")     , ki18n("Improving support for emu10k1 based soundcards"));
   aboutData.addCredit(ki18n("Sebestyen Zoltan")  , ki18n("*BSD fixes"), "szoli@digo.inf.elte.hu");
   aboutData.addCredit(ki18n("Lennart Augustsson"), ki18n("*BSD fixes"), "augustss@cs.chalmers.se");
   aboutData.addCredit(ki18n("Nick Lopez")        , ki18n("ALSA port"), "kimo_sabe@usa.net");
   aboutData.addCredit(ki18n("Nadeem Hasan")      , ki18n("Mute and volume preview, other fixes"), "nhasan@kde.org");

   KCmdLineArgs::init( argc, argv, &aboutData );

   KCmdLineOptions options;
   options.add("keepvisibility", ki18n("Inhibits the unhiding of the KMix main window, if KMix is already running."));
   KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
   KUniqueApplication::addCmdLineOptions();

   KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
   bool hasArgKeepvisibility = args->isSet("keepvisibility");
   KMixApp::keepVisibility(hasArgKeepvisibility);

   if (!KMixApp::start())
       return 0;

   KMixApp *app = new KMixApp();
   int ret = app->exec();
   delete app;
   return ret;
}
