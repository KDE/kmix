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
#include "kmix-platforms.cpp"


/**
 * Some general design hints. Hierachy is Mixer->MixDevice->Volume
 */


int Mixer::getDriverNum()
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

Mixer* Mixer::getMixer( int driver, int device )
{
    Mixer *mixer = 0;
    getMixerFunc *f = g_mixerFactories[driver].getMixer;
    if( f!=0 )
        mixer = f( device, 0 );
    if ( mixer != 0 )
        mixer->setupMixer(mixer->m_mixDevices); 
    return mixer;
}

/*
// !! Is anybody using the "Mixer::getMixer( int driver, MixSet set,int device, int card )" interface ?
Mixer* Mixer::getMixer( int driver, MixSet set,int device )
{
    Mixer *mixer = 0;
    getMixerSetFunc *f = g_mixerFactories[driver].getMixerSet;
    if( f!=0 )
       mixer = f( set, device, 0 );
    if ( mixer != 0 )
       mixer->setupMixer(m_mixDevices);
    return mixer;
}
*/

Mixer::Mixer( int device, int card ) : DCOPObject( "Mixer" )
{
  m_devnum = device;
  m_cardnum = card;
  m_masterDevice = 0;

  m_isOpen = false;
  //_stateMessage = "OK";
  _errno  = 0;

  m_balance = 0;
  m_mixDevices.setAutoDelete( true );
  m_profiles.setAutoDelete( true );
  m_mixerNum = 0;

  _pollingTimer = new QTimer(); // will be started on grab() and stopped on release()
  connect( _pollingTimer, SIGNAL(timeout()), this, SLOT(readSetFromHW()));

  QCString objid;
  objid.setNum(m_devnum);
  objid.prepend("Mixer");
  DCOPObject::setObjId( objid );
}

