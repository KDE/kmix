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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <dcopobject.h>

#include "mixer.h"
#include "mixer_backend.h"
#include "kmix-platforms.cpp"
#include "volume.h"

//#define MIXER_MASTER_DEBUG

#ifdef MIXER_MASTER_DEBUG
#warning MIXER_MASTER_DEBUG is enabled. DO NOT SHIP KMIX LIKE THIS !!!
#endif

/**
 * Some general design hints. Hierachy is Mixer->MixDevice->Volume
 */

// !! Warning: Don't commit with "KMIX_DCOP_OBJID_TEST" #define'd (cesken)
#undef KMIX_DCOP_OBJID_TEST
int Mixer::_dcopID = 0;

QPtrList<Mixer> Mixer::s_mixers;
QString Mixer::_masterCard;
QString Mixer::_masterCardDevice;

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
QPtrList<Mixer>& Mixer::mixers()
{
  return s_mixers;
}


Mixer::Mixer( int driver, int device ) : DCOPObject( "Mixer" )
{
  _pollingTimer = 0;

   _mixerBackend = 0;
   getMixerFunc *f = g_mixerFactories[driver].getMixer;
   if( f!=0 ) {
     _mixerBackend = f( device );
   }

  readSetFromHWforceUpdate();  // enforce an initial update on first readSetFromHW()

  m_balance = 0;
  m_profiles.setAutoDelete( true );

  _pollingTimer = new QTimer(); // will be started on open() and stopped on close()
  connect( _pollingTimer, SIGNAL(timeout()), this, SLOT(readSetFromHW()));

  QCString objid;
#ifndef KMIX_DCOP_OBJID_TEST
  objid.setNum(_mixerBackend->m_devnum);
#else
// should use a nice name like the Unique-Card-ID instead !!
  objid.setNum(Mixer::_dcopID);
  Mixer::_dcopID ++;
#endif
  objid.prepend("Mixer");
  DCOPObject::setObjId( objid );

}

Mixer::~Mixer() {
   // Close the mixer. This might also free memory, depending on the called backend method
   close();
   delete _pollingTimer;
}

void Mixer::volumeSave( KConfig *config )
{
    //    kdDebug(67100) << "Mixer::volumeSave()" << endl;
    readSetFromHW();
    QString grp("Mixer");
    grp.append(mixerName());
    _mixerBackend->m_mixDevices.write( config, grp );
}

void Mixer::volumeLoad( KConfig *config )
{
   QString grp("Mixer");
   grp.append(mixerName());
   if ( ! config->hasGroup(grp) ) {
      // no such group. Volumes (of this mixer) were never saved beforehand.
      // Thus don't restore anything (also see Bug #69320 for understanding the real reason)
      return; // make sure to bail out immediately
   }

   // else restore the volumes
   _mixerBackend->m_mixDevices.read( config, grp );

   // set new settings
   QPtrListIterator<MixDevice> it( _mixerBackend->m_mixDevices );
   for(MixDevice *md=it.toFirst(); md!=0; md=++it )
   {
       //       kdDebug(67100) << "Mixer::volumeLoad() writeVolumeToHW(" << md->num() << ", "<< md->getVolume() << ")" << endl;
       // !! @todo Restore record source
       //setRecordSource( md->num(), md->isRecSource() );
       _mixerBackend->setRecsrcHW( md->num(), md->isRecSource() );
       _mixerBackend->writeVolumeToHW( md->num(), md->getVolume() );
       if ( md->isEnum() ) _mixerBackend->setEnumIdHW( md->num(), md->enumId() );
   }
}


/**
 * Opens the mixer.
 * Also, starts the polling timer, for polling the Volumes from the Mixer.
 *
 * @return 0, if OK. An Mixer::ERR_ error code otherwise
 */
