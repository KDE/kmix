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

#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>

#include "backends/mixer_backend.h"
#include "backends/kmix-backends.cpp"
#include "core/ControlManager.h"
#include "core/volume.h"

/**
 * Some general design hints. Hierachy is Mixer->MixDevice->Volume
 */

QList<Mixer *> Mixer::s_mixers;
MasterControl Mixer::_globalMasterCurrent;
MasterControl Mixer::_globalMasterPreferred;
float Mixer::VOLUME_STEP_DIVISOR = 20;
float Mixer::VOLUME_PAGESTEP_DIVISOR = 10;
bool Mixer::m_beepOnVolumeChange = false;

int Mixer::numDrivers()
{
    MixerFactory *factory = g_mixerFactories;
    int num = 0;
    while( factory->getMixer!=0 )
    {
        num++;
        factory++;
    }

    return num;
}

/*
 * Returns a reference of the current mixer list.
 */
QList<Mixer *>& Mixer::mixers()
{
    return s_mixers;
}

/**
 * Returns whether there is at least one dynamic mixer active.
 * @returns true, if at least one dynamic mixer is active
 */
bool Mixer::dynamicBackendsPresent()
{
  foreach ( Mixer* mixer, Mixer::mixers() )
  {
    if ( mixer->isDynamic() )
      return true;
  }
  return false;
}

bool Mixer::pulseaudioPresent()
{
  foreach ( Mixer* mixer, Mixer::mixers() )
  {
    if ( mixer->getDriverName() == "PulseAudio" )
      return true;
  }
  return false;
}


Mixer::Mixer( QString& ref_driverName, int device )
    : m_balance(0), _mixerBackend(0L), m_dynamic(false)
{
    _cardInstance = 0;
    _mixerBackend = 0;
    int driverCount = numDrivers();
    for (int driver=0; driver<driverCount; driver++ ) {
        QString driverName = Mixer::driverName(driver);
        if ( driverName == ref_driverName ) {
            // driver found => retrieve Mixer factory for that driver
            getMixerFunc *f = g_mixerFactories[driver].getMixer;
            if( f!=0 ) {
                _mixerBackend = f( this, device );
                readSetFromHWforceUpdate();  // enforce an initial update on first readSetFromHW()
            }
            break;
        }
    }
}



Mixer::~Mixer() {
   // Close the mixer. This might also free memory, depending on the called backend method
   close();
   delete _mixerBackend;
}


/*
 * Find a Mixer. If there is no mixer with the given id, 0 is returned
 */
Mixer* Mixer::findMixer( const QString& mixer_id)
{
    Mixer *mixer = 0;
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
}


///**
// * Set the card instance. Usually this will be 1, but if there is
// * more than one card with the same name install, then you need
// * to use 2, 3, ...
// */
//void Mixer::setCardInstance(int cardInstance)
//{
//    _cardInstance = cardInstance;
//    recreateId();
//    // DBusMixerWrapper must be called after recreateId(), as it uses the id
//    new DBusMixerWrapper(this, dbusPath());
//}

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
            .arg(getDriverName())
            .arg(mixerName)
            .arg(_cardInstance);
    // The following 3 replaces are for not messing up the config file
    primaryKeyOfMixer.replace(']','_');
    primaryKeyOfMixer.replace('[','_'); // not strictly necessary, but lets play safe
    primaryKeyOfMixer.replace(' ','_');
    primaryKeyOfMixer.replace('=','_');
    _id = primaryKeyOfMixer;
	kDebug() << "Early _id=" << _id;
}

const QString Mixer::dbusPath()
{
	// _id needs to be fixed from the very beginning, as the MixDevice construction uses MixDevice::dbusPath().
	// So once the first MixDevice is created, this must return the correct value
	if (_id.isEmpty())
	{
		// Bug 308014: This a rather dirty hack, but it will guarantee that _id is definitely set.
		// Even the _cardInstance is set at default value during construction of the MixDevice instances
		recreateId();
	}

	kDebug() << "Late _id=" << _id;
//	kDebug() << "handMade=" << QString("/Mixers/" +  getDriverName() + "." + _mixerBackend->getId()).replace(" ", "x").replace(".", "_");

	// mixerName may contain arbitrary characters, so replace all that are not allowed to be be part of a DBUS path
	QString cardPath = _id;
	cardPath.replace(QRegExp("[^a-zA-Z0-9_]"), "_");
	cardPath.replace(QLatin1String("//"), QLatin1String("/"));

    return QString("/Mixers/" + cardPath);
}

