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

#include <solid/device.h>
#include <solid/block.h>


static QRegExp s_ignoreMixerExpression(QStringLiteral("Modem"));

static QStringList s_allowedBackends;			// filter for enabled backends
static QString s_hotplugBackend;			// backend for hotplug events
static bool s_multiDriverFlag = false;			// never activate by accident!

static QList<Mixer *> s_allMixers;

static MasterControl s_globalMasterCurrent;
static MasterControl s_globalMasterPreferred;

/**
 * External functions and data for the available backends
 */

#ifdef HAVE_SUN_MIXER
extern MixerBackend *SUN_getMixer(Mixer *mixer, int deviceIndex);
extern const char *SUN_driverName;
#endif

// Possibly encapsulated by #ifdef HAVE_DBUS
extern MixerBackend *MPRIS2_getMixer(Mixer *mixer, int deviceIndex);
extern const char *MPRIS2_driverName;

#ifdef HAVE_ALSA_MIXER
extern MixerBackend *ALSA_getMixer(Mixer *mixer, int deviceIndex);
extern int ALSA_acceptsHotplugId(const QString &id);
extern bool ALSA_acceptsDeviceNode(const QString &blkdev, int devnum);
extern const char *ALSA_driverName;
#endif

#ifdef HAVE_PULSEAUDIO
extern MixerBackend *PULSE_getMixer(Mixer *mixer, int deviceIndex);
extern const char *PULSE_driverName;
#endif

#ifdef HAVE_OSS_3
extern MixerBackend *OSS_getMixer(Mixer *mixer, int deviceIndex);
extern int OSS_acceptsHotplugId(const QString &id);
extern bool OSS_acceptsDeviceNode(const QString &blkdev, int devnum);
extern const char *OSS_driverName;
#endif

#ifdef HAVE_OSS_4
extern MixerBackend *OSS4_getMixer(Mixer *mixer, int deviceIndex);
extern const char *OSS4_driverName;
#endif

/**
 * Function type to create a mixer backend
 *
 * @param mixer The mixer to associate with the backend
 * @param device The device index
 * @return the created backend
 */
typedef MixerBackend *getMixerFuncType(Mixer *mixer, int device);

/**
 * Function type to check whether a hotplug ID applies
 *
 * @param id The hotplug ID
 * @return the device index if applicable, or @c -1 if the ID is not
 * recognised
 *
 * @note This test may be called fairly frequently, so it should operate
 * silently without any debug messages.
 *
 * @note For Solid, the UDI can be assumed to have already been checked
 * and contains "/sound/" within the path.
 */
typedef int acceptsHotplugIdFuncType(const QString &id);

/**
 * Function type to check whether a device node is recognised
 * by a backend.
 *
 * @param blkdev The device node path, normally a character device
 * (althougn Solid calls it a block device)
 * @param devnum The device instance number
 * @return @c true if the device is recognised and matches the
 * device instance number.
 */
typedef bool acceptsDeviceNodeFuncType(const QString &blkdev, int devnum);

/**
 * Data for one supported backend
 */
struct MixerFactory
{
    getMixerFuncType *getMixerFunc;
    const char *backendName;
    acceptsHotplugIdFuncType *acceptsHotplugIdFunc;
    acceptsDeviceNodeFuncType *acceptsDeviceNodeFunc;
};

/**
 * The list of supported backends
 */
static const MixerFactory g_mixerFactories[] =
{
#ifdef HAVE_SUN_MIXER
    { &SUN_getMixer, SUN_driverName, nullptr, nullptr },
#endif

#ifdef HAVE_PULSEAUDIO
    { &PULSE_getMixer, PULSE_driverName, nullptr, nullptr },
#endif

#ifdef HAVE_ALSA_MIXER
    { &ALSA_getMixer, ALSA_driverName, &ALSA_acceptsHotplugId, &ALSA_acceptsDeviceNode },
#endif

#ifdef HAVE_OSS_3
    { &OSS_getMixer, OSS_driverName, &OSS_acceptsHotplugId, &OSS_acceptsDeviceNode },
#endif

#ifdef HAVE_OSS_4
    { &OSS4_getMixer, OSS4_driverName, nullptr, nullptr },
#endif

    // Make sure MPRIS2 is at the end.  The implementation of SINGLE_PLUS_MPRIS2
    // in MixerToolBox is much easier.  And also we make sure that streams are always
    // the last backend, which is important for the default KMix GUI layout.
    { &MPRIS2_getMixer, MPRIS2_driverName, nullptr, nullptr }
};

