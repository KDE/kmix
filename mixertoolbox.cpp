/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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


#include <QDir>
#include <QWidget>
#include <QString>

//#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "guiprofile.h"
#include "kmixdevicemanager.h"
#include "mixdevice.h"
#include "mixer.h"

#include "mixertoolbox.h"


MixerToolBox* MixerToolBox::s_instance      = 0;
QMap<Mixer*,GUIProfile*> MixerToolBox::s_fallbackProfiles;
QRegExp MixerToolBox::s_ignoreMixerExpression("Modem");
//KLocale* MixerToolBox::s_whatsthisLocale = 0;

/***********************************************************************************
 Attention:
 This MixerToolBox is linked to the KMix Main Program, the KMix Applet and kmixctrl.
 As we do not want to link in more than necessary to kmixctrl, you are asked
 not to put any GUI classes in here.
 In the case where it is unavoidable, please put them in KMixToolBox.
 ***********************************************************************************/

MixerToolBox* MixerToolBox::instance()
{
   if ( s_instance == 0 ) {
      s_instance = new MixerToolBox();
//      if ( s_ignoreMixerExpression.isEmpty() )
//          s_ignoreMixerExpression.setPattern("Modem");
   }
   return s_instance;
}


/**
 * Scan for Mixers in the System. This is the method that implicitely fills the
 * list of Mixer's, which is accessible via the static Mixer::mixer() method.
 *
 * This is run only once during the initialization phase of KMix. It has the following tasks:
 * 1) Coldplug scan, to fill the initial mixer list
* 2) Rember UDI's, to match them when unplugging a device
* 3) Find out, which Backend to use (plugin events of other Backends are ignored).
 *
 * @par multiDriverMode Whether the Mixer scan should try more all backendends.
 *          'true' means to scan all backends. 'false' means: After scanning the
 *          current backend the next backend is only scanned if no Mixers were found yet.
 * @par ref_hwInfoString Here a descripitive text of the scan is returned (Hardware Information)
 */