void Mixer::volumeSave( KConfig *config )
{
    //    kDebug(67100) << "Mixer::volumeSave()";
    _mixerBackend->readSetFromHW();
    QString grp("Mixer");
    grp.append(id());
    _mixerBackend->m_mixDevices.write( config, grp );
}

void Mixer::volumeLoad( KConfig *config )
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
   for(int i=0; i<_mixerBackend->m_mixDevices.count() ; i++ )
   {
	   shared_ptr<MixDevice> md = _mixerBackend->m_mixDevices[i];
	   if ( md.get() == 0 )
		   continue;

       _mixerBackend->writeVolumeToHW( md->id(), md );
       if ( md->isEnum() )
    	   _mixerBackend->setEnumIdHW( md->id(), md->enumId() );
   }
}


/**
 * Opens the mixer.
 * Also, starts the polling timer, for polling the Volumes from the Mixer.
 *
 * @param cardId The cardId Usually this will be 1, but if there is
 * more than one card with the same name install, then you need
 * to use 2, 3, ...
 *
 * @return true, if Mixer could be opened.
 */
bool Mixer::openIfValid(int cardId)
{
	if (_mixerBackend == 0 )
	{
		// if we did not instantiate a suitable Backend, then Mixer is invalid
		return false;
	}

	_cardInstance = cardId;
    bool ok = _mixerBackend->openIfValid();
    if ( ok )
    {
        recreateId();
        shared_ptr<MixDevice> recommendedMaster = _mixerBackend->recommendedMaster();
        if ( recommendedMaster.get() != 0 )
        {
            QString recommendedMasterStr = recommendedMaster->id();
            setLocalMasterMD( recommendedMasterStr );
            kDebug() << "Mixer::open() detected master: " << recommendedMaster->id();
        }
        else
        {
            if ( !m_dynamic )
                kError(67100) << "Mixer::open() no master detected." << endl;
            QString noMaster = "---no-master-detected---";
            setLocalMasterMD(noMaster); // no master
        }
        connect( _mixerBackend, SIGNAL(controlChanged()), SIGNAL(controlChanged()) );
        new DBusMixerWrapper(this, dbusPath());
    }

    return ok;
}

/**
 * Closes the mixer.
 */
void Mixer::close()
{
	if ( _mixerBackend != 0)
		_mixerBackend->closeCommon();
}


/* ------- WRAPPER METHODS. START ------------------------------ */
unsigned int Mixer::size() const
{
  return _mixerBackend->m_mixDevices.count();
}

shared_ptr<MixDevice> Mixer::operator[](int num)
{
	shared_ptr<MixDevice> md =  _mixerBackend->m_mixDevices.at( num );
	return md;
}

MixSet& Mixer::getMixSet()
{
  return _mixerBackend->m_mixDevices;
}


/**
 * Returns the driver name, that handles this Mixer.
 */
QString Mixer::getDriverName()
{
  QString driverName = _mixerBackend->getDriverName();
//  kDebug(67100) << "Mixer::getDriverName() = " << driverName << "\n";
  return driverName;
}

bool Mixer::isOpen() const {
    if ( _mixerBackend == 0 )
        return false;
    else
        return _mixerBackend->isOpen();
}

void Mixer::readSetFromHWforceUpdate() const {
   _mixerBackend->readSetFromHWforceUpdate();
}

  /// Returns translated WhatsThis messages for a control.Translates from 
QString Mixer::translateKernelToWhatsthis(const QString &kernelName)
{
   return _mixerBackend->translateKernelToWhatsthis(kernelName);
}

/* ------- WRAPPER METHODS. END -------------------------------- */

void Mixer::setBeepOnVolumeChange(bool beepOnVolumeChange)
{
	m_beepOnVolumeChange = beepOnVolumeChange;
}

