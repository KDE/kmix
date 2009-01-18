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


#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>

#include "mixer.h"
#include "mixer_backend.h"
#include "kmix-platforms.cpp"
#include "volume.h"
#include "kmixadaptor.h"

/**
 * Some general design hints. Hierachy is Mixer->MixDevice->Volume
 */

QList<Mixer *> Mixer::s_mixers;
QString Mixer::_globalMasterCard;
QString Mixer::_globalMasterCardDevice;

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

Mixer::Mixer( QString& ref_driverName, int device )
    : m_balance(0), _mixerBackend(0L)
{
   (void)new KMixAdaptor(this);

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
    if ( ! m_dbusName.isEmpty() ) {
        kDebug(67100) << "Auto-unregistering DBUS object " << m_dbusName;
    //QDBusConnection::sessionBus().unregisterObject(m_dbusName);
   }
   close();
   delete _mixerBackend;
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
   _mixerBackend->m_mixDevices.read( config, grp );

   // set new settings
   //QListIterator<MixDevice*> it( _mixerBackend->m_mixDevices );
   for(int i=0; i<_mixerBackend->m_mixDevices.count() ; i++ )
   {
       MixDevice *md = _mixerBackend->m_mixDevices[i];
       _mixerBackend->setRecsrcHW( md->id(), md->isRecSource() );
       _mixerBackend->writeVolumeToHW( md->id(), md );
       if ( md->isEnum() ) _mixerBackend->setEnumIdHW( md->id(), md->enumId() );
   }
}


/**
 * Opens the mixer.
 * Also, starts the polling timer, for polling the Volumes from the Mixer.
 *
 * @return 0, if OK. An Mixer::ERR_ error code otherwise
 */
bool Mixer::openIfValid() {
    bool ok = _mixerBackend->openIfValid();
    if ( ok ) {
        _id = baseName();
        MixDevice* recommendedMaster = _mixerBackend->recommendedMaster();
        if ( recommendedMaster != 0 ) {
            QString recommendedMasterStr = recommendedMaster->id();
            setLocalMasterMD( recommendedMasterStr );
            kDebug() << "Mixer::open() detected master: " << recommendedMaster->id();
        }
        else {
            kError(67100) << "Mixer::open() no master detected." << endl;
            QString noMaster = "---no-master-detected---";
            setLocalMasterMD(noMaster); // no master
        }
        connect( _mixerBackend, SIGNAL(controlChanged()), SLOT(controlChangedForwarder()) );
        
        m_dbusName = "/Mixer" + QString::number(_mixerBackend->m_devnum);
        QDBusConnection::sessionBus().registerObject(m_dbusName, this);
    }

    return ok;
}

void Mixer::controlChangedForwarder()
{
    emit controlChanged();
}

/**
 * Closes the mixer.
 * Also, stops the polling timer.
 *
 * @return 0 (always)
 */
int Mixer::close()
{
  return _mixerBackend->close();
}


/* ------- WRAPPER METHODS. START ------------------------------ */
unsigned int Mixer::size() const
{
  return _mixerBackend->m_mixDevices.count();
}

MixDevice* Mixer::operator[](int num)
{
  MixDevice* md =  _mixerBackend->m_mixDevices.at( num );
  Q_ASSERT( md );
  return md;
}

MixSet Mixer::getMixSet()
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

/* ------- WRAPPER METHODS. END -------------------------------- */