void MixerToolBox::initMixer(bool multiDriverMode, QString& ref_hwInfoString)
{
   //kDebug(67100) << "IN MixerToolBox::initMixer()";

   // Find all mixers and initialize them
   int drvNum = Mixer::numDrivers();

   int driverWithMixer = -1;
   bool multipleDriversActive = false;

   QString driverInfo = "";
   QString driverInfoUsed = "";

   for( int drv1=0; drv1<drvNum; drv1++ )
   {
      QString driverName = Mixer::driverName(drv1);
      if ( driverInfo.length() > 0 ) {
         driverInfo += " + ";
      }
      driverInfo += driverName;
   }
   /* Run a loop over all drivers. The loop will terminate after the first driver which
      has mixers. And here is the reason:
      - If you run ALSA with ALSA-OSS-Emulation enabled, mixers will show up twice: once
         as native ALSA mixer, once as OSS mixer (emulated by ALSA). This is bad and WILL
         confuse users. So it is a design decision that we can compile in multiple drivers
         but we can run only one driver.
      - For special usage scenarios, people will still want to run both drivers at the
         same time. We allow them to hack their Config-File, where they can enable a
         multi-driver mode.
      - Another remark: For KMix3.0 or so, we should allow multiple-driver, for allowing
         addition of special-use drivers, e.g. an Jack-Mixer-Backend, or a CD-Rom volume Backend.
      */
   
   bool autodetectionFinished = false;
   for( int drv=0; drv<drvNum; drv++ )
   {
      QString driverName = Mixer::driverName(drv);
      kDebug(67100) << "Looking for mixers with the : " << driverName << " driver";

      if ( autodetectionFinished ) {
         // inner loop indicates that we are finished => sane exit from outer loop
         break;
      }
   
      bool drvInfoAppended = false;
      // The "19" below is just a "silly" number:
      // (Old: The loop will break as soon as an error is detected - e.g. on 3rd loop when 2 soundcards are installed)
      // New: We don't try be that clever anymore. We now blindly scan 20 cards, as the clever
      // approach doesn't work for the one or other user (e.g. hotplugging might create holes in the list of soundcards).
      int devNumMax = 19;
      for( int dev=0; dev<=devNumMax; dev++ )
      {
         Mixer *mixer = new Mixer( driverName, dev );
         bool mixerAccepted = possiblyAddMixer(mixer);
   
         /* Lets decide if the autoprobing shall end (BTW: In multiDriver mode we scan all devices, so no check is necessary) */
         if ( ! multiDriverMode ) {
            // In Single-Driver-mode we only need to check after we reached devNumMax
            if ( dev == devNumMax && Mixer::mixers().count() != 0 )
                autodetectionFinished = true; // highest device number of driver and a Mixer => finished
        }
      
         if ( mixerAccepted )
         {
            kDebug(67100) << "Success! Found a mixer with the : " << driverName << " driver";
            // append driverName (used drivers)
            if ( !drvInfoAppended )
            {
               drvInfoAppended = true;
               if (  Mixer::mixers().count() > 1)
                  driverInfoUsed += " + ";
               driverInfoUsed += driverName;
            }

            // Check whether there are mixers in different drivers, so that the user can be warned
            if ( !multipleDriversActive )
            {
               if ( driverWithMixer == -1 )
               {
                  // Aha, this is the very first detected device
                  driverWithMixer = drv;
               }
               else if ( driverWithMixer != drv )
               {
                   // Got him: There are mixers in different drivers
                   multipleDriversActive = true;
               }
            } //  !multipleDriversActive
         } // mixerAccepted
      
      } // loop over sound card devices of current driver

      if (autodetectionFinished) {
         break;
      }
   } // loop over soundcard drivers

   
    // Add a master device (if we haven't defined one yet)
   if ( Mixer::getGlobalMasterMD(false) == 0 ) {
      // We have no master card yet. This actually only happens when there was
      // not one defined in the kmixrc.
      // So lets just set the first card as master card.
      if ( Mixer::mixers().count() > 0 ) {
         QString controlId = Mixer::mixers().first()->getLocalMasterMD()->id();
         Mixer::setGlobalMaster( Mixer::mixers().first()->id(), controlId);
      }
   }
   else {
      // setGlobalMaster was already set after reading the configuration.
      // So we must make the local master consistent
      MixDevice* md = Mixer::getGlobalMasterMD();
      QString mdID = md->id();
      md->mixer()->setLocalMasterMD(mdID);
   }



   if ( Mixer::mixers().count() == 0 )
   {
      // If there was no mixer found, we assume, that hotplugging will take place
       // on the preferred driver (this is always the first in the backend list).
      driverInfoUsed = Mixer::driverName(0);
   }

   ref_hwInfoString = i18n("Sound drivers supported:");
   ref_hwInfoString.append(" ").append( driverInfo ).append(	"\n").append(i18n("Sound drivers used:")) .append(" ").append(driverInfoUsed);

   if ( multipleDriversActive )
   {
      // this will only be possible by hacking the config-file, as it will not be officially supported
      ref_hwInfoString += "\nExperimental multiple-Driver mode activated";
      QString allDrivermatch("*");
      KMixDeviceManager::instance()->setHotpluggingBackends(allDrivermatch);
   }
   else {
       KMixDeviceManager::instance()->setHotpluggingBackends(driverInfoUsed);
   }

   kDebug(67100) << ref_hwInfoString << endl << "Total number of detected Mixers: " << Mixer::mixers().count();
   //kDebug(67100) << "OUT MixerToolBox::initMixer()";

}

