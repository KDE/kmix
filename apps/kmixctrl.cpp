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

#include <qcoreapplication.h>
#include <qcommandlineparser.h>

#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kconfig.h>

#include "gui/kmixtoolbox.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "settings.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kmix");

    KAboutData aboutData("kmixctrl", i18n("KMixCtrl"),
                         KMIX_VERSION, i18n("kmixctrl - kmix volume save/restore utility"),
			 KAboutLicense::GPL,
			 i18n("(c) 2000 by Stefan Schimanski"));

   aboutData.addAuthor(i18n("Stefan Schimanski"), QString(), "1Stein@gmx.de");
   KAboutData::setApplicationData(aboutData);

   QCommandLineParser parser;
   aboutData.setupCommandLine(&parser);
   parser.addOption(QCommandLineOption((QStringList() << "s" << "save"),
                                       i18n("Save current volumes as default")));
   parser.addOption(QCommandLineOption((QStringList() << "r" << "restore"),
                                       i18n("Restore default volumes")));
   parser.process(app);

   // create mixers
   MixerToolBox::initMixer(false);

   // load volumes
   if ( parser.isSet("restore") )
   {
       for (Mixer *mixer : qAsConst(MixerToolBox::mixers()))
       {
           mixer->volumeLoad(Settings::self()->config());
       }
   }

   // save volumes
   if (parser.isSet("save"))
   {
       for (const Mixer *mixer : qAsConst(MixerToolBox::mixers()))
       {
           mixer->volumeSave(Settings::self()->config());
       }
   }

   MixerToolBox::deinitMixer();

   return 0;
}
