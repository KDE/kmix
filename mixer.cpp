/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2000 Christian Esken - esken@kde.org
 * 2002 Helio Chissini de Castro - helio@conectiva.com.br
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

/**
 * Constructs a MixDevice. A MixDevice represents one channel or control of
 * the mixer hardware. A MixDevice has a type (e.g. PCM), a descriptive name
 * (for example "Master" or "Headphone" or "IEC 958 Output"),
 * can have a volume level (2 when stereo), can be recordable and muted.
 * The category tells which kind of control the MixDevice is.
 * 
 * Hints: Meaning of "category" has changed. In future the MixDevice might contain two
 * Volume objects, one for Output (Playback volume) and one for Input (Record volume).
 */
MixDevice::MixDevice(	int num, Volume vol, bool recordable, bool mute,
		QString name, ChannelType type, DeviceCategory category ) :
	m_volume( vol ), m_type( type ), m_num( num ), m_recordable( recordable ),
	m_mute( mute ), m_category( category )
{
	m_switch = false;
	m_recSource = false;
   if( name.isEmpty() )
      m_name = i18n("unknown");
   else
      m_name = name;
	
	if( category == MixDevice::SWITCH )
		m_switch = true;
   
}

MixDevice::MixDevice(const MixDevice &md)
{
   m_name = md.m_name;
   m_volume = md.m_volume;
   m_type = md.m_type;
   m_num = md.m_num;
   m_recordable = md.m_recordable;
   m_recSource  = md.m_recSource;
   m_category = md.m_category;
	m_switch = md.m_switch;
	m_mute = md.m_mute;
}

int MixDevice::getVolume( int channel ) const
{
   return m_volume[ channel ];
}

int MixDevice::rightVolume() const
{
   return m_volume.getVolume( Volume::RIGHT );
}

int MixDevice::leftVolume() const
{
   return m_volume.getVolume( Volume::LEFT );
}

void MixDevice::setVolume( int channel, int vol )
{
   m_volume.setVolume( channel, vol );
}

void MixDevice::read( KConfig *config, const QString& grp )
{
   kdDebug() << "+ MixDevice::read" << endl;
   
   QString devgrp;
   devgrp.sprintf( "%s.Dev%i", grp.ascii(), m_num );
   config->setGroup( devgrp );

   int vl = config->readNumEntry("volumeL", -1);
   if (vl!=-1) setVolume( Volume::LEFT, vl );

   int vr = config->readNumEntry("volumeR", -1);
   if (vr!=-1) setVolume( Volume::RIGHT, vr );

   int mute = config->readNumEntry("is_muted", -1);
   if ( mute!=-1 ) setMuted( mute!=0 );

   int recsrc = config->readNumEntry("is_recsrc", -1);
	if ( recsrc!=-1 ) setRecSource( recsrc!=0 );
   
   kdDebug() << "- MixDevice::read" << endl;
}

void MixDevice::write( KConfig *config, const QString& grp )
{
   QString devgrp;
   devgrp.sprintf( "%s.Dev%i", grp.ascii(), m_num );
   config->setGroup(devgrp);

   config->writeEntry("volumeL", getVolume( Volume::LEFT ) );
   config->writeEntry("volumeR", getVolume( Volume::RIGHT ) );
   config->writeEntry("is_muted", (int)isMuted() );
   config->writeEntry("is_recsrc", (int)isRecSource() );
   config->writeEntry("name", m_name);
}


/***************** MixSet *****************/

void MixSet::clone( MixSet &set )
{
   clear();

   for( MixDevice *md=set.first(); md!=0; md=set.next() )
      append( new MixDevice( *md ) );
}

void MixSet::read( KConfig *config, const QString& grp )
{
   config->setGroup(grp);
   m_name = config->readEntry( "name", m_name );

   MixDevice* md;
   for( md=first(); md!=0; md=next() )
      md->read( config, grp );
}

void MixSet::write( KConfig *config, const QString& grp )
{
   config->setGroup(grp);
   config->writeEntry( "name", m_name );

   MixDevice* md;
   for( md=first(); md!=0; md=next() )
      md->write( config, grp );
}

/********************** Mixer ***********************/

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


Mixer* Mixer::getMixer( int driver, int device, int card )
{
    getMixerFunc *f = g_mixerFactories[driver].getMixer;
    if( f!=0 )
        return f( device, card );
    else
        return 0;
}


Mixer* Mixer::getMixer( int driver, MixSet set,int device, int card )
{
    getMixerSetFunc *f = g_mixerFactories[driver].getMixerSet;
    if( f!=0 )
        return f( set, device, card );
    else
        return 0;
}


Mixer::Mixer( int device, int card ) : DCOPObject( "Mixer" )
{
  m_devnum = device;
  m_cardnum = card;
  m_masterDevice = 0;

  m_isOpen = false;

  m_balance = 0;
  m_mixDevices.setAutoDelete( true );
  m_profiles.setAutoDelete( true );
  m_mixerNum = 0;

  QCString objid;
  objid.setNum(m_devnum);
  objid.prepend("Mixer");
  DCOPObject::setObjId( objid );
}

