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

#include "core/mixertoolbox.h"
#include "core/mixer.h"

#include <QDir>
#include <QWidget>
#include <QString>
#include <QStringBuilder>

#include <klocalizedstring.h>

#include "core/kmixdevicemanager.h"
#include "core/mixdevice.h"


static QRegExp s_ignoreMixerExpression(QStringLiteral("Modem"));

/***********************************************************************************
 Attention:
 This MixerToolBox is linked to the KMix Main Program, the KMix Applet and kmixctrl.
 As we do not want to link in more than necessary to kmixctrl, you are asked
 not to put any GUI classes in here.
 In the case where it is unavoidable, please put them in KMixToolBox.
 ***********************************************************************************/

namespace MixerToolBox
{

enum MultiDriverMode { SINGLE, SINGLE_PLUS_MPRIS2, MULTI };

/**
 * Scan for Mixers in the System. This is the method that implicitly fills the
 * list of Mixer's, which is accessible via the static Mixer::mixer() method.
 *
 * This is run only once during the initialization phase of KMix. It has the following tasks:
 * 1) Coldplug scan, to fill the initial mixer list
 * 2) Rember UDI's, to match them when unplugging a device
 * 3) Find out, which Backend to use (plugin events of other Backends are ignored).
 *
 * @deprecated TODO this method has to go away. Migrate to MultiDriverMode enum
 *
 * @par multiDriverMode Whether the Mixer scan should try more all backends.
 *          'true' means to scan all backends. 'false' means: After scanning the
 *          current backend the next backend is only scanned if no Mixers were found yet.
 * @par backendList Activated backends (typically a value from the kmixrc or a default)
 * @par ref_hwInfoString Here a descriptive text of the scan is returned (Hardware Information)
 */

static void initMixerInternal(MultiDriverMode multiDriverMode, const QStringList &backendList, bool hotplug)
{  
   bool useBackendFilter = ( ! backendList.isEmpty() );
   bool backendMprisFound = false; // only for SINGLE_PLUS_MPRIS2
   bool regularBackendFound = false; // only for SINGLE_PLUS_MPRIS2

   qCDebug(KMIX_LOG) << "multiDriverMode" << multiDriverMode << "backendList" << backendList;

   // Find all mixers and initialize them
   const int drvNum = Mixer::numDrivers();

   int driverWithMixer = -1;
   bool multipleDriversActive = false;

   QString driverInfo;

   for (int drv = 0; drv<drvNum; ++drv)
   {
       const QString driverName = Mixer::driverName(drv);
       if (!driverInfo.isEmpty()) driverInfo += QStringLiteral(",");
       driverInfo += driverName;
   }
   qCDebug(KMIX_LOG) << "Sound drivers supported -" << qPrintable(driverInfo);

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
   QString driverInfoUsed;

   for (int drv = 0; drv<drvNum; ++drv)
   {
      if ( autodetectionFinished )
      {
         // inner loop indicates that we are finished => sane exit from outer loop
         break;
      }

      QString driverName = Mixer::driverName(drv);
      qCDebug(KMIX_LOG) << "Looking for mixers with the" << driverName << "driver";
      if ( useBackendFilter && ! backendList.contains(driverName) )
      {
	  qCDebug(KMIX_LOG) << "Ignored" << driverName << "- filtered by backend list";
	  continue;
      }
      

      bool regularBackend =  driverName != QLatin1String("MPRIS2")  && driverName != QLatin1String("PulseAudio");
      if (regularBackend && regularBackendFound)
      {
	  qCDebug(KMIX_LOG) << "Ignored" << driverName << "- regular backend already found";
    	  // Only accept one regular backend => skip this one
    	  continue;
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
   
         /* Lets decide if the autoprobing shall end.
          * If the user has configured a backend filter, we will use that as a plain list to obey. It overrides the
          * multiDriverMode.
          */
         if ( ! useBackendFilter )
         {
        	 bool foundSomethingAndLastControlReached = dev == devNumMax && ! Mixer::mixers().isEmpty();
			 switch ( multiDriverMode )
			 {
			 case SINGLE:
					// In Single-Driver-mode we only need to check after we reached devNumMax
					if ( foundSomethingAndLastControlReached )
						autodetectionFinished = true; // highest device number of driver and a Mixer => finished
					break;

			 case MULTI:
				 // In multiDriver mode we scan all devices, so we will simply continue
				 break;

			 case SINGLE_PLUS_MPRIS2:
				 if ( driverName == QLatin1String("MPRIS2") )
				 {
					 backendMprisFound = true;
				 }
				 else if ( driverName == QLatin1String("PulseAudio") )
				 {
					 // PulseAudio is not useful together with MPRIS2. Treat it as "single"
					 if ( foundSomethingAndLastControlReached )
						 autodetectionFinished = true;
				 }
				 else
				 {
					 // same check as in SINGLE
					 if ( foundSomethingAndLastControlReached )
						 regularBackendFound = true;
				 }

				 if ( backendMprisFound && regularBackendFound )
					 autodetectionFinished = true; // highest device number of driver and a Mixer => finished
				 break;
			 }
         }
         else
         {
        	 // Using backend filter. This is a plain list to obey.
        	 // Simply continue (and filter at the start of the loop).
         }
      
         if ( mixerAccepted )
         {
             qCDebug(KMIX_LOG) << "Accepted mixer" << mixer->id() << "for the" << driverName << "driver";
            // append driverName (used drivers)
            if ( !drvInfoAppended )
            {
               drvInfoAppended = true;
               if (Mixer::mixers().count()>1) driverInfoUsed += ",";
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
   if ( !Mixer::getGlobalMasterMD(false) ) {
      // We have no master card yet. This actually only happens when there was
      // not one defined in the kmixrc.
      // So lets just set the first card as master card.
      if ( Mixer::mixers().count() > 0 ) {
    	  shared_ptr<MixDevice> master = Mixer::mixers().first()->getLocalMasterMD();
         if ( master ) {
             QString controlId = master->id();
             Mixer::setGlobalMaster( Mixer::mixers().first()->id(), controlId, true);
         }
      }
   }
   else {
      // setGlobalMaster was already set after reading the configuration.
      // So we must make the local master consistent
	  shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
      QString mdID = md->id();
      md->mixer()->setLocalMasterMD(mdID);
   }

   if (Mixer::mixers().count()==0)
   {
       // If there was no mixer found, we assume, that hotplugging will take place
       // on the preferred driver (this is always the first in the backend list).
       driverInfoUsed = Mixer::driverName(0);
   }
   qCDebug(KMIX_LOG) << "Sound drivers used -" << qPrintable(driverInfoUsed);

   if ( multipleDriversActive )
   {
       // this will only be possible by hacking the config-file, as it will not be officially supported
       qCDebug(KMIX_LOG) << "Experimental multiple-driver mode activated";
       if (hotplug) KMixDeviceManager::instance()->setHotpluggingBackends("*");
   }
   else
   {
       if (hotplug) KMixDeviceManager::instance()->setHotpluggingBackends(driverInfoUsed);
   }

   qCDebug(KMIX_LOG) << "Total number of detected mixers" << Mixer::mixers().count();
}


static void initMixer(MultiDriverMode multiDriverMode, const QStringList &backendList, bool hotplug)
{
    initMixerInternal(multiDriverMode, backendList, hotplug);
    if (Mixer::mixers().isEmpty())			// failed to find any mixers
    {							// try again without filter
        initMixerInternal(multiDriverMode, QStringList(), hotplug);
    }
}


void initMixer(bool multiDriverFlag, const QStringList &backendList, bool hotplug)
{
    MultiDriverMode multiDriverMode = multiDriverFlag ?  MULTI : SINGLE_PLUS_MPRIS2;
    initMixer(multiDriverMode, backendList, hotplug);
}


/**
 * Opens and adds a mixer to the KMix wide Mixer array, if the given Mixer is valid.
 * Otherwise the Mixer is deleted.
 * This method can be used for adding "static" devices (at program start) and also for hotplugging.
 *
 * @arg mixer
 * @returns true if the Mixer was added
 */
bool possiblyAddMixer(Mixer *mixer)
{
    if (mixer->openIfValid())
    {
        if (s_ignoreMixerExpression.isEmpty() || !mixer->id().contains(s_ignoreMixerExpression))
        {
            Mixer::mixers().append(mixer);
            qCDebug(KMIX_LOG) << "Added mixer " << mixer->id();
            return (true);
        }
        else
        {
            // This mixer should be ignored (the default ignore expression is "Modem").
            qCDebug(KMIX_LOG) << "mixer" << mixer->id() << "ignored";
        }
    }

    delete mixer;
    return (false);
}


/* This allows to set an expression form Mixers that should be ignored.
  The default is "Modem", because most people don't want to control the modem volume. */
void setMixerIgnoreExpression(const QString &ignoreExpr)
{
    s_ignoreMixerExpression.setPattern(ignoreExpr);
}

QString mixerIgnoreExpression()
{
     return s_ignoreMixerExpression.pattern( );
}

void removeMixer(Mixer *par_mixer)
{
    for (int i=0; i<Mixer::mixers().count(); ++i) {
        Mixer *mixer = (Mixer::mixers())[i];
        if ( mixer == par_mixer ) {
            qCDebug(KMIX_LOG) << "Removing card " << mixer->id();
            Mixer::mixers().removeAt(i);
            delete mixer;
        }
    }
}



/*
 * Clean up and free all resources of all found Mixers, which were found in the initMixer() call
 */
void deinitMixer()
{
   //qCDebug(KMIX_LOG) << "IN MixerToolBox::deinitMixer()";
   int mixerCount = Mixer::mixers().count();
   for ( int i=0; i<mixerCount; ++i)
   {
      Mixer* mixer = (Mixer::mixers())[i];
      //qCDebug(KMIX_LOG) << "MixerToolBox::deinitMixer() Remove Mixer";
      mixer->close();
      delete mixer;
   }
   Mixer::mixers().clear();
   // qCDebug(KMIX_LOG) << "OUT MixerToolBox::deinitMixer()";
}

}							// namespace MixerToolBox