int Mixer::open()
{
      int err = _mixerBackend->open();
      // A better ID is now calculated in mixertoolbox.cpp, and set via setID(),
      // but we want a somhow usable fallback just in case.
      _id = mixerName();

      if( err == ERR_INCOMPATIBLESET )   // !!! When does this happen ?!?
        {
          // Clear the mixdevices list
	  _mixerBackend->m_mixDevices.clear();
          // try again with fresh set
	  err = _mixerBackend->open();
        }

      MixDevice* recommendedMaster = _mixerBackend->recommendedMaster();
      if ( recommendedMaster != 0 ) {
         setMasterDevice(recommendedMaster->getPK() );
      }
      else {
         kdError(67100) << "Mixer::open() no master detected." << endl;
         QString noMaster = "---no-master-detected---";
         setMasterDevice(noMaster); // no master
      }
	/*
   // --------- Copy the hardware values to the MixDevice -------------------
      MixSet &mset = _mixerBackend->m_mixDevices;
      if( !mset.isEmpty() ) {
       // Copy the initial mix set
       //       kdDebug(67100) << "Mixer::setupMixer() copy Set" << endl;
	MixDevice* md;
	for( md = mset.first(); md != 0; md = mset.next() )
	{
	  MixDevice* mdCopy = _mixerBackend->m_mixDevices.first();
	  while( mdCopy!=0 && mdCopy->num() != md->num() ) {
	    mdCopy = _mixerBackend->m_mixDevices.next();
	  }
	  if ( mdCopy != 0 ) {
	       // The "mdCopy != 0" was not checked before, but its safer to do so
	    setRecordSource( md->num(), md->isRecSource() );
	    Volume &vol = mdCopy->getVolume();
	    vol.setVolume( md->getVolume() );
	    mdCopy->setMuted( md->isMuted() );

	       // !! might need writeVolumeToHW( mdCopy->num(), mdCopy->getVolume() );
	  }
	}
      }
	*/
      if ( _mixerBackend->needsPolling() ) {
          _pollingTimer->start(50);
      }
      else {
          _mixerBackend->prepareSignalling(this);
          // poll once to give the GUI a chance to rebuild it's info
          QTimer::singleShot( 50, this, SLOT( readSetFromHW() ) );
      }
      return err;
}


/**
 * Closes the mixer.
 * Also, stops the polling timer.
 *
 * @return 0 (always)
 */