int Mixer::balance() const {
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
   if ( master.get() == 0 )
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
   //_mixerBackend->readVolumeFromHW( master->id(), master );

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
QString Mixer::readableName()
{
	QString finalName = _mixerBackend->getName();
//	QString finalName = mixerName.left(mixerName.length() - 2);
	if ( getCardInstance() > 1)
		finalName = finalName.append(" %1").arg(getCardInstance());

	return finalName;
}


QString Mixer::getBaseName()
{
  return _mixerBackend->getName();
}

/**
 * Queries the Driver Factory for a driver.
 * @par driver Index number. 0 <= driver < numDrivers()
 */
QString Mixer::driverName( int driver )
{
    getDriverNameFunc *f = g_mixerFactories[driver].getDriverName;
    if( f!=0 )
        return f();
    else
        return "unknown";
}

/* obsoleted by setInstance()
void Mixer::setID(QString& ref_id)
{
  _id = ref_id;
}
*/

QString& Mixer::id()
{
  return _id;
}

QString& Mixer::udi(){
    return _mixerBackend->udi();
}

/**
 * Set the global master, which is shown in the dock area and which is accesible via the
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
void Mixer::setGlobalMaster(QString ref_card, QString ref_control, bool preferred)
{
    kDebug() << "ref_card=" << ref_card << ", ref_control=" << ref_control << ", preferred=" << preferred;
    _globalMasterCurrent.set(ref_card, ref_control);
    if ( preferred )
        _globalMasterPreferred.set(ref_card, ref_control);
    kDebug() << "Mixer::setGlobalMaster() card=" <<ref_card<< " control=" << ref_control;
}

Mixer* Mixer::getGlobalMasterMixerNoFalback()
{
   foreach ( Mixer* mixer, Mixer::mixers())
   {
      if ( mixer != 0 && mixer->id() == _globalMasterCurrent.getCard() )
         return mixer;
   }
   return 0;
}

Mixer* Mixer::getGlobalMasterMixer()
{
   Mixer *mixer = getGlobalMasterMixerNoFalback();
   if ( mixer == 0 && Mixer::mixers().count() > 0 ) {
      mixer = Mixer::mixers()[0];       // produce fallback
   }
   //kDebug() << "Mixer::masterCard() returns " << mixer->id();
   return mixer;
}


/**
 * Return the preferred global master.
 * If there is no preferred global master, returns the current master instead.
 */
MasterControl& Mixer::getGlobalMasterPreferred()
{
    if ( _globalMasterPreferred.isValid() ) {
        kDebug() << "Returning preferred master";
        return _globalMasterPreferred;
    }
    else {
        kDebug() << "Returning current master";
        return _globalMasterCurrent;
    }
}


shared_ptr<MixDevice> Mixer::getGlobalMasterMD()
{
   return getGlobalMasterMD(true);
}


shared_ptr<MixDevice> Mixer::getGlobalMasterMD(bool fallbackAllowed)
{
	shared_ptr<MixDevice> mdRet;
	shared_ptr<MixDevice> firstDevice;
	Mixer *mixer = fallbackAllowed ?
		   Mixer::getGlobalMasterMixer() : Mixer::getGlobalMasterMixerNoFalback();

	if ( mixer == 0 )
		return mdRet;

	foreach (shared_ptr<MixDevice> md, mixer->_mixerBackend->m_mixDevices )
	{
		if ( md.get() == 0 )
			continue; // invalid

		firstDevice=md;
		if ( md->id() == _globalMasterCurrent.getControl() )
		{
			mdRet = md;
			break; // found
		}
	}
	if ( mdRet.get() == 0 )
	{
	  //For some sound cards when using pulseaudio the mixer id is not proper hence returning the first device as master channel device
	  //This solves the bug id:290177 and problems stated in review #105422
		kDebug() << "Mixer::masterCardDevice() returns 0 (no globalMaster), returning the first device";
		mdRet=firstDevice;
	}

	return mdRet;
}




shared_ptr<MixDevice> Mixer::getLocalMasterMD()
{
  return find( _masterDevicePK );
}

void Mixer::setLocalMasterMD(QString &devPK)
{
    _masterDevicePK = devPK;
}


shared_ptr<MixDevice> Mixer::find(const QString& mixdeviceID)
{

	shared_ptr<MixDevice> mdRet;

	foreach (shared_ptr<MixDevice> md, _mixerBackend->m_mixDevices )
	{
		if ( md.get() == 0 )
			continue; // invalid
		if ( md->id() == mixdeviceID )
		{
			mdRet = md;
			break; // found
		}
	}

    return mdRet;
}


shared_ptr<MixDevice> Mixer::getMixdeviceById( const QString& mixdeviceID )
{
	shared_ptr<MixDevice> md;
   int num = _mixerBackend->id2num(mixdeviceID);
   if ( num!=-1 && num < (int)size() )
   {
      md = (*this)[num];
   }
   return md;
}

/**
   Call this if you have a *reference* to a Volume object and have modified that locally.
   Pass the MixDevice associated to that Volume to this method for writing back
   the changed value to the mixer.
   Hint: Why do we do it this way?
   - It is fast               (no copying of Volume objects required)
   - It is easy to understand ( read - modify - commit )
*/
void Mixer::commitVolumeChange( shared_ptr<MixDevice> md )
{
  _mixerBackend->writeVolumeToHW(md->id(), md );
   if (md->isEnum())
   {
     _mixerBackend->setEnumIdHW(md->id(), md->enumId() );
   }
   if ( md->captureVolume().hasSwitch() )
   {
      // Make sure to re-read the hardware, because setting capture might have failed.
      // This is due to exclusive capture groups.
      // If we wouldn't do this, KMix might show a Capture Switch disabled, but
      // in reality the capture switch is still on.
      //
      // We also cannot rely on a notification from the driver (SocketNotifier), because
      // nothing has changed, and so there s nothing to notify.
      _mixerBackend->readSetFromHWforceUpdate();
      kDebug() << "commiting a control with capture volume, that might announce: " << md->id();
      _mixerBackend->readSetFromHW();
   }
      kDebug() << "commiting announces the change of: " << md->id();
      // We announce the change we did, so all other parts of KMix can pick up the change
      ControlManager::instance().announce(md->mixer()->id(), ControlChangeType::Volume, QString("Mixer.commitVolumeChange()"));
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

void Mixer::increaseOrDecreaseVolume( const QString& mixdeviceID, bool decrease )
{

	shared_ptr<MixDevice> md= getMixdeviceById( mixdeviceID );
    if (md.get() != 0)
    {
        Volume& volP=md->playbackVolume();
        if ( volP.hasVolume() ) {
        	long volSpan = volP.volumeSpan();
           double step = volSpan / Mixer::VOLUME_STEP_DIVISOR;
           if ( step < 1 ) step = 1;
           if ( decrease ) step = -step;
           volP.changeAllVolumes(step);
        }
        
        Volume& volC=md->captureVolume();
        if ( volC.hasVolume() ) {
        	long volSpan = volC.volumeSpan();
           double step = volSpan / Mixer::VOLUME_STEP_DIVISOR;
           if ( step < 1 ) step = 1;
           if ( decrease ) step = -step;
           volC.changeAllVolumes(step);
        }

        _mixerBackend->writeVolumeToHW(mixdeviceID, md);
    }
   ControlManager::instance().announce(md->mixer()->id(), ControlChangeType::Volume, QString("Mixer.increaseOrDecreaseVolume()"));

    /************************************************************
        It is important, not to implement this method like this:
    int vol=volume(mixdeviceID);
    setVolume(mixdeviceID, vol-5);
        It creates too big rounding errors. If you don't believe me, then
        do a decreaseVolume() and increaseVolume() with "vol.maxVolume() == 31".
    ***********************************************************/
}


void Mixer::setDynamic ( bool dynamic )
{
    m_dynamic = dynamic;
}

bool Mixer::isDynamic()
{
    return m_dynamic;
}

bool Mixer::moveStream( const QString id, const QString& destId )
{
    // We should really check that id is within our md's....
    bool ret = _mixerBackend->moveStream( id, destId );
    ControlManager::instance().announce(QString(), ControlChangeType::ControlList, QString("Mixer.moveStream()"));
    return ret;
}

#include "mixer.moc"
