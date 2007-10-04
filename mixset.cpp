/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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


//KMix
#include "mixset.h"
#include "mixdevice.h"

// KDE
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>

// Qt
#include <QString>



void MixSet::read( KConfig *config, const QString& grp )
{
   kDebug(67100) << "MixSet::read() of group " << grp;
   KConfigGroup group = config->group(grp);
   m_name = group.readEntry( "name", m_name );

   for(int i=0; i < count() ; i++ )
   {
       MixDevice *md = operator[](i);
       md->read( config, grp );
   }
}

void MixSet::write( KConfig *config, const QString& grp )
{
   kDebug(67100) << "MixSet::write() of group " << grp;    
   KConfigGroup conf = config->group(grp);
   conf.writeEntry( "name", m_name );

   for(int i=0; i < count() ; i++ )
   {
       MixDevice *md = operator[](i);
       md->write( config, grp );
   }
}

void MixSet::setName( const QString &name )
{
    m_name = name;
}