int Mixer::close()
{
  _pollingTimer->stop();
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

bool Mixer::isValid() {
  return _mixerBackend->isValid();
}

bool Mixer::isOpen() const {
    if ( _mixerBackend == 0 )
        return false;
    else
        return _mixerBackend->isOpen();
}

/* ------- WRAPPER METHODS. END -------------------------------- */

/**
 * After calling this, readSetFromHW() will do a complete update. This will
 * trigger emitting the appropriate signals like newVolumeLevels().
 *
 * This method is useful, if you need to get a "refresh signal" - used at:
 * 1) Start of KMix - so that we can be sure an initial signal is emitted
 * 2) When reconstructing any MixerWidget (e.g. DockIcon after applying preferences)
 */
void Mixer::readSetFromHWforceUpdate() const {
   _readSetFromHWforceUpdate = true;
}

/**
   You can call this to retrieve the freshest information from the mixer HW.
   This method is also called regulary by the mixer timer.
*/
void Mixer::readSetFromHW()
{
  if ( ! _mixerBackend->isOpen() ) {
     // bail out immediately, if the mixer is not open.
     // This can happen currently only, if the user executes the DCOP close() call.
     return;
  }
  bool updated = _mixerBackend->prepareUpdateFromHW();
  if ( (! updated) && (! _readSetFromHWforceUpdate) ) {
    // Some drivers (ALSA) are smart. We don't need to run the following
    // time-consuming update loop if there was no change
    return;
  }
  _readSetFromHWforceUpdate = false;
  MixDevice* md;
  for( md = _mixerBackend->m_mixDevices.first(); md != 0; md = _mixerBackend->m_mixDevices.next() )
    {
      Volume& vol = md->getVolume();
      _mixerBackend->readVolumeFromHW( md->num(), vol );
      md->setRecSource( _mixerBackend->isRecsrcHW( md->num() ) );
      if (md->isEnum() ) {
	md->setEnumId( _mixerBackend->enumIdHW(md->num()) );
      }
    }
  // Trivial implementation. Without looking at the devices
  //  kdDebug(67100) << "Mixer::readSetFromHW(): emit newVolumeLevels()" << endl;
  emit newVolumeLevels();
  emit newRecsrc(); // cheap, but works
}


void Mixer::setBalance(int balance)
{
  // !! BAD, because balance only works on the master device. If you have not Master, the slider is a NOP
  if( balance == m_balance ) {
      // balance unchanged => return
      return;
  }

  m_balance = balance;

  MixDevice* master = masterDevice();
  if ( master == 0 ) {
      // no master device available => return
      return;
  }

  Volume& vol = master->getVolume();
  _mixerBackend->readVolumeFromHW( master->num(), vol );

  int left = vol[ Volume::LEFT ];
  int right = vol[ Volume::RIGHT ];
  int refvol = left > right ? left : right;
  if( balance < 0 ) // balance left
    {
      vol.setVolume( Volume::LEFT,  refvol);
      vol.setVolume( Volume::RIGHT, (balance * refvol) / 100 + refvol );
    }
  else
    {
      vol.setVolume( Volume::LEFT, -(balance * refvol) / 100 + refvol );
      vol.setVolume( Volume::RIGHT,  refvol);
    }

    _mixerBackend->writeVolumeToHW( master->num(), vol );

  emit newBalance( vol );
}

QString Mixer::mixerName()
{
  return _mixerBackend->m_mixerName;
}

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

void Mixer::setMasterCard(QString& ref_id)
{
  // The value is taken over without checking on existance. This allows the User to define
  // a MasterCard that is not always available (e.g. it is an USB hotplugging device).
  // Also you can set the master at any time you like, e.g. after reading the KMix configuration file
  // and before actually constructing the Mixer instances (hint: this mehtod is static!).
  _masterCard = ref_id;
}

Mixer* Mixer::masterCard()
{
  Mixer *mixer = 0;
  kdDebug(67100) << "Mixer::masterCard() searching for id=" << _masterCard << "\n";
  for (mixer=Mixer::mixers().first(); mixer!=0; mixer=Mixer::mixers().next())
  {
     if ( mixer->id() == _masterCard ) {
#ifdef MIXER_MASTER_DEBUG
        kdDebug(67100) << "Mixer::masterCard() found id=" << mixer->id() << "\n";
#endif
        break;
     }
  }
#ifdef MIXER_MASTER_DEBUG
  if ( mixer == 0) kdDebug(67100) << "Mixer::masterCard() found no Mixer* mixer \n";
#endif
  return mixer;
}

void Mixer::setMasterCardDevice(QString& ref_id)
{
  // The value is taken over without checking on existance. This allows the User to define
  // a MasterCard that is not always available (e.g. it is an USB hotplugging device).
  // Also you can set the master at any time you like, e.g. after reading the KMix configuration file
  // and before actually constructing the Mixer instances (hint: this mehtod is static!).
  _masterCardDevice = ref_id;
#ifdef MIXER_MASTER_DEBUG
   kdDebug(67100) << "Mixer::setMasterCardDevice(\"" << ref_id << "\")\n";
#endif
}

MixDevice* Mixer::masterCardDevice()
{
  MixDevice* md = 0;
  Mixer *mixer = masterCard();
  if ( mixer != 0 ) {
     for( md = mixer->_mixerBackend->m_mixDevices.first(); md != 0; md = mixer->_mixerBackend->m_mixDevices.next() ) {


       if ( md->getPK() == _masterCardDevice )
        {
#ifdef MIXER_MASTER_DEBUG
         kdDebug(67100) << "Mixer::masterCardDevice() getPK()="
         << md->getPK() << " , _masterCardDevice="
         << _masterCardDevice << "\n";
#endif
          break;
        }
     }
  }

#ifdef MIXER_MASTER_DEBUG
  if ( md == 0) kdDebug(67100) << "Mixer::masterCardDevice() found no MixDevice* md" "\n";
#endif

  return md;
}




/**
   Used internally by the Mixer class and as DCOP method
*/
void Mixer::setRecordSource( int devnum, bool on )
{
  if( !_mixerBackend->setRecsrcHW( devnum, on ) ) // others have to be updated
  {
    for( MixDevice* md = _mixerBackend->m_mixDevices.first(); md != 0; md = _mixerBackend->m_mixDevices.next() ) {
	  bool isRecsrc =  _mixerBackend->isRecsrcHW( md->num() );
//		kdDebug(67100) << "Mixer::setRecordSource(): isRecsrcHW(" <<  md->num() << ") =" <<  isRecsrc << endl;
		md->setRecSource( isRecsrc );
	}
	// emitting is done after read
	//emit newRecsrc(); // like "emit newVolumeLevels()", but for record source
  }
  else {
	// just the actual mixdevice
    for( MixDevice* md = _mixerBackend->m_mixDevices.first(); md != 0; md = _mixerBackend->m_mixDevices.next() ) {
		  if( md->num() == devnum ) {
		    bool isRecsrc =  _mixerBackend->isRecsrcHW( md->num() );
			md->setRecSource( isRecsrc );
		  }
	}
	// emitting is done after read
	//emit newRecsrc(); // like "emit newVolumeLevels()", but for record source
  }

}


MixDevice* Mixer::masterDevice()
{
  return find( _masterDevicePK );
}

void Mixer::setMasterDevice(QString &devPK)
{
    _masterDevicePK = devPK;
}


MixDevice* Mixer::find(QString& devPK)
{
    MixDevice* md = 0;
    for( md = _mixerBackend->m_mixDevices.first(); md != 0; md = _mixerBackend->m_mixDevices.next() ) {
        if( devPK == md->getPK() ) {
           break;
        }
    }
    return md;
}


MixDevice *Mixer::mixDeviceByType( int deviceidx )
{
  unsigned int i=0;
  while (i<size() && (*this)[i]->num()!=deviceidx) i++;
  if (i==size()) return 0;

  return (*this)[i];
}

// @dcop
// Used also by the setMasterVolume() method.
void Mixer::setVolume( int deviceidx, int percentage )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return;

  Volume vol=mixdev->getVolume();
  // @todo The next call doesn't handle negative volumes correctly.
  vol.setAllVolumes( (percentage*vol.maxVolume())/100 );
  _mixerBackend->writeVolumeToHW(deviceidx, vol);
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
  _mixerBackend->writeVolumeToHW(md->num(), md->getVolume() );
  _mixerBackend->setEnumIdHW(md->num(), md->enumId() );
}

