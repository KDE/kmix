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

#include <kcmdlineargs.h>
#include <kdebug.h>

#include "apps/kmix.h"
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"


KMixApp::KMixApp() :
		KUniqueApplication(), m_kmix(0)
{
	GlobalConfig::init();

	// We must disable QuitOnLastWindowClosed. Rationale:
	// 1) The normal state of KMix is to only have the dock icon shown.
	// 2a) The dock icon gets reconstructed, whenever a soundcard is hotplugged or unplugged.
	// 2b) The dock icon gets reconstructed, when the user selects a new master.
	// 3) During the reconstruction, it can easily happen that no window is present => KMix would quit
	// => disable QuitOnLastWindowClosed
	setQuitOnLastWindowClosed(false);
}

KMixApp::~KMixApp()
{
	ControlManager::instance().shutdownNow();
	delete m_kmix;
}

bool KMixApp::restoreSessionIfApplicable()
{
	bool restore = isSessionRestored() && KMainWindow::canBeRestored(0);
	if (restore)
	{
		m_kmix->restore(0, false);
	}

	return restore;
}

int KMixApp::newInstance()
{
	// There are 3 cases for a new instance

	//kDebug(67100) <<  "KMixApp::newInstance() isRestored()=" << isRestored() << "_keepVisibility=" << _keepVisibility;
	static bool first = true;
	if (!first)
	{
		// There already exists an instance/window
		kDebug(67100)
		<< "KMixApp::newInstance() Instance exists";

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		bool hasArgKeepvisibility = args->isSet("keepvisibility");
		bool reset = args->isSet("failsafe");
		if (reset)
		{
			kWarning() << "Reset cannot be performed while KMix is running. Please quit KMix and retry then.";
		}

		if (!hasArgKeepvisibility)
		{
			// *** CASE 1 ******************************************************
			/*
			 * KMix is running, AND the *USER* starts it again (w/o --keepvisibilty), the KMix main window will be shown.
			 */
			kDebug(67100)
			<< "KMixApp::newInstance() SHOW WINDOW (_keepVisibility="
					<< hasArgKeepvisibility << ", isSessionRestored="
					<< isSessionRestored();

			/*
			 * Restore Session. This may look strange to you, as the instance already exists. But the following
			 * sequence might happen:
			 * 1) Autostart (no restore) => create m_kmix instance (via CASE 3)
			 * 2) Session restore => we are here at this line of code (CASE 1). m_kmix exists, but still must be restored
			 *
			 */
			bool wasRestored = restoreSessionIfApplicable();

			// Use standard newInstances(), which shows and activates the main window. But skip it for the
			// special "restored" case, as we should not override the session rules.
			if (!wasRestored)
			{
				KUniqueApplication::newInstance();
			}
		}
		else
		{
			// *** CASE 2 ******************************************************
			/*
			 * If KMix is running, AND launched again with --keepvisibilty
			 *
			 * =>  We don't want to change the visibiliy, thus we don't call show() here.
			 *
			 * Hint: --keepVisibility is a special (legacy) option for applications that want to start
			 *       a mixer service, but don't need to show the KMix GUI (like KMilo , KAlarm, ...).
			 *       See Bug 58901.
			 *
			 *       Nowadays this switch can be considered legacy, as applications should use KMixD instead.
			 */
			kDebug(67100)
			<< "KMixApp::newInstance() REGULAR_START _keepVisibility="
					<< hasArgKeepvisibility;
		}
	}
	else
	{
		// *** CASE 3 ******************************************************
		/*
		 * Regular case: KMix was not running yet => create a new KMixWindow
		 */
		first = false;// NB See https://qa.mandriva.com/show_bug.cgi?id=56893#c3
		// It is important to track this via a separate variable and not
		// based on m_kmix to handle this race condition.
		// Specific protection for the activation-prior-to-full-construction
		// case exists above in the 'already running case'
		GlobalConfig::init();

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		bool hasArgKeepvisibility = args->isSet("keepvisibility");
		bool reset = args->isSet("failsafe");

		m_kmix = new KMixWindow(hasArgKeepvisibility, reset);

		restoreSessionIfApplicable();
	}

	return 0;
}


#include "KMixApp.moc"
