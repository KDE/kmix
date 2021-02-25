/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken - esken@kde.org
 *                    2002 Helio Chissini de Castro - helio@conectiva.com.br
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

#include "core/mixer.h"

#include <klocalizedstring.h>
#include <kconfig.h>

#include "settings.h"
#include "backends/mixer_backend.h"
#include "backends/kmix-backends.cpp"
#include "core/ControlManager.h"
#include "core/volume.h"

/**
 * Some general design hints. Hierarchy is Mixer->MixDevice->Volume
 */

QList<Mixer *> Mixer::s_mixers;
MasterControl Mixer::_globalMasterCurrent;
MasterControl Mixer::_globalMasterPreferred;

/* static */ int Mixer::numDrivers()
{
    const MixerFactory *factory = g_mixerFactories;
    int num = 0;
    while (factory->getMixer!=nullptr)
    {
        ++num;
        ++factory;
    }

    return (num);
}

/*
 * Returns a reference to the current mixer list.
 */
/* static */ QList<Mixer *> &Mixer::mixers()
{
    return s_mixers;
}

/**
 * Returns whether there is at least one dynamic mixer active.
 * @returns true, if at least one dynamic mixer is active
 */
/* static */ bool Mixer::dynamicBackendsPresent()
{
    for (const Mixer *mixer : qAsConst(s_mixers))
    {
        if (mixer->isDynamic()) return (true);
    }
    return (false);
}

/* static */ bool Mixer::pulseaudioPresent()
{
    for (const Mixer *mixer : qAsConst(s_mixers))
    {
        if (mixer->getDriverName()=="PulseAudio") return (true);
    }
    return (false);
}


Mixer::Mixer(const QString &ref_driverName, int device)
    : m_balance(0),
      m_dynamic(false)
{
    _mixerBackend = nullptr;
    const int driverCount = numDrivers();
    for (int driver = 0; driver<driverCount; ++driver)
    {
        const QString name = driverName(driver);
        if (name==ref_driverName)
        {
            // driver found => retrieve Mixer factory for that driver
            getMixerFunc *f = g_mixerFactories[driver].getMixer;
            if (f!=nullptr)
            {
                _mixerBackend = f(this, device);
                readSetFromHWforceUpdate();  // enforce an initial update on first readSetFromHW()
            }
            break;
        }
    }
}



Mixer::~Mixer()
{
   // Close the mixer. This might also free memory, depending on the called backend method
   close();
   _mixerBackend->deleteLater();
}


/*
 * Find a Mixer. If there is no mixer with the given id, a null pointer is returned
 */
/* static */ Mixer *Mixer::findMixer(const QString &mixer_id)
{
    const int mixerCount = mixers().count();
    for (int i = 0; i<mixerCount; ++i)
    {
        Mixer *mix = mixers().at(i);
        if (mix->id()==mixer_id) return (mix);
    }

    return (nullptr);
}


/**
 * Set the final ID of this Mixer.
 * <br>Warning: This method is VERY fragile, because it is requires information that we have very late,
 * especially the _cardInstance. We only know the _cardInstance, when we know the ID of the _mixerBackend->getId().
 * OTOH, the Mixer backend needs the _cardInstance during construction of its MixDevice instances.
 *
 * This means, we need the _cardInstance during construction of the Mixer, but we only know it after its constructed.
 * Actually its a design error. The _cardInstance MUST be set and managed by the backend.
 *
 * The current solution works but is very hacky - cardInstance is a parameter of openIfValid().
 *
 */
void Mixer::recreateId()
{
    /* As we use "::" and ":" as separators, the parts %1,%2 and %3 may not
     * contain it.
     * %1, the driver name is from the KMix backends, it does not contain colons.
     * %2, the mixer name, is typically coming from an OS driver. It could contain colons.
     * %3, the mixer number, is a number: it does not contain colons.
     */
    QString mixerName = _mixerBackend->getId();
    mixerName.replace(':','_');
    QString primaryKeyOfMixer = QString("%1::%2:%3")
            .arg(getDriverName(), mixerName)
            .arg(getCardInstance());
    // The following 3 replaces are for not messing up the config file
    primaryKeyOfMixer.replace(']','_');
    primaryKeyOfMixer.replace('[','_'); // not strictly necessary, but lets play safe
    primaryKeyOfMixer.replace(' ','_');
    primaryKeyOfMixer.replace('=','_');
    _id = primaryKeyOfMixer;
//	qCDebug(KMIX_LOG) << "Early _id=" << _id;
}

