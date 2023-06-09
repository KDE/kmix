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

#include <qcommandlineparser.h>
#include <qapplication.h>

#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kdbusservice.h>

#include "kmixapp.h"
#include "kmix_debug.h"

#ifdef HAVE_PULSEAUDIO
#include <pulse/version.h>
#endif

#ifdef HAVE_ALSA_MIXER
#include <alsa/version.h>
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif
#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif


int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication qapp(argc, argv);

    KLocalizedString::setApplicationDomain("kmix");

    KAboutData aboutData("kmix", i18n("KMix"),
                         KMIX_VERSION, i18n("KMix - KDE's full featured mini mixer"), KAboutLicense::GPL,
                         i18n("(c) 1996-2013 The KMix Authors"));

   // Author Policy: Long-term maintainers and backend writers/maintainers go in the Authors list.
   aboutData.addAuthor(i18n("Christian Esken")   , i18n("Original author and current maintainer"), "esken@kde.org");
   aboutData.addAuthor(i18n("Colin Guthrie")     , i18n("PulseAudio support"), "colin@mageia.org");
   aboutData.addAuthor(i18n("Helio Chissini de Castro"), i18n("ALSA 0.9x port"), "helio@kde.org" );
   aboutData.addAuthor(i18n("Brian Hanson")      , i18n("Solaris support"), "bhanson@hotmail.com");
// The HP/UX port is not maintained anymore, and no official part of KMix anymore
// aboutData.addAuthor(i18n("Helge Deller")      , i18n("HP/UX port"), "deller@gmx.de");
// The initial support was for ALSA 0.5. The new code is not based on it IIRC.
// aboutData.addAuthor(i18n("Nick Lopez")        , i18n("Initial ALSA port"), "kimo_sabe@usa.net");

   // Credit Policy: Authors who did a discrete part, like the Dataengine, OSD, help on specific platforms or soundcards.
   aboutData.addCredit(i18n("Jonathan Marten")   , i18n("KF5 hotplugging, ALSA/OSS volume feedback, GUI improvements"), "jonathan.marten@kdemail.net");
   aboutData.addCredit(i18n("Igor Poboiko")      , i18n("Plasma Dataengine"), "igor.poboiko@gmail.com");
   aboutData.addCredit(i18n("Stefan Schimanski") , i18n("Temporary maintainer"), "schimmi@kde.org");
   aboutData.addCredit(i18n("Sebestyen Zoltan")  , i18n("*BSD fixes"), "szoli@digo.inf.elte.hu");
   aboutData.addCredit(i18n("Lennart Augustsson"), i18n("*BSD fixes"), "augustss@cs.chalmers.se");
   aboutData.addCredit(i18n("Nadeem Hasan")      , i18n("Mute and volume preview, other fixes"), "nhasan@kde.org");
   aboutData.addCredit(i18n("Erwin Mascher")     , i18n("Improving support for emu10k1 based soundcards"));
   aboutData.addCredit(i18n("Valentin Rusu")     , i18n("TerraTec DMX6Fire support"), "kde@rusu.info");

   aboutData.setOrganizationDomain(QByteArray("kde.org"));
   aboutData.setDesktopFileName(QStringLiteral("org.kde.kmix"));

#ifdef HAVE_PULSEAUDIO
   // Using pa_get_library_version() here would require the
   // PulseAudio::PulseAudio target to be linked as PUBLIC.
   aboutData.addComponent(i18n("PulseAudio"),
                          i18n("Sound server"),
                          pa_get_headers_version(),
                          "https://www.freedesktop.org/wiki/Software/PulseAudio");
#endif
#ifdef HAVE_ALSA_MIXER
    aboutData.addComponent(i18n("ALSA"),
                           i18n("Advanced Linux Sound Architecture"),
                           SND_LIB_VERSION_STR,
                           "https://alsa-project.org/wiki/Main_Page");
#endif
#ifdef HAVE_OSS_3
    aboutData.addComponent(i18n("OSS 3"),
                           i18n("Open Sound System"),
                           QByteArray::number(SOUND_VERSION, 16));
#endif
#ifdef HAVE_OSS_4
    aboutData.addComponent(i18n("OSS 4"),
                           i18n("Open Sound System"),
                           QByteArray::number(SOUND_VERSION, 16),
                           "http://www.opensound.com/");
#endif

   KAboutData::setApplicationData(aboutData);

   // Set up and parse the command line now, so as to be able to respond to
   // help and version options even if the application is already running.
   QCommandLineParser parser;
   aboutData.setupCommandLine(&parser);
   parser.addOption(QCommandLineOption("keepvisibility", i18n("Inhibit showing the KMix main window, if KMix is already running.")));
   parser.addOption(QCommandLineOption("failsafe", i18n("Start KMix in failsafe mode.")));
   parser.addOption(QCommandLineOption("backends", i18n("A list of backends to use (for testing only)."), i18n("name[,name...]"), QString()));
   parser.addOption(QCommandLineOption("multidriver", i18n("Enable the (experimental) multiple driver mode.")));
   parser.process(qapp);

   // Implement running as a unique application
   KDBusService service(KDBusService::Unique);
   KMixApp kmapp;					// continues here if unique
   QObject::connect(&service, &KDBusService::activateRequested, &kmapp, &KMixApp::newInstance);

   // Now known to be the first or only instance of the application.
   // Parse the command line options applicable to KMix, then start
   // the application main loop.
   kmapp.parseOptions(parser);
   kmapp.newInstance();
   int ret = qapp.exec();

   qCDebug(KMIX_LOG) << "KMix is now exiting, status=" << ret;
   return (ret);
}