void Mixer::setBalance(int balance)
{
   if( balance == m_balance ) {
      // balance unchanged => return
      return;
   }

   m_balance = balance;

   MixDevice* master = getLocalMasterMD();
   if ( master == 0 ) {
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

   int left = vol[ Volume::LEFT ];
   int right = vol[ Volume::RIGHT ];
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

QString Mixer::baseName()
{
  return _mixerBackend->m_mixerName;
}

// should return a name suitable for a human user to read (on a label, ...)
QString Mixer::readableName()
{
  if ( _mixerBackend->m_mixerName.endsWith(":0"))
     return _mixerBackend->m_mixerName.left(_mixerBackend->m_mixerName.length() - 2);
  else
     return _mixerBackend->m_mixerName;
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

void Mixer::setID(QString& ref_id)
{
  _id = ref_id;
}


QString& Mixer::id()
{
  return _id;
}

QString& Mixer::udi(){
    return _mixerBackend->udi();
}
void Mixer::setGlobalMaster(QString& ref_card, QString& ref_control)
{
  // The value is taken over without checking on existence. This allows the User to define
  // a MasterCard that is not always available (e.g. it is an USB hotplugging device).
  // Also you can set the master at any time you like, e.g. after reading the KMix configuration file
  // and before actually constructing the Mixer instances (hint: this mehtod is static!).
  _globalMasterCard       = ref_card;
  _globalMasterCardDevice = ref_control;
  kDebug() << "Mixer::setGlobalMaster() card=" <<ref_card<< " control=" << ref_control;
}

Mixer* Mixer::getGlobalMasterMixerNoFalback()
{
   Mixer *mixer = 0;
   if(Mixer::mixers().count() == 0)
      return mixer;

   for (int i=0; i< Mixer::mixers().count(); ++i )
   {
      Mixer* mixerTmp = Mixer::mixers()[i];
      if ( mixerTmp != 0 && mixerTmp->id() == _globalMasterCard ) {
         //kDebug() << "Mixer::masterCard() found " << _globalMasterCard;
         mixer = mixerTmp;
         break;
      }
   }
   return mixer;
}

Mixer* Mixer::getGlobalMasterMixer()
{
   Mixer *mixer = getGlobalMasterMixerNoFalback();
   if ( mixer == 0 && Mixer::mixers().count() > 0 ) {
      // produce fallback
      mixer = Mixer::mixers()[0];
      _globalMasterCard = mixer->id();
      kDebug() << "Mixer::masterCard() fallback to  " << _globalMasterCard;
   }
   //kDebug() << "Mixer::masterCard() returns " << mixer->id();
   return mixer;
}

MixDevice* Mixer::getGlobalMasterMD()
{
   return getGlobalMasterMD(true);
}

MixDevice* Mixer::getGlobalMasterMD(bool fallbackAllowed)
{
   MixDevice* md = 0;
   Mixer *mixer;
   if ( fallbackAllowed)
      mixer = Mixer::getGlobalMasterMixer();
   else
      mixer = Mixer::getGlobalMasterMixerNoFalback();
   if ( mixer != 0 ) {
      for(int i=0; i < mixer->_mixerBackend->m_mixDevices.count() ; i++ )
      {
         md = mixer->_mixerBackend->m_mixDevices[i];
         if ( md->id() == _globalMasterCardDevice ) {
            //kDebug() << "Mixer::masterCardDevice() found " << _globalMasterCardDevice;
            break;
         }
      }
   }
   if ( ! md ) 
        kDebug() << "Mixer::masterCardDevice() returns 0 (no globalMaster)";
   return md;
}




MixDevice* Mixer::getLocalMasterMD()
{
  return find( _masterDevicePK );
}

void Mixer::setLocalMasterMD(QString &devPK)
{
    _masterDevicePK = devPK;
}



/**
   Used internally by KMix and as DBUS method
*/
void Mixer::setRecordSource( const QString& mixdeviceID, bool on )
{
   _mixerBackend->setRecsrcHW( mixdeviceID, on );
}





MixDevice* Mixer::find(const QString& mixdeviceID)
{
   MixDevice *md = 0;
   for(int i=0; i<_mixerBackend->m_mixDevices.count() ; i++ )
   {
       md = _mixerBackend->m_mixDevices[i];
       if( mixdeviceID == md->id() ) {
           break;
       }
    }
    return md;
}


MixDevice* Mixer::getMixdeviceById( const QString& mixdeviceID )
{
   MixDevice* md = 0;
   int num = _mixerBackend->id2num(mixdeviceID);
   if ( num!=-1 && num < (int)size() ) {
      md = (*this)[num];
   }
   return md;
}

// @dcop
// Used also by the setMasterVolume() method.
void Mixer::setVolume( const QString& mixdeviceID, int percentage )
{
    MixDevice *md = getMixdeviceById( mixdeviceID );
    if (!md) return;

    Volume& volP = md->playbackVolume();
    Volume& volC = md->captureVolume();

    // @todo The next call doesn't handle negative volumes correctly.
    volP.setAllVolumes( (percentage*volP.maxVolume())/100 );
    volC.setAllVolumes( (percentage*volC.maxVolume())/100 );
    _mixerBackend->writeVolumeToHW(mixdeviceID, md);
}

/**
   Call this if you have a *reference* to a Volume object and have modified that locally.
   Pass the MixDevice associated to that Volume to this method for writing back
   the changed value to the mixer.
   Hint: Why do we do it this way?
   - It is fast               (no copying of Volume objects required)
   - It is easy to understand ( read - modify - commit )
*/
void Mixer::commitVolumeChange( MixDevice* md ) {
  _mixerBackend->writeVolumeToHW(md->id(), md );
   if (md->isEnum()) _mixerBackend->setEnumIdHW(md->id(), md->enumId() );
   if ( md->captureVolume().hasSwitch() ) {
      // Make sure to re-read the hardware, because seting capture might have failed.
      // This is due to exclusive capture groups.
      // If we wouldn't do this, KMix might show a Capture Switch disabled, but
      // in reality the capture switch is still on.
      //
      // We also cannot rely on a notification from the driver (SocketNotifier), because
      // nothing has changed, and so there s nothing to notify.
      _mixerBackend->readSetFromHWforceUpdate();
      _mixerBackend->readSetFromHW();
   }
}

// @dcop only
void Mixer::setMasterVolume( int percentage )
{
  MixDevice *master = getLocalMasterMD();
  if (master != 0 ) {
    setVolume( master->id(), percentage );
  }
}

// @dcop
int Mixer::volume( const QString& mixdeviceID )
{
  MixDevice *md= getMixdeviceById( mixdeviceID );
  if (!md) return 0;

  Volume vol=md->playbackVolume();  // @todo Is hardcoded to PlaybackVolume
  // @todo This will not work, if minVolume != 0      !!!
  //       e.g.: minVolume=5 or minVolume=-10
  // The solution is to check two cases:
  //     volume < 0 => use minVolume for volumeRange
  //     volume > 0 => use maxVolume for volumeRange
  //     If chosen volumeRange==0 => return 0
  // As this is potentially used often (Sliders, ...), it
  // should beimplemented in the Volume class.

  // For now we go with "maxVolume()", like in the rest of KMix.
  long volumeRange = vol.maxVolume(); // -vol.minVolume() ;
  if ( volumeRange == 0 )
  {
    return 0;
  }
  else
  {
     return ( vol.getVolume( Volume::LEFT )*100) / volumeRange ;
  }
}

// @dcop , especially for use in KMilo
void Mixer::setAbsoluteVolume( const QString& mixdeviceID, long absoluteVolume ) {
    MixDevice *md= getMixdeviceById( mixdeviceID );
    if (!md) return;

    Volume& volP=md->playbackVolume();
    Volume& volC=md->captureVolume();
    volP.setAllVolumes( absoluteVolume );
    volC.setAllVolumes( absoluteVolume );
    _mixerBackend->writeVolumeToHW(mixdeviceID, md);
}

// @dcop , especially for use in KMilo
long Mixer::absoluteVolume( const QString& mixdeviceID )
{
    MixDevice *md = getMixdeviceById( mixdeviceID );
    if (!md) return 0;

    Volume& volP=md->playbackVolume();  // @todo Is hardcoded to PlaybackVolume
    long avgVolume=volP.getAvgVolume((Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT));
    return avgVolume;
}

// @dcop , especially for use in KMilo
long Mixer::absoluteVolumeMax( const QString& mixdeviceID )
{
   MixDevice *md= getMixdeviceById( mixdeviceID );
   if (!md) return 0;

   Volume vol=md->playbackVolume();  // @todo Is hardcoded to PlaybackVolume
   long maxVolume=vol.maxVolume();
   return maxVolume;
}

// @dcop , especially for use in KMilo
long Mixer::absoluteVolumeMin( const QString& mixdeviceID )
{
   MixDevice *md= getMixdeviceById( mixdeviceID );
   if (!md) return 0;

   Volume vol=md->playbackVolume();  // @todo Is hardcoded to PlaybackVolume
   long minVolume=vol.minVolume();
   return minVolume;
}

// @dcop
int Mixer::masterVolume()
{
  int vol = 0;
  MixDevice *master = getLocalMasterMD();
  if (master != 0 ) {
    vol = volume( master->id() );
  }
  return vol;
}

// @dbus
QString Mixer::masterDeviceIndex()
{
  MixDevice *master = getLocalMasterMD();
  return master ? master->id() : QString();
}


// @dcop
void Mixer::increaseVolume( const QString& mixdeviceID )
{
    MixDevice *md= getMixdeviceById( mixdeviceID );
    if (md != 0) {
        Volume& volP=md->playbackVolume();
        if ( volP.hasVolume() ) {
           double step = (volP.maxVolume()-volP.minVolume()+1) / 20;
           if ( step < 1 ) step = 1;
           volP.changeAllVolumes(step);
        }
        
        Volume& volC=md->captureVolume();
        if ( volC.hasVolume() ) {
           double step = (volC.maxVolume()-volC.minVolume()+1) / 20;
           if ( step < 1 ) step = 1;
           volC.changeAllVolumes(step);
        }

        _mixerBackend->writeVolumeToHW(mixdeviceID, md);
    }

  /* see comment at the end of decreaseVolume()
  int vol=volume(mixdeviceID);
  setVolume(mixdeviceID, vol+5);
  */
}

// @dcop
void Mixer::decreaseVolume( const QString& mixdeviceID )
{
    MixDevice *md= getMixdeviceById( mixdeviceID );
    if (md != 0) {
        Volume& volP=md->playbackVolume();
        if ( volP.hasVolume() ) {
           double step = (volP.maxVolume()-volP.minVolume()+1) / 20;
           if ( step < 1 ) step = 1;
           volP.changeAllVolumes(-step);
        }
        
        Volume& volC=md->captureVolume();
        if ( volC.hasVolume() ) {
           double step = (volC.maxVolume()-volC.minVolume()+1) / 20;
           if ( step < 1 ) step = 1;
           volC.changeAllVolumes(-step);
        }
    }

    _mixerBackend->writeVolumeToHW(mixdeviceID, md);

    /************************************************************
        It is important, not to implement this method like this:
    int vol=volume(mixdeviceID);
    setVolume(mixdeviceID, vol-5);
        It creates too big rounding errors. If you don't beleive me, then
        do a decreaseVolume() and increaseVolume() with "vol.maxVolume() == 31".
    ***********************************************************/
}

// @dcop
void Mixer::setMute( const QString& mixdeviceID, bool on )
{
  MixDevice *md= getMixdeviceById( mixdeviceID );
  if (!md) return;

  md->setMuted( on );

     _mixerBackend->writeVolumeToHW(mixdeviceID, md);
}

// @dcop
void Mixer::toggleMute( const QString& mixdeviceID )
{
    MixDevice *md= getMixdeviceById( mixdeviceID );
    if (!md) return;

    md->setMuted( ! md->isMuted() );
    _mixerBackend->writeVolumeToHW(mixdeviceID, md);
}

// @dcop
bool Mixer::mute( const QString& mixdeviceID )
{
  MixDevice *md= getMixdeviceById( mixdeviceID );
  if (!md) return true;

  return md->isMuted();
}

bool Mixer::isRecordSource( const QString& mixdeviceID )
{
  MixDevice *md= getMixdeviceById( mixdeviceID );
  if (!md) return false;

  return md->isRecSource();
}

/// @DCOP    WHAT DOES THIS METHOD?!?!?
bool Mixer::isAvailableDevice( const QString& mixdeviceID )
{
  return getMixdeviceById( mixdeviceID );
}

#include "mixer.moc"