bool MixerToolBox::possiblyAddMixer(Mixer *mixer)
{
    if ( mixer->openIfValid() )
    {
        if ( (!s_ignoreMixerExpression.isEmpty()) && mixer->id().contains(s_ignoreMixerExpression) ) {
            // This Mixer should be ignored (default expression is "Modem").
            delete mixer;
            mixer = 0;
            return false;
        }
        Mixer::mixers().append( mixer );
        // Count mixer nums for every mixer name to identify mixers with equal names.
        // This is for creating persistent (reusable) primary keys, which can safely
        // be referenced (especially for config file access, so it is meant to be persistent!).
        s_mixerNums[mixer->baseName()]++;
        kDebug(67100) << "mixerNums entry of added mixer is now: " <<  s_mixerNums[mixer->baseName()];
        // Create a useful PK
        /* As we use "::" and ":" as separators, the parts %1,%2 and %3 may not
         * contain it.
         * %1, the driver name is from the KMix backends, it does not contain colons.
         * %2, the mixer name, is typically coming from an OS driver. It could contain colons.
         * %3, the mixer number, is a number: it does not contain colons.
         */
        QString mixerName = mixer->baseName();
        mixerName.replace(":","_");
        QString primaryKeyOfMixer = QString("%1::%2:%3")
                .arg(mixer->getDriverName())
                .arg(mixerName)
                .arg(s_mixerNums[mixer->baseName()]);
        // The following 3 replaces are for not messing up the config file
        primaryKeyOfMixer.replace("]","_");
        primaryKeyOfMixer.replace("[","_"); // not strictly necessary, but lets play safe
        primaryKeyOfMixer.replace(" ","_");
        primaryKeyOfMixer.replace("=","_");
    
        mixer->setID(primaryKeyOfMixer);
        emit mixerAdded(primaryKeyOfMixer);
        return true;
    } // valid
    else
    {
        delete mixer;
        mixer = 0;
        return false;
    } // invalid
}

/* This allows to set an expression form Mixers that should be ignored.
  The default is "Modem", because most people don't want to control the modem volume. */
void MixerToolBox::setMixerIgnoreExpression(QString& ignoreExpr)
{
    s_ignoreMixerExpression.setPattern(ignoreExpr);
}

QString MixerToolBox::mixerIgnoreExpression()
{
     return s_ignoreMixerExpression.pattern( );
}

void MixerToolBox::removeMixer(Mixer *par_mixer)
{
    for (int i=0; i<Mixer::mixers().count(); ++i) {
        Mixer *mixer = (Mixer::mixers())[i];
        if ( mixer == par_mixer ) {
            s_mixerNums[mixer->baseName()]--;
            kDebug(67100) << "mixerNums entry of removed mixer is now: " <<  s_mixerNums[mixer->baseName()];
            Mixer::mixers().removeAt(i);
            delete mixer;
        }
    }
}



/*
 * Clean up and free all resources of all found Mixers, which were found in the initMixer() call
 */
void MixerToolBox::deinitMixer()
{
   //kDebug(67100) << "IN MixerToolBox::deinitMixer()";

   int mixerCount = Mixer::mixers().count();
   for ( int i=0; i<mixerCount; ++i)
   {
      Mixer* mixer = (Mixer::mixers())[i];
      //kDebug(67100) << "MixerToolBox::deinitMixer() Remove Mixer";
      mixer->close();
      delete mixer;
   }
   Mixer::mixers().clear();
   // kDebug(67100) << "OUT MixerToolBox::deinitMixer()";
}

/*
 * Find a Mixer. If there is no mixer with the given id, 0 is returned
 */
Mixer* MixerToolBox::find( const QString& mixer_id)
{
   //kDebug(67100) << "IN MixerToolBox::find()";

   Mixer* mixer = 0;
   int mixerCount = Mixer::mixers().count();
   for ( int i=0; i<mixerCount; ++i)
   {
      if ( ((Mixer::mixers())[i])->id() == mixer_id )
      {
         mixer = (Mixer::mixers())[i];
         break;
      }
   }

   return mixer;
   // kDebug(67100) << "OUT MixerToolBox::find()";
}

/*
KLocale* MixerToolBox::whatsthisControlLocale()
{
   if ( s_whatsthisLocale == 0 ) {
	  s_whatsthisLocale = new KLocale("kmix-controls");
   }
   return s_whatsthisLocale;
}
*/

/**
   Returns a GUI Profile
  */