const QString Mixer::dbusPath()
{
	// _id needs to be fixed from the very beginning, as the MixDevice construction uses MixDevice::dbusPath().
	// So once the first MixDevice is created, this must return the correct value
	if (_id.isEmpty())
	{
		bool wasRegistered = _mixerBackend->_cardRegistered;
		// Bug 308014: Actually this a shortcut (you could also call it a hack). It would likely better if registerCard()
		//             would create the Id, but it requires cooperation from ALL backends. Also Mixer->getId() would need to
		//             proxy that to the backend.
		// So for now we lazily create the MixerId here, while creating the first MixDevice for that card.
		recreateId();
		if (! wasRegistered)
		{
			// Bug 308014: By checking _cardRegistered, we can be sure that everything is fine, including the fact that
			// the cardId (aka "card instance") is set. If _cardRegistered would be false, we will create potentially
			// wrong/duplicated DBUS Paths here.
			qCWarning(KMIX_LOG) << "Mixer id was empty when creating DBUS path. Emergency code created the id=" <<_id;
		}
	}

	// mixerName may contain arbitrary characters, so replace all that are not allowed to be be part of a DBUS path
	QString cardPath = _id;
	cardPath.replace(QRegExp("[^a-zA-Z0-9_]"), "_");
	cardPath.replace(QLatin1String("//"), QLatin1String("/"));

    return QString("/Mixers/" + cardPath);
}

void Mixer::volumeSave(KConfig *config) const
{
    //    qCDebug(KMIX_LOG) << "Mixer::volumeSave()";
    _mixerBackend->readSetFromHW();
    QString grp("Mixer");
    grp.append(id());
    _mixerBackend->m_mixDevices.write( config, grp );

    // This might not be the standard application config object
    // => Better be safe and call sync().
    config->sync();
}

void Mixer::volumeLoad(const KConfig *config)
{
   QString grp("Mixer");
   grp.append(id());
   if ( ! config->hasGroup(grp) ) {
      // no such group. Volumes (of this mixer) were never saved beforehand.
      // Thus don't restore anything (also see Bug #69320 for understanding the real reason)
      return; // make sure to bail out immediately
   }

   // else restore the volumes
   if ( ! _mixerBackend->m_mixDevices.read( config, grp ) ) {
      // Some mixer backends don't support reading the volume into config
      // files, so bail out early if that's the case.
      return;
   }

   // set new settings
   for (int i = 0; i<_mixerBackend->m_mixDevices.count(); ++i)
   {
	   shared_ptr<MixDevice> md = _mixerBackend->m_mixDevices[i];
	   if (!md) continue;

       _mixerBackend->writeVolumeToHW( md->id(), md );
       if ( md->isEnum() )
    	   _mixerBackend->setEnumIdHW( md->id(), md->enumId() );
   }
}


/**
 * Opens the mixer.
 * Also, starts the polling timer, for polling the Volumes from the Mixer.
 *
 * @return true, if Mixer could be opened.
 */
bool Mixer::openIfValid()
{
    if (_mixerBackend==nullptr)
    {
        // If we did not instantiate a suitable backend, then the mixer is invalid.
        qCWarning(KMIX_LOG) << "no mixer backend";
        return false;
    }

    bool ok = _mixerBackend->openIfValid();
    if (!ok) return (false);

    recreateId();
    shared_ptr<MixDevice> recommendedMaster = _mixerBackend->recommendedMaster();
    if (recommendedMaster)
    {
        QString recommendedMasterStr = recommendedMaster->id();
        setLocalMasterMD( recommendedMasterStr );
        qCDebug(KMIX_LOG) << "Detected master" << recommendedMaster->id();
    }
    else
    {
        if (!m_dynamic) qCCritical(KMIX_LOG) << "No master detected and not dynamic";
        else qCDebug(KMIX_LOG) << "No master detected but dynamic";
        QString noMaster = "---no-master-detected---";
        setLocalMasterMD(noMaster); // no master
    }

    new DBusMixerWrapper(this, dbusPath());
    return (true);
}