int Mixer::setupMixer( MixSet mset )
{
    //   kdDebug(67100) << "Mixer::setupMixer()" << endl;
   release();	// To be sure, release mixer before (re-)opening

   int ret = openMixer();
   if (ret != 0) {
      _errno = ret;
      return ret;
   } else
   {
      // This case is a workaround for old Mixer_*.cpp backends. They return 0 on openMixer() but
      // might not have devices in them. So we work around them here. It would be better if they
      // would return ERR_NODEV themselves.
      if( m_mixDevices.isEmpty() )
      {
         _errno = ERR_NODEV;
	 return ERR_NODEV;
      }
   }


   if( !mset.isEmpty() ) {
       // Copy the initial mix set
       //       kdDebug(67100) << "Mixer::setupMixer() copy Set" << endl;
       MixDevice* md;
       for( md = mset.first(); md != 0; md = mset.next() )
       {
	   MixDevice* mdCopy = m_mixDevices.first();
	   while( mdCopy!=0 && mdCopy->num() != md->num() ) {
	       mdCopy = m_mixDevices.next();
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

   return 0;
}

void Mixer::volumeSave( KConfig *config )
{
    //    kdDebug(67100) << "Mixer::volumeSave()" << endl;
    readSetFromHW();
    QString grp = QString("Mixer") + mixerName();
    m_mixDevices.write( config, grp );
}

void Mixer::volumeLoad( KConfig *config )
{
   QString grp = QString("Mixer") + mixerName();
   if ( ! config->hasGroup(grp) ) {
      // no such group. Volumes (of this mixer) were never saved beforehand.
      // Thus don't restore anything (also see Bug #69320 for understanding the real reason)
      return; // make sure to bail out immediately
   }

   // else restore the volumes
   m_mixDevices.read( config, grp );

   // set new settings
   QPtrListIterator<MixDevice> it( m_mixDevices );
   for(MixDevice *md=it.toFirst(); md!=0; md=++it )
   {
       //       kdDebug(67100) << "Mixer::volumeLoad() writeVolumeToHW(" << md->num() << ", "<< md->getVolume() << ")" << endl;
       // !! @todo Restore record source
       setRecordSource( md->num(), md->isRecSource() );
       writeVolumeToHW( md->num(), md->getVolume() );
   }
}


/**
 * Opens the mixer. If it is already open, this call is ignored
 * Also, starts the polling timer, for polling the Volumes from the Mixer.
 *
 * @return 0 (always)
 */
int Mixer::grab()
{
  if ( !m_isOpen )
    {
      // Try to open Mixer, if it is not open already.
      if ( size() == 0 ) {
          // there is no point in opening a mixer with no devices in it
          return ERR_NODEV;
      }
      int err =  openMixer();
      if( err == ERR_INCOMPATIBLESET )
        {
          // Clear the mixdevices list
          m_mixDevices.clear();
          // try again with fresh set
          err =  openMixer();
        }
      if( !err && m_mixDevices.isEmpty() ) {
        return ERR_NODEV;
      }
      return err;
    }
  _pollingTimer->start(50); // !!
  return 0;
}


/**
 * Closes the mixer. If it is already closed, this call is ignored
 * Also, stops the polling timer.
 *
 * @return 0 (always)
 */
int Mixer::release()
{
  _pollingTimer->stop();
  if( m_isOpen ) {
    m_isOpen = false;
    // Call the target system dependent "release device" function
    return releaseMixer();
  }

  return 0;
}

unsigned int Mixer::size() const
{
  return m_mixDevices.count();
}

MixDevice* Mixer::operator[](int num)
{
  MixDevice* md =  m_mixDevices.at( num );
  Q_ASSERT( md );
  return md;
}


/**
   You can call this to retrieve the freshest information from the mixer HW.
   This method is also called regulary by the mixer timer.
*/
void Mixer::readSetFromHW()
{
  bool updated = prepareUpdate();
  if ( ! updated ) {
    // Some drivers (ALSA) are smart. We don't need to run the following
    // time-consuming update loop if there was no change
    return;
  }
  MixDevice* md;
  for( md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() )
    {
      Volume& vol = md->getVolume();
      readVolumeFromHW( md->num(), vol );
      md->setRecSource( isRecsrcHW( md->num() ) );
    }
  // Trivial implementation. Without looking at the devices
  //  kdDebug(67100) << "Mixer::readSetFromHW(): emit newVolumeLevels()" << endl;
  emit newVolumeLevels();
  emit newRecsrc(); // cheap, but works
}

bool Mixer::prepareUpdate() {
  return true;
}
void Mixer::setBalance(int balance)
{
  // !! BAD, because balance only works on the master device. If you have not Master, the slider is a NOP
  if( balance == m_balance ) {
      // balance unchanged => return
      return;
  }

  m_balance = balance;

  MixDevice* master = m_mixDevices.at( m_masterDevice );
  if ( master == 0 ) {
      // no master device available => return
      return;
  }

  Volume& vol = master->getVolume();
  readVolumeFromHW( m_masterDevice, vol );

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

  writeVolumeToHW( m_masterDevice, vol );

  emit newBalance( vol );
}

QString Mixer::mixerName()
{
  return m_mixerName;
}

QString Mixer::driverName( int driver )
{
    getDriverNameFunc *f = g_mixerFactories[driver].getDriverName;
    if( f!=0 )
        return f();
    else
        return "unknown";
}


void Mixer::setMixerNum( int num )
{
    m_mixerNum = num;
}

int Mixer::mixerNum()
{
    return m_mixerNum;
}

int Mixer::getErrno() const {
    return this->_errno;
}

void Mixer::errormsg(int mixer_error)
{
  QString l_s_errText;
  l_s_errText = errorText(mixer_error);
  kdError() << l_s_errText << "\n";
}

QString Mixer::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n("kmix:You do not have permission to access the mixer device.\n" \
			"Please check your operating systems manual to allow the access.");
      break;
    case ERR_WRITE:
      l_s_errmsg = i18n("kmix: Could not write to mixer.");
      break;
    case ERR_READ:
      l_s_errmsg = i18n("kmix: Could not read from mixer.");
      break;
    case ERR_NODEV:
      l_s_errmsg = i18n("kmix: Your mixer does not control any devices.");
      break;
    case  ERR_NOTSUPP:
      l_s_errmsg = i18n("kmix: Mixer does not support your platform. See mixer.cpp for porting hints (PORTING).");
      break;
    case  ERR_NOMEM:
      l_s_errmsg = i18n("kmix: Not enough memory.");
      break;
    case ERR_OPEN:
    case ERR_MIXEROPEN:
      // ERR_MIXEROPEN means: Soundcard could be opened, but has no mixer. ERR_MIXEROPEN is normally never
      //      passed to the errorText() method, because KMix handles that case explicitely
      l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
			"Please check that the soundcard is installed and that\n" \
			"the soundcard driver is loaded.\n");
      break;
    case ERR_INCOMPATIBLESET:
      l_s_errmsg = i18n("kmix: Initial set is incompatible.\n" \
			"Using a default set.\n");
      break;
    default:
      l_s_errmsg = i18n("kmix: Unknown error. Please report how you produced this error.");
      break;
    }
  return l_s_errmsg;
}

