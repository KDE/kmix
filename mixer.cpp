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

Mixer::Mixer( int device, int card )
   : m_mixDevices( this )
{
  m_devnum = device;
  m_cardnum = card;
  m_masterDevice = 0;

  m_isOpen = false;

  m_balance = 0;
  m_mixDevices.setAutoDelete( true );

  m_mixSets.setAutoDelete( true );
};

int Mixer::setupMixer( MixSet mset )
{
   kDebugInfo("Mixer::setupMixer");
   m_mixSets.clear();
   sessionLoad( false );
   
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

MixSet* Mixer::getSet( int num )
{
   return m_mixSets.at( num );
}

int Mixer::createSet()
{
   kDebugInfo("Mixer::createSet - this=%x", this);
   MixSet *ms = new MixSet( m_mixDevices );
   m_mixSets.append( ms );
   kDebugInfo("m_mixSets size=%i", m_mixSets.count());
   return m_mixSets.find( ms );
}

void Mixer::destroySet( int num )
{
   kDebugInfo("Mixer::destroySet( num=%i )", num);
   m_mixSets.remove( m_mixSets.at( num ) );
}

void Mixer::destroySet( MixSet *set )
{
   kDebugInfo("Mixer::destroySet( set=%x )", set);
   m_mixSets.remove( set );
}

void Mixer::sessionSave( bool /*sessionConfig*/ )
{
   kDebugInfo("Mixer::sessionSave - this=%x m_mixSets.count=%i", this, m_mixSets.count());
   QString grp = QString("Mixer") + mixerName();
   KConfig* config = KGlobal::config();
   config->setGroup(grp);
   config->writeEntry( "sets", m_mixSets.count() );

   MixSet* ms;
   int set = 0;
   for( ms=m_mixSets.first(); ms!=0; ms=m_mixSets.next() )
   {
      kDebugInfo("saving mixset %i", set);
      QString setgrp;
      setgrp.sprintf( "%s.Set%i", grp.ascii(), set );
      ms->write( setgrp );
      set++;
   }
}

void Mixer::sessionLoad( bool /*sessionConfig*/ )
{
   QString grp = QString("Mixer") + mixerName();
   KConfig* config = KGlobal::config();
   config->setGroup(grp);
   int num = config->readNumEntry( "sets", 0 );

   m_mixSets.clear();

   int set;
   for ( set=0; set<num; set++ )
   {
      MixSet* ms = getSet( createSet() );
      
      QString setgrp;
      setgrp.sprintf( "%s.Set%i", grp.ascii(), set );
      ms->read( setgrp );
      set++;
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
