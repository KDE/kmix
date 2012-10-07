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
#include "apps/kmix.h"
#include <core/ControlManager.h>
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
   ControlManager::instance().shutdownNow();
   delete m_kmix;
}


int
KMixApp::newInstance()
{
        // There are 3 cases for a new instance

	//kDebug(67100) <<  "KMixApp::newInstance() isRestored()=" << isRestored() << "_keepVisibility=" << _keepVisibility;
	static bool first = true;
	if ( !first )
	{	// There already exists an instance/window
 
                /* !!! @bug : _keepVisibilty has the wrong value here.
                    It is supposed to have the value set by the command line
                    arg, and the keepVisibilty() method.
                    All looks fine, BUT(!!!) THIS code is NEVER entered in
                    the just started process.
                    KDE IPC (DBUS) has instead notified the already running
                    KMix process, about a newInstance(). So _keepVisibilty
                    has always the value of the first started KMix process.
                    This is a bug in KMix and  must be fixed.
                    cesken, 2008-11-01
                 */
                 
		kDebug(67100) <<  "KMixApp::newInstance() Instance exists";

		if ( ! _keepVisibility && !isSessionRestored() ) {
			kDebug(67100) <<  "KMixApp::newInstance() SHOW WINDOW (_keepVisibility=" << _keepVisibility << ", isSessionRestored=" << isSessionRestored();
			// CASE 1: If KMix is running AND the *USER*
                        // starts it again, the KMix main window will be shown.
			// If KMix is restored by SM or the --keepvisibilty is used, KMix will NOT
			// explicitly be shown.
			KUniqueApplication::newInstance();
//			if ( !m_kmix ) {
//				m_kmix->show();
//			} else {
//				kWarning(67100) << "KMixApp::newInstance() Window has not finished constructing yet so ignoring the show() request.";
//			}
		}
		else {
                        // CASE 2: If KMix is running, AND  ( session gets restored OR keepvisibilty command line switch )
			kDebug(67100) <<  "KMixApp::newInstance() REGULAR_START _keepVisibility=" << _keepVisibility;
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
                // CASE 3: KMix was not running yet => instanciate a new one
		//kDebug(67100) <<  "KMixApp::newInstance() Instanciate: _keepVisibility=" << _keepVisibility ;
		first = false;	// NB See https://qa.mandriva.com/show_bug.cgi?id=56893#c3
				// It is important to track this via a separate variable and not
				// based on m_kmix to handle this race condition.
				// Specific protection for the activation-prior-to-full-construction
				// case exists above in the 'already running case'
		m_kmix = new KMixWindow(_keepVisibility);
		//connect(this, SIGNAL(stopUpdatesOnVisibility()), m_kmix, SLOT(stopVisibilityUpdates()));
		if ( isSessionRestored() && KMainWindow::canBeRestored(0) )
		{
			m_kmix->restore(0, false);
		}
	}

	return 0;
}

void KMixApp::keepVisibility(bool val_keepVisibility) {
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