GUIProfile* MixerToolBox::selectProfile(Mixer* mixer)
{
   /** This is a two-step process *****************************************
      * (1) Build a list of all files we want to check
      * (2) Evaluate all files and keep the best
      ***********************************************************************/
 
   GUIProfile* guiprofBest = 0;
   unsigned long matchValueBest = 0;
   unsigned long matchValueTemp = 0;
   
   QString userProfileDir = KStandardDirs::locateLocal("appdata", "profiles/" );

   QString mixerNameSpacesToUnderscores = mixer->baseName();
   mixerNameSpacesToUnderscores.replace(" ","_");

   // (1) User profile Directory
   QDir dir(userProfileDir);
   dir.setFilter(QDir::Files);
   dir.setNameFilters(QStringList(mixer->getDriverName() + "." + mixerNameSpacesToUnderscores + "*.xml"));
   QFileInfoList fileList = dir.entryInfoList();

   QString fileNamePrefix = "profiles/" + mixer->getDriverName() + ".";

   // (2) Default profile for Soundcard Driver (usually from system Directory)
   QString fileName = fileNamePrefix + "default.xml";
   QString fileNameFQ;
   fileNameFQ = KStandardDirs::locate("appdata", fileName );
kDebug() << fileName << "; fnfq1=" << fileNameFQ;
   if (!fileNameFQ.isEmpty())
       fileList.insert(0, QFileInfo(fileNameFQ));

   // (3) Soundcard specific profile (usually from system Directory)
   fileName = fileNamePrefix + mixerNameSpacesToUnderscores + ".xml";
   fileNameFQ = KStandardDirs::locate("appdata", fileName );
kDebug() << fileName << "; fnfq2=" << fileNameFQ;
   if (!fileNameFQ.isEmpty())
      fileList.insert(0, QFileInfo(fileNameFQ));



	for (int i = 0; i < fileList.size(); ++i) {
		QFileInfo fileInfo = fileList.at(i);
		QString fileNameAbs = fileInfo.absoluteFilePath();
		QString fileNameRelToProfile = "profiles/" + fileInfo.fileName();
		kDebug() << i << ": Try user profile " << fileNameAbs;
		GUIProfile* guiprofTemp = new GUIProfile();
		if ( guiprofTemp->readProfile(fileNameAbs, fileNameRelToProfile) ) {
			matchValueTemp = guiprofTemp->match(mixer);
			if ( matchValueTemp < matchValueBest ) {
				delete guiprofTemp;
				guiprofTemp = 0;
				matchValueTemp = 0;
			}
			else {
				guiprofBest = guiprofTemp;
				matchValueBest = matchValueTemp;
			}
		}
	}


   if ( guiprofBest == 0 ) {
      // Still no profile found. This should usually not happen. This means one of the following things:
      // a) The KMix installation is not OK
      // b) The user has a defective profile in ~/.kde/share/apps/kmix/profiles/
      // c) It is a Backend that ships no default profile (currently this is only Mixer_SUN and Mixer_PULSE)
      guiprofBest = fallbackProfile(mixer);
   }
//    kDebug(67100) << "New Best    =" << matchValueBest << " pointer=" << guiprofBest << "\n";

   return guiprofBest;
}


GUIProfile* MixerToolBox::fallbackProfile(Mixer *mixer)
{
   if ( ! s_fallbackProfiles.contains(mixer) ) {
      GUIProfile *fallback = new GUIProfile();

      ProfProduct* prd = new ProfProduct();
      prd->vendor         = mixer->getDriverName();
      prd->productName    = mixer->readableName();
      prd->productRelease = "1.0";
      fallback->_products.insert(prd);

      ProfControl* ctl = new ProfControl();
      ctl->id          = ".*";
      ctl->regexp      = ".*";   // make sure id matches the regexp
      ctl->subcontrols = ".*";
      ctl->show        = "simple";
      fallback->_controls.push_back(ctl);

      fallback->_soundcardDriver = mixer->getDriverName();
      fallback->_soundcardName   = mixer->readableName();

      fallback->finalizeProfile();
      s_fallbackProfiles[mixer] = fallback;
   }
   return s_fallbackProfiles[mixer];
}

#include "mixertoolbox.moc"
