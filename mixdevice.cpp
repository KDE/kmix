/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *		 1996-2000 Christian Esken <esken@kde.org>
 *        		   Sven Fischer <herpes@kawo2.rwth-aachen.de>
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

#include "mixdevice.h"

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <iostream.h>


MixDevice::MixDevice(int num, Volume vol, bool recordable,
                     QString name, ChannelType type ) :
   m_volume( vol ), m_type( type ), m_num( num ), m_recordable( recordable )
{
   m_stereoLink = true;

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

   setVolume( Volume::LEFT, config->readNumEntry("volumeL", 50) );
   setVolume( Volume::RIGHT, config->readNumEntry("volumeR", 50) );

   setDisabled( config->readNumEntry("is_disabled", 0) );
   setMuted( config->readNumEntry("is_muted", 0) );
   setStereoLinked( config->readNumEntry("StereoLink", 1) );
   m_name = config->readEntry("name", "unnamed");
}

void MixDevice::write( const QString& grp )
{
   QString devgrp;
   devgrp.sprintf( "%s.Dev%i", grp.ascii(), m_num );
   KConfig* config = KGlobal::config();
   config->setGroup(devgrp);

   config->writeEntry("volumeL", getVolume( Volume::LEFT ) );
   config->writeEntry("volumeR", getVolume( Volume::RIGHT ) );
   config->writeEntry("is_disabled", isDisabled() );
   config->writeEntry("is_muted", isMuted() );
   config->writeEntry("StereoLink", isStereoLinked() );
   config->writeEntry("name", m_name);
}


/***************** MixSet *****************/


MixSet::MixSet( Mixer *mixer, const QString &setname )
   : m_name(setname), m_mixer(mixer)
{
}

MixSet::MixSet( const MixSet &list )
   : QList<MixDevice>( list )
{
   m_name = list.m_name;
   m_mixer = list.m_mixer;
}

void MixSet::read( const QString& grp )
{
   KConfig* config = KGlobal::config();
   config->setGroup(grp);
   
   m_name = config->readEntry("name", "unnamed");

   MixDevice* md;
   for( md=first(); md!=0; md=next() )
   {
      kDebugInfo("-> reading dev=%x", md);
      md->read( grp );
      kDebugInfo("<- reading dev=%x", md);
   }
}

void MixSet::write( const QString& grp )
{
   KConfig* config = KGlobal::config();
   config->setGroup(grp);

   config->writeEntry("name", m_name);

   MixDevice* md;
   for( md=first(); md!=0; md=next() )
   {
      kDebugInfo("-> saving dev=%x", md);
      md->write( grp );
      kDebugInfo("<- saving dev=%x", md);
   }
}