static const int numBackends = sizeof(g_mixerFactories)/sizeof(MixerFactory);


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
 * to the specified @p driverIndex in the list.  This also implements
 * the allowed backend filtering and so may return a null string if
 * the backend at the specified index is not to be used.
 *
 * @param driverIndex Index in list, 0 <= index < numBackends
 * @param filter If @c false, ignore the allowed backend filter list
 * @return the backend name, or a null string if that backend is filtered out
 */
static QString backendNameFor(int driverIndex, bool filter = true)
{
    const QString &name = g_mixerFactories[driverIndex].backendName;
    if (filter && !s_allowedBackends.isEmpty() && !s_allowedBackends.contains(name, Qt::CaseInsensitive)) return (QString());
    return (name);
}


/**
 * This is used during initial coldplugging to find the Solid UDI that should
 * be associated with the detected mixer.  It is needed so that if the device
 * is later unplugged, the relevant instance can be found to remove.  For a
 * hotplugged device the UDI is provided by Solid in the first place, but for
 * coldplugged devices it has to be searched for.
 *
 * Note: The need for this could possibly be eliminated by asking Solid for
 * all currently known sound devices at coldplug time, instead of probing with
 * each backend.  However, much of initMixerInternal() would still have to be
 * retained to handle the mixer setup for PulseAudio and MPRIS2.
 */
static QString hotplugIdForDevice(int driverIndex, int devnum)
{
    // If the backend does not support hotplugging, ignore it.
    acceptsDeviceNodeFuncType *f = g_mixerFactories[driverIndex].acceptsDeviceNodeFunc;
    if (f==nullptr) return (QString());

    // If the backend is not configured, ignore it.
    const QString &backend = backendNameFor(driverIndex);
    if (backend.isEmpty()) return (QString());

    // If the backend is not enabled for hotplugging, ignore it.
    if (!s_hotplugBackend.isEmpty() && backend!=s_hotplugBackend) return (QString());

    const QList<Solid::Device> allDevices = Solid::Device::allDevices();
    for (const Solid::Device &device : allDevices)
    {
        const QString &udi = device.udi();		// quickly ignore non-sound devices
        if (!MixerToolBox::isSoundDevice(udi)) continue;

        if (!device.is<Solid::Block>()) continue;	// must have a device node path

        const Solid::Block *blk = device.as<Solid::Block>();
        const QString blkdev = blk->device();
        if ((*f)(blkdev, devnum))
        {
            qCDebug(KMIX_LOG) << "for device" << blkdev << "found UDI" << udi;
            return (udi);		// see if device is recognised
        }
    }

    return (QString());					// no UDI found
}


/**
 * Scan for Mixer devices on the system. This fills the list of Mixer's which is
 * accessible via MixerToolBox::mixers().
 *
 * This is run only once during the initialization phase of KMix. It performs
 * the following tasks:
 *
 * 1) Coldplug scan, to fill the initial mixer list
 * 2) Resolve which backend to use (hotplug events of other backends are ignored).
 *
 * @param multiDriverMode Whether the scan should try to scan for more backends
 * once one "regular" backand has been found.  "Regular" means a backend which
 * has a 1-1 correspndence between an instance and a hardware device (card).
 * Therefore ALSA and OSS are "regular" backends but PulseAudio is not.
 * @c MULTI means to scan all backends.  @c SINGLE means to only scan until a
 * "regular" backend to use has been found.  @c SINGLE_PLUS_MPRIS2 is the same
 * as @c SINGLE but also accepts the MPRIS2 backend.
 *
 * @param allowHotplug Whether hotplugging is expected to happen for the detected
 * backends.  The KMix application and the KDED module set this to @c true, while
 * kmixctrl sets this to @c false because it is only a transient utility.
 */
