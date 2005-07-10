/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2003 Sven Leiber <s.leiber@web.de>
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

#include "KMixApp.h"
#include "kmix.h"
#include <kdebug.h>


KMixApp::KMixApp()
    : KUniqueApplication(), m_kmix( 0 )
{
}


KMixApp::~KMixApp()
{
   delete m_kmix;
}


int
KMixApp::newInstance()
{
	if ( m_kmix )
	{
		m_kmix->show();
	}
	else
	{
		m_kmix = new KMixWindow;
		connect(this, SIGNAL(stopUpdatesOnVisibility()), m_kmix, SLOT(stopVisibilityUpdates()));
		if ( isRestored() && KMainWindow::canBeRestored(0) )
		{
			m_kmix->restore(0, FALSE);
		}
	}

	return 0;
}


void
KMixApp::quitExtended()
{
    // This method is here for quiting from the dock icon: When directly calling
    // quit(), the main window will be hidden before saving the configuration.
    // isVisible() would return on quit always false (which would be bad).
    emit stopUpdatesOnVisibility();
    quit();
}

#include "KMixApp.moc"
