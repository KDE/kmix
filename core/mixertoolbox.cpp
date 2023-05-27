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

#include <QStringBuilder>

#include "core/mixer.h"
#include "core/kmixdevicemanager.h"
#include "core/mixdevice.h"

// This file contains global data for the available backends.
#include "backends/kmix-backends.cpp"


static QRegExp s_ignoreMixerExpression(QStringLiteral("Modem"));

static QList<Mixer *> s_allMixers;

static MasterControl s_globalMasterCurrent;
static MasterControl s_globalMasterPreferred;

/***********************************************************************************
 Attention:
 This MixerToolBox is linked to the KMix Main Program, the KMix Applet and kmixctrl.
 As we do not want to link in more than necessary to kmixctrl, you are asked
 not to put any GUI classes in here.
 In the case where it is unavoidable, please put them in KMixToolBox.
 ***********************************************************************************/

enum MultiDriverMode { SINGLE, SINGLE_PLUS_MPRIS2, MULTI };


/**
 * Query the backend factory list for the backend name corresponding
 * to the specified @p driverIndex in the list.
 *
 * @param driverIndex Index in list, 0 <= driver < numDrivers()
 */
static QString backendNameFor(int driverIndex)
{
    return (g_mixerFactories[driverIndex].backendName);
}


/**
 * Scan for Mixer devices on the system. This fills the list of Mixer's which is
 * accessible via Mixer::mixers().
 *
 * This is run only once during the initialization phase of KMix. It performs
 * the following tasks:
 *
 * 1) Coldplug scan, to fill the initial mixer list
 * 2) Resolve which backend to use (plugin events of other Backends are ignored).
 *
 * @param multiDriverMode Whether the scan should try to scan for more backends
 * once one "regular" backand has been found.  "Regular" means a backend which
 * has a 1-1 correspndence between an instance and a hardware device (card).
 * Therefore ALSA and OSS are "regular" backends but PulseAudio is not.
 * @c MULTI means to scan all backends.  @c SINGLE means to only scan until a
 * "regular" backend to use has been found.  @c SINGLE_PLUS_MPRIS2 is the same
 * as @c SINGLE but also accepts the MPRIS2 backend.
 *
 * @param backendList A list of allowed backends (typically a value from kmixrc or
 * a default).  A null list meanbs to accept any backend.
 *
 * @param hotplug Whether hotplugging is expected to happen for the detected backends.
 * The KMix application and the KDED module set this to @c true, while kmixctrl
 * sets this to @c false because it is only a transient utility.
 */
