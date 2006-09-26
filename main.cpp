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

static KCmdLineOptions options[] =
{
   { "keepvisibility", I18N_NOOP("Inhibits the unhiding of the KMix main window, if KMix is already running."), 0 },
   KCmdLineLastOption
   // INSERT YOUR COMMANDLINE OPTIONS HERE
};

extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
{
   KAboutData aboutData( "kmix", I18N_NOOP("KMix"),
                         APP_VERSION, description, KAboutData::License_GPL,
                         I18N_NOOP("(c) 1996-2000 Christian Esken\n(c) 2000-2003 Christian Esken, Stefan Schimanski\n(c) 2002-2005 Christian Esken, Helio Chissini de Castro"));

   aboutData.addAuthor("Christian Esken", "Current maintainer", "esken@kde.org");
   aboutData.addAuthor("Helio Chissini de Castro", I18N_NOOP("Co-maintainer, Alsa 0.9x port"), "helio@kde.org" );
   aboutData.addAuthor("Brian Hanson"      , I18N_NOOP("Solaris port"), "bhanson@hotmail.com");
/* The SGI and HP/UX ports are not maintained anymore, and no official part of KMix anymore
   aboutData.addAuthor("Paul Kendall"      , I18N_NOOP("SGI Port"), "paul@orion.co.nz");
   aboutData.addAuthor("Helge Deller"      , I18N_NOOP("HP/UX port"), "deller@gmx.de");
*/
   aboutData.addCredit("Stefan Schimanski" , I18N_NOOP("Temporary maintainer"), "schimmi@kde.org");
   aboutData.addCredit("Erwin Mascher"     , I18N_NOOP("Improving support for emu10k1 based soundcards"), "");
   aboutData.addCredit("Sebestyen Zoltan"  , I18N_NOOP("*BSD fixes"), "szoli@digo.inf.elte.hu");
   aboutData.addCredit("Lennart Augustsson", I18N_NOOP("*BSD fixes"), "augustss@cs.chalmers.se");
   aboutData.addCredit("Nick Lopez"        , I18N_NOOP("ALSA port"), "kimo_sabe@usa.net");
   aboutData.addCredit("Nadeem Hasan"      , I18N_NOOP("Mute and volume preview, other fixes"), "nhasan@kde.org");

   KCmdLineArgs::init( argc, argv, &aboutData );
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