/*
QString& Mixer::stateMessage() const {
    const QString &s = _stateMessage;
    return s;
}
*/

/**
   Used internally by the Mixer class and as DCOP method
*/
void Mixer::setRecordSource( int devnum, bool on )
{
  if( !setRecsrcHW( devnum, on ) ) // others have to be updated
  {
	for( MixDevice* md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() ) {
		bool isRecsrc =  isRecsrcHW( md->num() );
//		kdDebug(67100) << "Mixer::setRecordSource(): isRecsrcHW(" <<  md->num() << ") =" <<  isRecsrc << endl;
		md->setRecSource( isRecsrc );
	}
	// emitting is done after read
	//emit newRecsrc(); // like "emit newVolumeLevels()", but for record source
  }
  else {
	// just the actual mixdevice
	for( MixDevice* md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() ) {
		  if( md->num() == devnum ) {
			bool isRecsrc =  isRecsrcHW( md->num() );
			md->setRecSource( isRecsrc );
		  }
	}
	// emitting is done after read
	//emit newRecsrc(); // like "emit newVolumeLevels()", but for record source
  }

  if ( hasBrokenRecSourceHandling() ) {
      //      kdDebug(67100) << "Applying RecordSource workaround" << endl;
      // If we are too stupid to read from the Hardware (as with the current Mixer_ALSA class),
      // we simply set all other devices to "off".
      for (unsigned int i=0; i<size(); i++) {
	  if (i != (unsigned int)devnum) {
	      //      kdDebug(67100) << "Applying RecordSource workaround" << i << endl;
	      setRecsrcHW( i, false );
	  }
      }
  }
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
  vol.setAllVolumes( (percentage*vol.maxVolume())/100 );
  writeVolumeToHW(deviceidx, vol);
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
    writeVolumeToHW(md->num(), md->getVolume() );
}

// @dcop only
void Mixer::setMasterVolume( int percentage )
{
  setVolume( 0, percentage );
}

// @dcop
int Mixer::volume( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return 0;

  Volume vol=mixdev->getVolume();
  return (vol.getVolume( Volume::LEFT )*100)/vol.maxVolume();
}

// @dcop
int Mixer::masterVolume()
{
  return volume( 0 );
}

// @dcop
void Mixer::increaseVolume( int deviceidx )
{
  int vol=volume(deviceidx);
  setVolume(deviceidx, vol+5);
}

// @dcop
void Mixer::decreaseVolume( int deviceidx )
{
  int vol=volume(deviceidx);
  setVolume(deviceidx, vol-5);
}

// @dcop
void Mixer::setMute( int deviceidx, bool on )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return;

  mixdev->setMuted( on );

  writeVolumeToHW(deviceidx, mixdev->getVolume() );
}

// @dcop
bool Mixer::mute( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return true;

  return mixdev->isMuted();
}

bool Mixer::isRecordSource( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return false;

  return mixdev->isRecSource();
}

bool Mixer::isAvailableDevice( int deviceidx )
{
  return mixDeviceByType( deviceidx );
}

bool Mixer::hasBrokenRecSourceHandling() {
    // Only for the current Mixer_ALSA implementation.
    // This implementation does not see changes from the Mixer Hardware to the Record Sources.
    // So the workaround is to manually call md.setRecSrc(false) for all aother channels.
    return false;
}

#include "mixer.moc"