static void initMixerInternal(MultiDriverMode multiDriverMode, bool allowHotplug)
{  
   const bool useBackendFilter = !s_allowedBackends.isEmpty();
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
   qCDebug(KMIX_LOG) << "multiDriverMode =" << qPrintable(multiModeString);

   // Find all mixers and initialize them

   int driverWithMixer = -1;
   bool multipleDriversActive = false;

   QStringList allDrivers;
   QStringList allowedDrivers;
   for (int drv = 0; drv<numBackends; ++drv)
   {
       allDrivers.append(backendNameFor(drv, false));
       const QString &name = backendNameFor(drv);
       if (!name.isEmpty()) allowedDrivers.append(name);
   }

   qCDebug(KMIX_LOG) << "Sound drivers supported =" << qPrintable(allDrivers.join(','));
   if (allowedDrivers.count()<allDrivers.count()) qCDebug(KMIX_LOG) << "Sound drivers allowed =" << qPrintable(allowedDrivers.join(','));

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
   QStringList driverInfoUsed;

   for (int drv = 0; drv<numBackends; ++drv)
   {
      if ( autodetectionFinished )
      {
         // inner loop indicates that we are finished => sane exit from outer loop
         break;
      }

      const QString driverName = backendNameFor(drv);
      if (driverName.isEmpty())
      {
	  qCDebug(KMIX_LOG) << "Ignored" << backendNameFor(drv, false) << "- filtered";
	  continue;
      }

      const bool regularBackend = driverName!=QLatin1String("MPRIS2") && driverName!=QLatin1String("PulseAudio");
      if (regularBackend && regularBackendFound)
      {
	  qCDebug(KMIX_LOG) << "Ignored" << driverName << "- regular backend already found";
    	  // Only accept one regular backend => skip this one
    	  continue;
      }
   
      qCDebug(KMIX_LOG) << "Looking for mixers with the" << driverName << "driver";

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
#ifdef HAVE_PULSEAUDIO
				 else if ( driverName == QLatin1String("PulseAudio") )
				 {
					 // PulseAudio is not useful together with MPRIS2. Treat it as "single"
					 if ( foundSomethingAndLastControlReached )
						 autodetectionFinished = true;
				 }
#endif
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

             if (!mixer->isDynamic())			// not PulseAudio or MPRIS2
             {
                 // Find and set the hotplug ID, so that the device will be
                 // able to be unplugged later.
                 const QString &id = hotplugIdForDevice(drv, dev);
                 if (!id.isEmpty()) mixer->setHotplugId(id);
                 else qCWarning(KMIX_LOG) << "Cannot find UDI - unplug not possible";
             }

            // append driverName (used drivers)
            if ( !drvInfoAppended )
            {
               drvInfoAppended = true;
               driverInfoUsed.append(driverName);
            }

            // Check whether there are mixers in different drivers, so that the user can be warned
            if ( !multipleDriversActive )
            {
               if ( driverWithMixer == -1 )
               {
                  // Aha, this is the very first detected device
                  driverWithMixer = drv;
               }
               else if (driverWithMixer!=drv && driverName!="MPRIS2")
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

   qCDebug(KMIX_LOG) << "Sound drivers detected =" << qPrintable(driverInfoUsed.join(','));
   if (driverInfoUsed.isEmpty() || driverInfoUsed.first()=="MPRIS2")
   {
       // If there were no "regular" mixers found, we assume that hotplugging
       // will take place on the preferred backend (which is always the first
       // "regular" backend in the list).
       driverInfoUsed.prepend(MixerToolBox::preferredBackend());
   }
   qCDebug(KMIX_LOG) << "Sound drivers used =" << qPrintable(driverInfoUsed.join(','));

   if (multipleDriversActive)
   {
       qCDebug(KMIX_LOG) << "Experimental multiple-driver mode activated";
       if (allowHotplug) s_hotplugBackend.clear();
   }
   else
   {
       if (allowHotplug)
       {
           // Even if the first backend in use is PulseAudio, this is mostly
           // harmless because KMixWindow::initActionsAfterInitMixer() will
           // not call KMixDeviceManager::initHotplug().
           s_hotplugBackend = driverInfoUsed.first();
           qCDebug(KMIX_LOG) << "Hotplug events accepted =" << qPrintable(s_hotplugBackend);
       }
   }

   qCDebug(KMIX_LOG) << "Total detected mixers" << s_allMixers.count();
}


void MixerToolBox::initMixer(bool allowHotplug)
{
    const MultiDriverMode multiDriverMode = s_multiDriverFlag ?  MULTI : SINGLE_PLUS_MPRIS2;
    initMixerInternal(multiDriverMode, allowHotplug);
}


/**
 * Opens and adds a mixer to the KMix wide Mixer array, if the given Mixer is valid.
 * Otherwise the Mixer is deleted.
 * This method can be used for adding "static" devices (at program start) and also for hotplugging.
 *
 * @param mixer The mixer to add
 * @return @c true if the mixer was added
 */
bool MixerToolBox::possiblyAddMixer(Mixer *mixer)
{
    if (mixer==nullptr) return (false);
    if (mixer->openIfValid())
    {
        // TODO: wrong order, do this check before opening the mixer?
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
#ifdef HAVE_PULSEAUDIO
    return (backendPresent("PulseAudio"));
#else
    return (false);
#endif
}


/**
 * Check whether an instance of the specified backend is active:
 * that is, whether any existing mixer is using this driver.
 *
 * @param driverName The driver name to check
 * @return @c true if at least one mixer using that backend is active
 */
bool MixerToolBox::backendPresent(const QString &driverName)
{
    // TODO: can use std::find_if
    for (const Mixer *mixer : std::as_const(s_allMixers))
    {
        if (mixer->getDriverName()==driverName) return (true);
    }
    return (false);
}


/**
 * The preferred fallback backend name, if none could be found during
 * the initial scan.  This is the first in the backend list which is
 * not PulseAudio.
 */
QString MixerToolBox::preferredBackend()
{
    for (int driverIndex = 0; driverIndex<numBackends; ++driverIndex)
    {
        const QString &name = backendNameFor(driverIndex);
        if (name.isEmpty()) continue;			// backend name is filtered out
        if (name!="PulseAudio") return (name);		// acceptable name
    }

    return (backendNameFor(0));				// very last resort fallback
}


/**
 * Create a backend instance for the specified @p backendName and its
 * internal @p deviceIndex.  It will be associated with the parent @p mixer.
 */
MixerBackend *MixerToolBox::getBackendFor(const QString &backendName, int deviceIndex, Mixer *mixer)
{
    for (int driverIndex = 0; driverIndex<numBackends; ++driverIndex)
    {
        const QString &name = backendNameFor(driverIndex);
        if (!name.isEmpty() && name==backendName)	// look for backend with that name
        {
            // Retrieve the mixer factory for that backend and use it
            getMixerFuncType *f = g_mixerFactories[driverIndex].getMixerFunc;
            Q_ASSERT(f!=nullptr);			// known data, should never happen
            return ((*f)(mixer, deviceIndex));
        }
    }

    qCWarning(KMIX_LOG) << "No backend available for" << backendName;
    return (nullptr);					// no such backend found
}


void MixerToolBox::setAllowedBackends(const QStringList &backends)
{
    qCDebug(KMIX_LOG) << backends;
    s_allowedBackends = backends;
}


void MixerToolBox::setMultiDriverMode(bool multiDriverMode)
{
    qCDebug(KMIX_LOG) << multiDriverMode;
    s_multiDriverFlag = multiDriverMode;
}


bool MixerToolBox::isMultiDriverMode()
{
    return (s_multiDriverFlag);
}


bool MixerToolBox::isSoundDevice(const QString &id)
{
    return (id.contains("/sound/"));			// any UDI mentioning sound
}


/**
 * See whether the specified hotplug ID applies to any of the
 * available backends, in priority order.
 *
 * @param id The hotplug ID
 * @return A @c <backendName,deviceIndex> pair if the ID is
 * recognised, otherwise a pair with @c -1 as the index.
 *
 * @note This function and the backend functions that it calls may
 * be called very frequently and for arbitrary UDIs.  Keep them silent!
 */
QPair<QString,int> MixerToolBox::acceptsHotplugId(const QString &id)
{
    QPair<QString,int> res;
    res.second = -1;					// no backend found yet

    if (isSoundDevice(id))
    {
        for (int driverIndex = 0; driverIndex<numBackends; ++driverIndex)
        {
            const QString &name = backendNameFor(driverIndex);
            // If the backend is not enabled by configuration, ignore it.
            if (name.isEmpty()) continue;
            // If the backend is not enabled for hotplugging, ignore it.
            if (!s_hotplugBackend.isEmpty() && name!=s_hotplugBackend) continue;

            acceptsHotplugIdFuncType *f = g_mixerFactories[driverIndex].acceptsHotplugIdFunc;
            if (f==nullptr) continue;			// backend doesn't support hotplugging

            int devnum = (*f)(id);			// see if ID is accepted
            if (devnum!=-1)				// yes, with valid device number
            {
                res.first = name;
                res.second = devnum;
                break;
            }
        }
    }

    return (res);
}
