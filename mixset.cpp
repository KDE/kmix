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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include <qstring.h>

#include <kdebug.h>
#include <kconfig.h>

#include "mixdevice.h"
#include "mixset.h"

void MixSet::clone( MixSet &set )
{
    clear();

    for( MixDevice *md=set.first(); md!=0; md=set.next() )
    {
        append( new MixDevice( *md ) );
    }
}

void MixSet::read( KConfig *config, const QString& grp )
{
    kdDebug(67100) << "MixSet::read() of group " << grp << endl;
    config->setGroup(grp);
    m_name = config->readEntry( "name", m_name );

    MixDevice* md;
    for( md=first(); md!=0; md=next() )
    {
        md->read( config, grp );
    }
}

void MixSet::write( KConfig *config, const QString& grp )
{
    kdDebug(67100) << "MixSet::write() of group " << grp << endl;    
    config->setGroup(grp);
    config->writeEntry( "name", m_name );

    MixDevice* md;
    for( md=first(); md!=0; md=next() )
    {
        md->write( config, grp );
    }
}

void MixSet::setName( const QString &name )
{
    m_name = name;
}

