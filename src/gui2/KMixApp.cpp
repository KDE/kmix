/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
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

#include "KMixApp.h"
#include "KMixWindow.h"
#include <kdebug.h>


KMixApp::KMixApp()
    : KUniqueApplication(), m_kmix( 0 )
{
    // We must disable QuitOnLastWindowClosed. Rationale:
    // 1) The normal state of KMix is to only have the dock icon shown.
    // 2a) The dock icon gets reconstructed, whenever a soundcard is hotplugged or unplugged.
    // 2b) The dock icon gets reconstructed, when the user selects a new master.
    // 3) During the reconstruction, it can easily happen that no window is present => KMix would quit
    // => disable QuitOnLastWindowClosed
    setQuitOnLastWindowClosed ( false );
}


KMixApp::~KMixApp()
{
   delete m_kmix;
}


int
KMixApp::newInstance()
{
    if ( m_kmix ) {
        m_kmix->show();
    } else {
        m_kmix = new KMixWindow(NULL);
    }
	return 0;
}

