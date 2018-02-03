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

#include <qcoreapplication.h>
#include <qcommandlineparser.h>

#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kconfig.h>

#include "gui/kmixtoolbox.h"
#include "core/GlobalConfig.h"
#include "core/mixer.h"
#include "core/version.h"

static const char description[] =
I18N_NOOP("kmixctrl - kmix volume save/restore utility");

extern "C" int
Q_DECL_EXPORT
kdemain(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kmix");

    KAboutData aboutData("kmixctrl", i18n("KMixCtrl"),
			 APP_VERSION, i18n(description),
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

   GlobalConfig::init();

   // create mixers
   QString dummyStringHwinfo;
   MixerToolBox::instance()->initMixer(false, QList<QString>(), dummyStringHwinfo, false);

   // load volumes
   if ( parser.isSet("restore") )
   {
      for (int i=0; i<Mixer::mixers().count(); ++i) {
         Mixer *mixer = (Mixer::mixers())[i];
         mixer->volumeLoad(KSharedConfig::openConfig().data());
      }
   }

   // save volumes
	if (parser.isSet("save"))
	{
		for (int i = 0; i < Mixer::mixers().count(); ++i)
		{
			Mixer *mixer = (Mixer::mixers())[i];
                        KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
			qCDebug(KMIX_LOG) << "save " << cfg->name();
			mixer->volumeSave(cfg.data());
		}
	}

   MixerToolBox::instance()->deinitMixer();

   return 0;
}
