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
#include "core/mixdevice.h"

// KDE
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>

// Qt
#include <QString>


MixSet::~MixSet()
{
	clear();
}

bool MixSet::read( KConfig *config, const QString& grp )
{
   kDebug(67100) << "MixSet::read() of group " << grp;
   KConfigGroup group = config->group(grp);
   m_name = group.readEntry( "name", m_name );

   bool have_success = false, have_fail = false;
   foreach ( shared_ptr<MixDevice> md, *this)
   {
       if ( md->read( config, grp ) )
           have_success = true;
       else
           have_fail = true;
   }
   return have_success && !have_fail;
}

bool MixSet::write( KConfig *config, const QString& grp )
{
   kDebug(67100) << "MixSet::write() of group " << grp;    
   KConfigGroup conf = config->group(grp);
   conf.writeEntry( "name", m_name );

   bool have_success = false, have_fail = false;
   foreach ( shared_ptr<MixDevice> md, *this)
   {
       if ( md->write( config, grp ) )
           have_success = true;
       else
           have_fail = true;
   }
   return have_success && !have_fail;
}

void MixSet::setName( const QString &name )
{
    m_name = name;
}

shared_ptr<MixDevice> MixSet::get(QString id)
{
	shared_ptr<MixDevice> mdRet;

	foreach ( shared_ptr<MixDevice> md, *this)
	{
		if ( md->id() == id )
		{
			mdRet = md;
			break;
		}
	}
	return mdRet;
}

void MixSet::removeById(QString id)
{
	for (int i=0; i < count() ; i++ )
	{
		shared_ptr<MixDevice> md = operator[](i);
	    if ( md->id() == id )
	    {
	    	removeAt(i);
	    	break;
	    }
	}
}