/**
 * Closes the mixer.
 */
void Mixer::close()
{
    if (_mixerBackend!=nullptr) _mixerBackend->closeCommon();
}


/* ------- WRAPPER METHODS. START ------------------------------ */

bool Mixer::isOpen() const
{
    if (_mixerBackend==nullptr) return (false);
    else return (_mixerBackend->isOpen());
}

void Mixer::readSetFromHWforceUpdate() const
{
    _mixerBackend->readSetFromHWforceUpdate();
}

  /// Returns translated WhatsThis messages for a control.Translates from 
QString Mixer::translateKernelToWhatsthis(const QString &kernelName) const
{
   return _mixerBackend->translateKernelToWhatsthis(kernelName);
}

/* ------- WRAPPER METHODS. END -------------------------------- */

int Mixer::balance() const
{
    return m_balance;
}

void Mixer::setBalance(int balance)
{
   if( balance == m_balance ) {
      // balance unchanged => return
      return;
   }

   m_balance = balance;

   shared_ptr<MixDevice> master = getLocalMasterMD();
   if (!master)
   {
      // no master device available => return
      return;
   }

   Volume& volP = master->playbackVolume();
   setBalanceInternal(volP);
   Volume& volC = master->captureVolume();
   setBalanceInternal(volC);

   _mixerBackend->writeVolumeToHW( master->id(), master );
   emit newBalance( volP );
}

void Mixer::setBalanceInternal(Volume& vol)
{
   int left = vol.getVolume(Volume::LEFT);
   int right = vol.getVolume( Volume::RIGHT );
   int refvol = left > right ? left : right;
   if( m_balance < 0 ) // balance left
   {
      vol.setVolume( Volume::LEFT,  refvol);
      vol.setVolume( Volume::RIGHT, (m_balance * refvol) / 100 + refvol );
   }
   else
   {
      vol.setVolume( Volume::LEFT, -(m_balance * refvol) / 100 + refvol );
      vol.setVolume( Volume::RIGHT,  refvol);
   }
}

/**
 * Returns a name suitable for a human user to read (on a label, ...)
 */

QString Mixer::readableName(bool ampersandQuoted) const
{
    QString finalName = _mixerBackend->getName();
    if (ampersandQuoted) finalName.replace('&', "&&");
    if (getCardInstance()>1) finalName = finalName.append(" %1").arg(getCardInstance());
    //qCDebug(KMIX_LOG) << "name=" << _mixerBackend->getName() << "instance=" <<  getCardInstance() << ", finalName" << finalName;
    return (finalName);
}


QString Mixer::getBaseName() const
{
  return _mixerBackend->getName();
}

/**
 * Queries the Driver Factory for a driver.
 * @p driver Index number. 0 <= driver < numDrivers()
 */
