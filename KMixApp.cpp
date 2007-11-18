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
#include "kmix.h"
#include <kdebug.h>


bool KMixApp::_keepVisibility = false;

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
	//kDebug(67100) <<  "KMixApp::newInstance() isRestored()=" << isRestored() << "_keepVisibility=" << _keepVisibility;
	if ( m_kmix )
	{	// There already exists an instance/window
		kDebug(67100) <<  "KMixApp::newInstance() Instance exists";
#ifdef __GNUC__
#warning Have to find another way for KUniqueApplication::isRestored()
#endif
		if ( ! _keepVisibility /*&& ! isRestored()*/ ) {
			//kDebug(67100) <<  "KMixApp::newInstance() _keepVisibility=false";
			// Default case: If KMix is running and the *USER*
                        // starts it again, the KMix main window will be shown.
			// If KMix is restored by SM or the --keepvisibilty is used, KMix will NOT
			// explicitly be shown.
			m_kmix->show();
		}
		else {
			//kDebug(67100) <<  "KMixApp::newInstance() _keepVisibility=true || isRestored()=true";
			// Special case: Command line arg --keepVisibility was used:
			// We don't want to change the visibiliy, thus we don't call show() here.
			//
			//  Hint: --keepVisibility is a special option for applications that
			//    want to start a mixer service, but don't need to show the KMix
			//    GUI (like KMilo , KAlarm, ...).
			//    See (e.g.) Bug 58901 for deeper insight.
		}
	}
	else
	{
		//kDebug(67100) <<  "KMixApp::newInstance() !m_kmix";
		m_kmix = new KMixWindow;
		//connect(this, SIGNAL(stopUpdatesOnVisibility()), m_kmix, SLOT(stopVisibilityUpdates()));
		if ( isSessionRestored() && KMainWindow::canBeRestored(0) )
		{
			m_kmix->restore(0, false);
		}
	}

	return 0;
}


void KMixApp::keepVisibility(bool val_keepVisibility) {
   //kDebug(67100) <<  "KMixApp::keepVisibility()";
   _keepVisibility = val_keepVisibility;
}

/*
void
KMixApp::quitExtended()
{
    // This method is here to quit hold from the dock icon: When directly calling
    // quit(), the main window will be hidden before saving the configuration.
    // isVisible() would return on quit always false (which would be bad).
    kDebug(67100) <<  "quitExtended ENTER";
    emit stopUpdatesOnVisibility();
    quit();
}
*/

#include "KMixApp.moc"