// @dcop only
void Mixer::setMasterVolume( int percentage )
{
  MixDevice *master = masterDevice();
  if (master != 0 ) {
    setVolume( master->num(), percentage );
  }
}

// @dcop
int Mixer::volume( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return 0;

  Volume vol=mixdev->getVolume();
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
void Mixer::setAbsoluteVolume( int deviceidx, long absoluteVolume ) {
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return;

  Volume vol=mixdev->getVolume();
  vol.setAllVolumes( absoluteVolume );
  _mixerBackend->writeVolumeToHW(deviceidx, vol);
}

// @dcop , especially for use in KMilo
long Mixer::absoluteVolume( int deviceidx )
{
   MixDevice *mixdev= mixDeviceByType( deviceidx );
   if (!mixdev) return 0;

   Volume vol=mixdev->getVolume();
   long avgVolume=vol.getAvgVolume((Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT));
   return avgVolume;
}

// @dcop , especially for use in KMilo
long Mixer::absoluteVolumeMax( int deviceidx )
{
   MixDevice *mixdev= mixDeviceByType( deviceidx );
   if (!mixdev) return 0;

   Volume vol=mixdev->getVolume();
   long maxVolume=vol.maxVolume();
   return maxVolume;
}

// @dcop , especially for use in KMilo
long Mixer::absoluteVolumeMin( int deviceidx )
{
   MixDevice *mixdev= mixDeviceByType( deviceidx );
   if (!mixdev) return 0;

   Volume vol=mixdev->getVolume();
   long minVolume=vol.minVolume();
   return minVolume;
}

// @dcop
int Mixer::masterVolume()
{
  int vol = 0;
  MixDevice *master = masterDevice();
  if (master != 0 ) {
    vol = volume( master->num() );
  }
  return vol;
}

// @dcop
void Mixer::increaseVolume( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (mixdev != 0) {
     Volume vol=mixdev->getVolume();
     double fivePercent = (vol.maxVolume()-vol.minVolume()+1) / 20;
     for (unsigned int i=Volume::CHIDMIN; i <= Volume::CHIDMAX; i++) {
        int volToChange = vol.getVolume((Volume::ChannelID)i);
        if ( fivePercent < 1 ) fivePercent = 1;
        volToChange += (int)fivePercent;
        vol.setVolume((Volume::ChannelID)i, volToChange);
     }
     _mixerBackend->writeVolumeToHW(deviceidx, vol);
  }

  /* see comment at the end of decreaseVolume()
  int vol=volume(deviceidx);
  setVolume(deviceidx, vol+5);
  */
}

// @dcop
void Mixer::decreaseVolume( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (mixdev != 0) {
     Volume vol=mixdev->getVolume();
     double fivePercent = (vol.maxVolume()-vol.minVolume()+1) / 20;
     for (unsigned int i=Volume::CHIDMIN; i <= Volume::CHIDMAX; i++) {
        int volToChange = vol.getVolume((Volume::ChannelID)i);
        //std::cout << "Mixer::decreaseVolume(): before: volToChange " <<i<< "=" <<volToChange << std::endl;
        if ( fivePercent < 1 ) fivePercent = 1;
        volToChange -= (int)fivePercent;
	//std::cout << "Mixer::decreaseVolume():  after: volToChange " <<i<< "=" <<volToChange << std::endl;
        vol.setVolume((Volume::ChannelID)i, volToChange);
        //int volChanged = vol.getVolume((Volume::ChannelID)i);
        //std::cout << "Mixer::decreaseVolume():  check: volChanged " <<i<< "=" <<volChanged << std::endl;
     } // for
     _mixerBackend->writeVolumeToHW(deviceidx, vol);
  }

  /************************************************************
    It is important, not to implement this method like this:
  int vol=volume(deviceidx);
  setVolume(deviceidx, vol-5);
     It creates too big rounding errors. If you don't beleive me, then
     do a decreaseVolume() and increaseVolume() with "vol.maxVolume() == 31".
   ***********************************************************/
}

// @dcop
void Mixer::setMute( int deviceidx, bool on )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return;

  mixdev->setMuted( on );

  _mixerBackend->writeVolumeToHW(deviceidx, mixdev->getVolume() );
}

// @dcop only
void Mixer::setMasterMute( bool on )
{
  MixDevice *master = masterDevice();
  if (master != 0 ) {
    setMute( master->num(), on );
  }
}


// @dcop
void Mixer::toggleMute( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return;

  bool previousState= mixdev->isMuted();

  mixdev->setMuted( !previousState );

  _mixerBackend->writeVolumeToHW(deviceidx, mixdev->getVolume() );
}

// @dcop only
void Mixer::toggleMasterMute()
{
  MixDevice *master = masterDevice();
  if (master != 0 ) {
    toggleMute( master->num() );
  }
}


// @dcop
bool Mixer::mute( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return true;

  return mixdev->isMuted();
}

// @dcop only
bool Mixer::masterMute()
{
  MixDevice *master = masterDevice();
  if (master != 0 ) {
    return mute( master->num() );
  }
  return true;
}

// @dcop only
int Mixer::masterDeviceIndex()
{
  return masterDevice()->num();
}

bool Mixer::isRecordSource( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return false;

  return mixdev->isRecSource();
}

/// @DCOP    WHAT DOES THIS METHOD?!?!?
bool Mixer::isAvailableDevice( int deviceidx )
{
  return mixDeviceByType( deviceidx );
}

#include "mixer.moc"