/* static */ QString Mixer::driverName(int driver)
{
    getDriverNameFunc *f = g_mixerFactories[driver].getDriverName;
    if (f!=nullptr) return f();
    else return "unknown";
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
/* static */ void Mixer::setGlobalMaster(QString ref_card, QString ref_control, bool preferred)
{
    qCDebug(KMIX_LOG) << "ref_card=" << ref_card << ", ref_control=" << ref_control << ", preferred=" << preferred;
    _globalMasterCurrent.set(ref_card, ref_control);
    if ( preferred )
        _globalMasterPreferred.set(ref_card, ref_control);
    qCDebug(KMIX_LOG) << "Mixer::setGlobalMaster() card=" <<ref_card<< " control=" << ref_control;
}

/* static */ Mixer *Mixer::getGlobalMasterMixerNoFalback()
{
    for (Mixer *mixer : qAsConst(s_mixers))
    {
        if (mixer!=nullptr && mixer->id()==_globalMasterCurrent.getCard())
            return mixer;
    }
    return (nullptr);
}

/* static */ Mixer* Mixer::getGlobalMasterMixer()
{
    Mixer *mixer = getGlobalMasterMixerNoFalback();
    if (mixer==nullptr && mixers().count()>0) mixer = mixers()[0]; // produce fallback
    //qCDebug(KMIX_LOG) << "Mixer::masterCard() returns " << mixer->id();
    return (mixer);
}


/**
 * Return the preferred global master.
 * If there is no preferred global master, returns the current master instead.
 */
/* static */ MasterControl &Mixer::getGlobalMasterPreferred(bool fallbackAllowed)
{
    static MasterControl result;

    if ( !fallbackAllowed || _globalMasterPreferred.isValid() ) {
//        qCDebug(KMIX_LOG) << "Returning preferred master";
        return _globalMasterPreferred;
    }

    Mixer* mm = Mixer::getGlobalMasterMixerNoFalback();
    if (mm!=nullptr) {
        result.set(_globalMasterPreferred.getCard(), mm->getRecommendedDeviceId());
        if (!result.getControl().isEmpty())
//            qCDebug(KMIX_LOG) << "Returning extended preferred master";
            return result;
    }

    qCDebug(KMIX_LOG) << "Returning current master";
    return _globalMasterCurrent;
}


/* static */ shared_ptr<MixDevice> Mixer::getGlobalMasterMD(bool fallbackAllowed)
{
	shared_ptr<MixDevice> mdRet;
	shared_ptr<MixDevice> firstDevice;
	Mixer *mixer = fallbackAllowed ?
		   Mixer::getGlobalMasterMixer() : Mixer::getGlobalMasterMixerNoFalback();

	if (mixer==nullptr)
		return mdRet;

	if (_globalMasterCurrent.getControl().isEmpty())
	{
		// Default (recommended) control
		return mixer->_mixerBackend->recommendedMaster();
	}

	for (const shared_ptr<MixDevice> md : qAsConst(mixer->_mixerBackend->m_mixDevices))
	{
		if (!md) continue; // invalid

		firstDevice=md;
		if ( md->id() == _globalMasterCurrent.getControl() )
		{
			mdRet = md;
			break; // found
		}
	}

	if (!mdRet)
	{
	  //For some sound cards when using pulseaudio the mixer id is not proper hence returning the first device as master channel device
	  //This solves the bug id:290177 and problems stated in review #105422
		qCDebug(KMIX_LOG) << "Mixer::masterCardDevice() returns 0 (no globalMaster), returning the first device";
		mdRet=firstDevice;
	}

	return mdRet;
}

QString Mixer::getRecommendedDeviceId() const
{
    if (_mixerBackend!=nullptr)
    {
        shared_ptr<MixDevice> recommendedMaster = _mixerBackend->recommendedMaster();
        if (recommendedMaster) return (recommendedMaster->id());
    }
    return (QString());
}

shared_ptr<MixDevice> Mixer::getLocalMasterMD() const
{
    if (_mixerBackend!=nullptr && _masterDevicePK.isEmpty()) return (_mixerBackend->recommendedMaster());
    return (find(_masterDevicePK));
}

void Mixer::setLocalMasterMD(const QString &devPK)
{
    _masterDevicePK = devPK;
}


shared_ptr<MixDevice> Mixer::find(const QString &mixdeviceID) const
{
	shared_ptr<MixDevice> mdRet;
	for (const shared_ptr<MixDevice> md : qAsConst(_mixerBackend->m_mixDevices))
	{
		if (!md) continue; // invalid
		if (md->id()==mixdeviceID)
		{
			mdRet = md;
			break; // found
		}
	}

    return mdRet;
}


shared_ptr<MixDevice> Mixer::getMixdeviceById(const QString& mixdeviceID) const
{
	qCDebug(KMIX_LOG) << "id=" << mixdeviceID << "md=" << _mixerBackend->m_mixDevices.get(mixdeviceID).get()->id();
	return _mixerBackend->m_mixDevices.get(mixdeviceID);
}

/**
   Call this if you have a *reference* to a Volume object and have modified that locally.
   Pass the MixDevice associated to that Volume to this method for writing back
   the changed value to the mixer.
   Hint: Why do we do it this way?
   - It is fast               (no copying of Volume objects required)
   - It is easy to understand ( read - modify - commit )
*/
void Mixer::commitVolumeChange(shared_ptr<MixDevice> md)
{
	_mixerBackend->writeVolumeToHW(md->id(), md);
	if (md->isEnum())
	{
		_mixerBackend->setEnumIdHW(md->id(), md->enumId());
	}
	if (md->captureVolume().hasSwitch())
	{
		// Make sure to re-read the hardware, because setting capture might have failed.
		// This is due to exclusive capture groups.
		// If we wouldn't do this, KMix might show a Capture Switch disabled, but
		// in reality the capture switch is still on.
		//
		// We also cannot rely on a notification from the driver (SocketNotifier), because
		// nothing has changed, and so there s nothing to notify.
		_mixerBackend->readSetFromHWforceUpdate();
		if (Settings::debugControlManager())
			qCDebug(KMIX_LOG)
			<< "committing a control with capture volume, that might announce: " << md->id();
		_mixerBackend->readSetFromHW();
	}
	if (Settings::debugControlManager())
		qCDebug(KMIX_LOG)
		<< "committing announces the change of: " << md->id();

	// We announce the change we did, so all other parts of KMix can pick up the change
	ControlManager::instance().announce(md->mixer()->id(), ControlManager::Volume,
		QString("Mixer.commitVolumeChange()"));
}

// @dbus, used also in kmix app
void Mixer::increaseVolume( const QString& mixdeviceID )
{
	increaseOrDecreaseVolume(mixdeviceID, false);
}

// @dbus
void Mixer::decreaseVolume( const QString& mixdeviceID )
{
	increaseOrDecreaseVolume(mixdeviceID, true);
}

/**
 * Increase or decrease all playback and capture channels of the given control.
 * This method is very similar to MDWSlider::increaseOrDecreaseVolume(), but it will
 * NOT auto-unmute.
 *
 * @param mixdeviceID The control name
 * @param decrease true for decrease. false for increase
 */
void Mixer::increaseOrDecreaseVolume( const QString& mixdeviceID, bool decrease )
{
    shared_ptr<MixDevice> md= getMixdeviceById( mixdeviceID );
    if (md)
    {
        Volume& volP=md->playbackVolume();
        if ( volP.hasVolume() )
        {
           volP.changeAllVolumes(volP.volumeStep(decrease));
        }
        
        Volume& volC=md->captureVolume();
        if ( volC.hasVolume() )
        {
           volC.changeAllVolumes(volC.volumeStep(decrease));
        }

        _mixerBackend->writeVolumeToHW(mixdeviceID, md);
    }
   ControlManager::instance().announce(md->mixer()->id(), ControlManager::Volume, QString("Mixer.increaseOrDecreaseVolume()"));

    /************************************************************
        It is important, not to implement this method like this:
    int vol=volume(mixdeviceID);
    setVolume(mixdeviceID, vol-5);
        It creates too big rounding errors. If you don't believe me, then
        do a decreaseVolume() and increaseVolume() with "vol.maxVolume() == 31".
    ***********************************************************/
}


bool Mixer::moveStream(const QString &id, const QString &destId)
{
    // We should really check that id is within our md's....
    bool ret = _mixerBackend->moveStream(id, destId);
    ControlManager::instance().announce(QString(), ControlManager::ControlList, QString("Mixer.moveStream()"));
    return (ret);
}


QString Mixer::currentStreamDevice(const QString &id) const
{
    return (_mixerBackend->currentStreamDevice(id));
}


QString Mixer::iconName() const
{
    const shared_ptr<MixDevice> master = getLocalMasterMD();
    if (master!=nullptr) return (master->iconName());
    return ("media-playback-start");			// fallback default icon
}
