/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <iostream.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>

#include "mixer.h"
#include "kmix-platforms.cpp"


MixDevice::MixDevice(int num, Volume vol, bool recordable,
                     QString name, ChannelType type ) :
   m_volume( vol ), m_type( type ), m_num( num ), m_recordable( recordable )
{
   if( name.isEmpty() )
      m_name = i18n("unknown");
   else
      m_name = name;
};

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

void MixDevice::read( const QString& grp )
{
   QString devgrp;
   devgrp.sprintf( "%s.Dev%i", grp.ascii(), m_num );
   KConfig* config = KGlobal::config();
   config->setGroup( devgrp );
   
   int vl = config->readNumEntry("volumeL", -1);
   if (vl!=-1) setVolume( Volume::LEFT, vl );

   int vr = config->readNumEntry("volumeR", -1);
   if (vr!=-1) setVolume( Volume::RIGHT, vr );

   int mute = config->readNumEntry("is_muted", -1);
   if ( mute!=-1 ) setMuted( mute!=0 );
   
   int recsrc = config->readNumEntry("is_recsrc", -1);
   if ( recsrc!=-1 ) setRecsrc( recsrc!=0 );
}

void MixDevice::write( const QString& grp )
{
   QString devgrp;
   devgrp.sprintf( "%s.Dev%i", grp.ascii(), m_num );
   KConfig* config = KGlobal::config();
   config->setGroup(devgrp);

   config->writeEntry("volumeL", getVolume( Volume::LEFT ) );
   config->writeEntry("volumeR", getVolume( Volume::RIGHT ) );
   config->writeEntry("is_muted", (int)isMuted() );
   config->writeEntry("is_recsrc", (int)isRecsrc() );
   config->writeEntry("name", m_name);
}


/***************** MixSet *****************/

void MixSet::read( const QString& grp )
{
   KConfig* config = KGlobal::config();
   config->setGroup(grp);

   MixDevice* md;
   for( md=first(); md!=0; md=next() )
      md->read( grp );
}

void MixSet::write( const QString& grp )
{
   KConfig* config = KGlobal::config();
   config->setGroup(grp);

   MixDevice* md;
   for( md=first(); md!=0; md=next() )
      md->write( grp );      
}

/********************** Mixer ***********************/

Mixer::Mixer( int device, int card )
{
  m_devnum = device;
  m_cardnum = card;
  m_masterDevice = 0;

  m_isOpen = false;

  m_balance = 0;
  m_mixDevices.setAutoDelete( true );
};

int Mixer::setupMixer( MixSet mset )
{
   kDebugInfo("Mixer::setupMixer");
      
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

void Mixer::sessionSave( bool /*sessionConfig*/ )
{
   kDebugInfo("-> Mixer::sessionSave");

   QString grp = QString("Mixer") + mixerName();
   m_mixDevices.write(grp);

   kDebugInfo("<- Mixer::sessionSave");
}

void Mixer::sessionLoad( bool /*sessionConfig*/ )
{
   kDebugInfo("Mixer::sessionLoad");

   QString grp = QString("Mixer") + mixerName();
   m_mixDevices.read(grp);

   QListIterator<MixDevice> it( m_mixDevices );
   for(MixDevice *md=it.toFirst(); md!=0; md=++it )
   {
      setRecsrc( md->num(), md->isRecsrc() );
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
  ASSERT( md );
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
      md->setRecsrc( isRecsrcHW( md->num() ) );
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

void Mixer::errormsg(int mixer_error)
{
  QString l_s_errText;
  l_s_errText = errorText(mixer_error);
  cerr << l_s_errText << "\n";
}

QString Mixer::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n("kmix:You have no permission to access the mixer device.\n" \
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
			"Please check that the soundcard is installed and the\n" \
			"soundcard driver is loaded\n");
      break;
    case ERR_INCOMPATIBLESET:
      l_s_errmsg = i18n("kmix: Initial set is incompatible.\n" \
			"Using a default set.\n");
      break;
    default:
      l_s_errmsg = i18n("kmix: Unknown error. Please report, how you produced this error.");
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
      while( comp && comp->num() != md->num() ) comp = m_mixDevices.next();
      comp->setVolume( md->getVolume() );
    }
}

void Mixer::setRecsrc( int devnum, bool on )
{
  if( !setRecsrcHW( devnum, on ) ) // others have to be updated
    {
      for( MixDevice* md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() )
        md->setRecsrc( isRecsrcHW( md->num()) );
      emit newRecsrc();
    }
  else // just the actual mixdevice
    for( MixDevice* md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() )
        if( md->num() == devnum ) md->setRecsrc( on );
}