int Mixer::setupMixer( MixSet mset )
{
   release();	// To be sure, release mixer before (re-)opening

   int ret = openMixer();
   if (ret != 0) {
      return ret;
   } else
      if( m_mixDevices.isEmpty() )
	 return ERR_NODEV;

   if( !mset.isEmpty() ) // Copies the initial mix set
      writeMixSet( mset );

   return 0;
}

void Mixer::volumeSave( KConfig *config )
{
   readSetFromHW();
   QString grp = QString("Mixer") + mixerName();
   m_mixDevices.write( config, grp );
}

void Mixer::volumeLoad( KConfig *config )
{
   QString grp = QString("Mixer") + mixerName();
   m_mixDevices.read( config, grp );

   // set new settings
   QPtrListIterator<MixDevice> it( m_mixDevices );
   for(MixDevice *md=it.toFirst(); md!=0; md=++it )
   {
		//setRecordSource( md->num(), md->isRecSource() );
      writeVolumeToHW( md->num(), md->getVolume() );
   }
}

int Mixer::grab()
{
  if ( !m_isOpen )
    {
      // Try to open Mixer, if it is not open already.
      int err =  openMixer();
      if( err == ERR_INCOMPATIBLESET )
        {
          // Clear the mixdevices list
          m_mixDevices.clear();
          // try again with fresh set
          err =  openMixer();
        }
      if( !err && m_mixDevices.isEmpty() )
        return ERR_NODEV;
      return err;
    }
  return 0;
}

int Mixer::release()
{
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

void Mixer::readSetFromHW()
{
  MixDevice* md;
  for( md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() )
    {
      Volume vol = md->getVolume();
      readVolumeFromHW( md->num(), vol );
      md->setVolume( vol );
      md->setRecSource( isRecsrcHW( md->num() ) );
    }
}

void Mixer::setBalance(int balance)
{
  if( balance == m_balance ) return;

  m_balance = balance;

  MixDevice* master = m_mixDevices.at( m_masterDevice );

  Volume vol = master->getVolume();
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

void Mixer::writeMixSet( MixSet mset )
{
  MixDevice* md;
  for( md = mset.first(); md != 0; md = mset.next() )
    {
      MixDevice* comp = m_mixDevices.first();
      while( comp && comp->num() != md->num() ) {
		comp = m_mixDevices.next();
      }
      setRecordSource( md->num(), md->isRecSource() );
      comp->setVolume( md->getVolume() );
      comp->setMuted( md->isMuted() );
    }
}

void Mixer::setRecordSource( int devnum, bool on )
{
  if( !setRecsrcHW( devnum, on ) ) // others have to be updated
  {
	for( MixDevice* md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() ) {
		bool isRecsrc =  isRecsrcHW( md->num() );
		kdDebug() << "Mixer::setRecordSource(): isRecsrcHW(" <<  md->num() << ") =" <<  isRecsrc << endl;
		md->setRecSource( isRecsrc );
	}
	emit newRecsrc();
  }
  else {
	// just the actual mixdevice
	for( MixDevice* md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() ) {
		  if( md->num() == devnum ) {
			bool isRecsrc =  isRecsrcHW( md->num() );
			md->setRecSource( isRecsrc );
		  }
	}
	emit newRecsrc();
  }
}


MixDevice *Mixer::mixDeviceByType( int deviceidx )
{
  unsigned int i=0;
  while (i<size() && (*this)[i]->num()!=deviceidx) i++;
  if (i==size()) return 0;

  return (*this)[i];
}

void Mixer::setVolume( int deviceidx, int percentage )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return;

  Volume vol=mixdev->getVolume();
  vol.setAllVolumes( (percentage*vol.maxVolume())/100 );
  writeVolumeToHW(deviceidx, vol);
}

void Mixer::setMasterVolume( int percentage )
{
  setVolume( 0, percentage );
}

int Mixer::volume( int deviceidx )
{
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return 0;

  Volume vol=mixdev->getVolume();
  return (vol.getVolume( Volume::LEFT )*100)/vol.maxVolume();
}

int Mixer::masterVolume()
{
  return volume( 0 );
}

void Mixer::increaseVolume( int deviceidx )
{
  int vol=volume(deviceidx);
  setVolume(deviceidx, vol+5);
}

void Mixer::decreaseVolume( int deviceidx )
{
  int vol=volume(deviceidx);
  setVolume(deviceidx, vol-5);
}

void Mixer::setMute( int deviceidx, bool on )
{
  kdDebug() << "+ Mixer::setMute" << endl;
  
  MixDevice *mixdev= mixDeviceByType( deviceidx );
  if (!mixdev) return;

  mixdev->setMuted( on );

  writeVolumeToHW(deviceidx, mixdev->getVolume() );
  
  kdDebug() << "- Mixer::setMute" << endl;
}

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

#include "mixer.moc"