static void initMixerInternal(MultiDriverMode multiDriverMode, const QStringList &backendList, bool hotplug)
{  
   bool useBackendFilter = ( ! backendList.isEmpty() );
   bool backendMprisFound = false; // only for SINGLE_PLUS_MPRIS2
   bool regularBackendFound = false; // only for SINGLE_PLUS_MPRIS2

   QByteArray multiModeString;
   switch (multiDriverMode)
   {
case SINGLE:			multiModeString = "SINGLE";				break;
case SINGLE_PLUS_MPRIS2:	multiModeString = "SINGLE_PLUS_MPRIS2";			break;
case MULTI:			multiModeString = "MULTI";				break;
default:			multiModeString = QByteArray::number(multiDriverMode);	break;
   }
   qCDebug(KMIX_LOG) << "multiDriverMode" << qPrintable(multiModeString) << "backendList" << backendList;

   // Find all mixers and initialize them

   int driverWithMixer = -1;
   bool multipleDriversActive = false;

   QString driverInfo;
   for (int drv = 0; drv<numBackends; ++drv)
   {
       const QString driverName = backendNameFor(drv);
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

   for (int drv = 0; drv<numBackends; ++drv)
   {
      if ( autodetectionFinished )
      {
         // inner loop indicates that we are finished => sane exit from outer loop
         break;
      }

      QString driverName = backendNameFor(drv);
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
         bool mixerAccepted = MixerToolBox::possiblyAddMixer(mixer);
   
         /* Lets decide if the autoprobing shall end.
          * If the user has configured a backend filter, we will use that as a plain list to obey. It overrides the
          * multiDriverMode.
          */
         if ( ! useBackendFilter )
         {
		 bool foundSomethingAndLastControlReached = dev==devNumMax && !s_allMixers.isEmpty();
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
               if (s_allMixers.count()>1) driverInfoUsed += ",";
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
   if ( !MixerToolBox::getGlobalMasterMD(false) ) {
      // We have no master card yet. This actually only happens when there was
      // not one defined in the kmixrc.
      // So lets just set the first card as master card.
       if (!s_allMixers.isEmpty()) {
	  shared_ptr<MixDevice> master = s_allMixers.first()->getLocalMasterMD();
         if ( master ) {
             QString controlId = master->id();
             MixerToolBox::setGlobalMaster(s_allMixers.first()->id(), controlId, true);
         }
      }
   }
   else {
      // setGlobalMaster was already set after reading the configuration.
      // So we must make the local master consistent
	  shared_ptr<MixDevice> md = MixerToolBox::getGlobalMasterMD();
      QString mdID = md->id();
      md->mixer()->setLocalMasterMD(mdID);
   }

   if (s_allMixers.isEmpty())
   {
       // If there was no mixer found, we assume, that hotplugging will take place
       // on the preferred driver (which is always the first in the backend list).
       driverInfoUsed = backendNameFor(0);
   }
   qCDebug(KMIX_LOG) << "Sound drivers used -" << qPrintable(driverInfoUsed);

   if (multipleDriversActive)
   {
       // Note: Even if only the ALSA or OSS driver is in use, this will be
       // true because MPRIS2 also counts as a driver above.  However this
       // does not actually change anything, because the backend names given
       // to KMixDeviceManager::setHotpluggingBackends() are ignored.
       //
       // The original comment here, which I don't understand the relevance
       // of, said:
       //   this will only be possible by hacking the config-file,
       //   as it will not be officially supported
       qCDebug(KMIX_LOG) << "Experimental multiple-driver mode activated";
       if (hotplug) KMixDeviceManager::instance()->setHotpluggingBackends("*");
   }
   else
   {
       if (hotplug) KMixDeviceManager::instance()->setHotpluggingBackends(driverInfoUsed);
   }

   qCDebug(KMIX_LOG) << "Total detected mixers" << s_allMixers.count();
}


static void initMixer(MultiDriverMode multiDriverMode, const QStringList &backendList, bool hotplug)
{
    initMixerInternal(multiDriverMode, backendList, hotplug);
    if (s_allMixers.isEmpty())				// failed to find any mixers,
    {							// try again without filter
        initMixerInternal(multiDriverMode, QStringList(), hotplug);
    }
}


void MixerToolBox::initMixer(bool multiDriverFlag, const QStringList &backendList, bool hotplug)
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
bool MixerToolBox::possiblyAddMixer(Mixer *mixer)
{
    if (mixer==nullptr) return (false);
    if (mixer->openIfValid())
    {
        if (s_ignoreMixerExpression.isEmpty() || !mixer->id().contains(s_ignoreMixerExpression))
        {
            s_allMixers.append(mixer);
            qCDebug(KMIX_LOG) << "Added mixer" << mixer->id();
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
void MixerToolBox::setMixerIgnoreExpression(const QString &ignoreExpr)
{
    s_ignoreMixerExpression.setPattern(ignoreExpr);
}

QString MixerToolBox::mixerIgnoreExpression()
{
    return s_ignoreMixerExpression.pattern();
}


void MixerToolBox::removeMixer(Mixer *mixer)
{
    qCDebug(KMIX_LOG) << "Removing mixer" << mixer->id();
    s_allMixers.removeAll(mixer);
    delete mixer;
}


// Clean up and free all resources of all currently known 'Mixer's,
// which were found in the initMixer() call.
void MixerToolBox::deinitMixer()
{
    qCDebug(KMIX_LOG);
    while (!s_allMixers.isEmpty())
    {
        Mixer *mixer = s_allMixers.takeFirst();
        if (mixer!=nullptr) mixer->close();
        delete mixer;
    }
}


const QList<Mixer *> &MixerToolBox::mixers()
{
    return (s_allMixers);
}


Mixer *MixerToolBox::findMixer(const QString &mixerId)
{
    const int mixerCount = s_allMixers.count();
    for (int i = 0; i<mixerCount; ++i)
    {
        Mixer *mix = s_allMixers[i];
        if (mix->id()==mixerId) return (mix);
    }

    return (nullptr);
}


/**
 * Set the global master, which is shown in the dock area and which is accessible via the
 * DBUS masterVolume() method.
 *
 * The parameters are taken over as-is, this means without checking for validity.
 * This allows the User to define a master card that is not always available
 * (e.g. it is an USB hotplugging device). Also you can set the master at any time you
 * like, e.g. after reading the KMix configuration file and before actually constructing
 * the Mixer instances (hint: this method is static!).
 *
 * @param ref_card The card id
 * @param ref_control The control id. The corresponding control must be present in the card.
 * @param preferred Whether this is the preferred master (auto-selected on coldplug and hotplug).
 */
void MixerToolBox::setGlobalMaster(const QString &ref_card, const QString &ref_control, bool preferred)
{
    qCDebug(KMIX_LOG) << "card" << ref_card << "control" << ref_control << "preferred?" << preferred;
    s_globalMasterCurrent.set(ref_card, ref_control);
    if (preferred) s_globalMasterPreferred.set(ref_card, ref_control);
}


Mixer *MixerToolBox::getGlobalMasterMixer(bool fallbackAllowed)
{
    const QString &ref_card = s_globalMasterCurrent.getCard();
    for (Mixer *mixer : std::as_const(s_allMixers))
    {
        if (mixer->id()==ref_card) return (mixer);
    }

    if (fallbackAllowed)
    {							// first mixer available as fallback
        if (!s_allMixers.isEmpty()) return (s_allMixers.first());
    }

    return (nullptr);
}


/**
 * Return the preferred global master.
 * If there is no preferred global master, returns the current master instead.
 */
MasterControl &MixerToolBox::getGlobalMasterPreferred(bool fallbackAllowed)
{
    static MasterControl result;

    if (!fallbackAllowed || s_globalMasterPreferred.isValid())
    {
        //qCDebug(KMIX_LOG) << "Returning preferred master";
        return (s_globalMasterPreferred);
    }

    Mixer *mm = getGlobalMasterMixer(false);		// no fallback
    if (mm!=nullptr)
    {
        result.set(s_globalMasterPreferred.getCard(), mm->getRecommendedDeviceId());
        if (!result.getControl().isEmpty())
        {
            //qCDebug(KMIX_LOG) << "Returning extended preferred master";
            return (result);
        }
    }

    qCDebug(KMIX_LOG) << "Returning current master";
    return (s_globalMasterCurrent);
}


shared_ptr<MixDevice> MixerToolBox::getGlobalMasterMD(bool fallbackAllowed)
{
    shared_ptr<MixDevice> mdRet;
    Mixer *mixer = getGlobalMasterMixer(fallbackAllowed);
    if (mixer==nullptr) return (mdRet);

    if (s_globalMasterCurrent.getControl().isEmpty())
    {
        // Default (recommended) control
        return (mixer->recommendedMaster());
    }

    shared_ptr<MixDevice> firstDevice;
    for (const shared_ptr<MixDevice> &md : std::as_const(mixer->getMixSet()))
    {							// getMixSet() = _mixerBackend->m_mixDevices
        if (md==nullptr) continue;			// invalid

        firstDevice = md;
        if (md->id()==s_globalMasterCurrent.getControl())
        {
            mdRet = md;
            break;					// found
        }
    }

    if (mdRet==nullptr)
    {
        // For some sound cards when using PulseAudio the mixer ID is not proper,
        // hence returning the first device as master channel device.
        // This solves bug 290177 and problems stated in review #105422.
        qCDebug(KMIX_LOG) << " No global master, returning the first device";
        mdRet = firstDevice;
    }

    return (mdRet);
}


/**
 * Check whether a dynamic mixer if active.
 *
 * @return @c true if at least one dynamic mixer is active
 */
bool MixerToolBox::dynamicBackendsPresent()
{
    for (const Mixer *mixer : std::as_const(s_allMixers))
    {
        if (mixer->isDynamic()) return (true);
    }
    return (false);
}


/**
 * Check whether the PulseAudio backend is active.
 *
 * @return @c true if at least one mixer using PulseAudio is active
 *
 * @note In the default mode of operation, if PulseAudio is active
 * then it will be the only active backend.  In the experimental
 * multi-driver mode, then it may be one among others.
 */
bool MixerToolBox::pulseaudioPresent()
{
    for (const Mixer *mixer : std::as_const(s_allMixers))
    {
        if (mixer->getDriverName()=="PulseAudio") return (true);
    }
    return (false);
}


/**
 * Create a backend instance for the specified @p backendName and its
 * internal @p deviceIndex.  It will be associated with the parent @p mixer.
 */
MixerBackend *MixerToolBox::getBackendFor(const QString &backendName, int deviceIndex, Mixer *mixer)
{
    for (int driverIndex = 0; driverIndex<numBackends; ++driverIndex)
    {
        const QString name = backendNameFor(driverIndex);
        if (name==backendName)				// look for backend with that name
        {
            // Retrieve the mixer factory for that backend and use it
            getMixerFunc *f = g_mixerFactories[driverIndex].getMixer;
            Q_ASSERT(f!=nullptr);			// known data, should never happen
            return (f(mixer, deviceIndex));
        }
    }

    qCWarning(KMIX_LOG) << "No backend available for" << backendName;
    return (nullptr);					// no such backend found
}
