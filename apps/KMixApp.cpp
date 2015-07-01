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

bool KMixApp::firstCaller = true;

KMixApp::KMixApp() :
		KUniqueApplication(), m_kmix(0), creationLock(QMutex::Recursive)
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
	qCDebug(KMIX_LOG) << "Deleting KMixApp";
	ControlManager::instance().shutdownNow();
	delete m_kmix;
	m_kmix = 0;
	GlobalConfig::shutdown();
}

void KMixApp::createWindowOnce(bool hasArgKeepvisibility, bool reset)
{
	// Create window, if it was not yet created (e.g. via autostart or manually)
	if (m_kmix == 0)
	{
		qCDebug(KMIX_LOG) << "Creating new KMix window";
		m_kmix = new KMixWindow(hasArgKeepvisibility, reset);
	}
}

bool KMixApp::restoreSessionIfApplicable(bool hasArgKeepvisibility, bool reset)
{
	/**
	 * We should lock session creation. Rationale:
	 * KMix can be started multiple times during session start. By "autostart" and "session restore". The order is
	 * undetermined, as KMix will initialize in the background of KDE session startup (Hint: As a
	 * KUniqueApplication it decouples from the startkde process!).
	 *
	 * Now we must make sure that window creation is definitely done, before the "other" process comes, as it might
	 * want to restore the session. Working on a half-created window would not be smart! Why can this happen? It
	 * depends on implementation details insinde Qt, which COULD potentially lead to the following scenarios:
	 * 1) "Autostart" and "session restore" run concurrenty in 2 differnent Threads.
	 * 2) The current "main/gui" thread "pops up" a "session restore" message from the Qt event dispatcher.
	 *    This means that  "Autostart" and "session restore" run interleaved in a single Thread.
	 */
	creationLock.lock();

	bool restore = isSessionRestored(); // && KMainWindow::canBeRestored(0);
	qCDebug(KMIX_LOG) << "Starting KMix using kepvisibility=" << hasArgKeepvisibility << ", failsafe=" << reset << ", sessionRestore=" << restore;
	int createCount = 0;
	if (restore)
	{
		if (reset)
		{
			qCWarning(KMIX_LOG) << "Reset cannot be performed while KMix is running. Please quit KMix and retry then.";
		}
		int n = 1;
		while (KMainWindow::canBeRestored(n))
		{
			qCDebug(KMIX_LOG) << "Restoring window " << n;
			if (n > 1)
			{
				// This code path is "impossible". It is here only for analyzing possible issues with session resoring.
				// KMix is a single-instance app. If more than one instance is craeated we have a bug.
				qCWarning(KMIX_LOG) << "KDE session management wants to restore multiple instances of KMix. Please report this as a bug.";
				break;
			}
			else
			{
				// Create window, if it was not yet created (e.g. via autostart or manually)
				createWindowOnce(hasArgKeepvisibility, reset);
				// #restore() is called with the parameter of "show == false", as KMixWindow iteself decides on it.
				m_kmix->restore(n, false);
				createCount++;
				n++;
			}
		}
	}

	if (createCount == 0)
	{
		// Normal start, or if nothing could be restored
		createWindowOnce(hasArgKeepvisibility, reset);
	}

	creationLock.unlock();
	return restore;
}

int KMixApp::newInstance()
{
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	bool hasArgKeepvisibility = args->isSet("keepvisibility");
	bool reset = args->isSet("failsafe");

	/**
	 * There are 3 cases when starting KMix:
	 * Autostart            : Cases 1) or 3) below
	 * Session restore      : Cases 1) or 2a) below
	 * Manual start by user : Cases 1) or 2b) below
	 *
	 * Each may be the creator a new instance, but only if the instance did not exist yet.
	 */


	//qCDebug(KMIX_LOG) <<  "KMixApp::newInstance() isRestored()=" << isRestored() << "_keepVisibility=" << _keepVisibility;
	/**
	 * 		NB See https://qa.mandriva.com/show_bug.cgi?id=56893#c3
	 *
	 * 		It is important to track this via a separate variable and not
	 * 		based on m_kmix to handle this race condition.
	 * 		Specific protection for the activation-prior-to-full-construction
	 * 		case exists above in the 'already running case'
	 */
	creationLock.lock(); // Guard a complete construction
	bool first = firstCaller;
	firstCaller = false;

	if (first)
	{
		/** CASE 1 *******************************************************
		 *
		 * Typical case: Normal start. KMix was not running yet => create a new KMixWindow
		 */
		GlobalConfig::init();
		restoreSessionIfApplicable(hasArgKeepvisibility, reset);

	}
	else
	{
		if (!hasArgKeepvisibility)
		{
			/** CASE 2 ******************************************************
			 *
			 * KMix is running, AND the *USER* starts it again (w/o --keepvisibilty)
			 * 2a) Restored the KMix main window will be shown.
			 * 2b) Not restored
			 */

			/*
			 * Restore Session. This may look strange to you, as the instance already exists. But the following
			 * sequence might happen:
			 * 1) Autostart (no restore) => create m_kmix instance (via CASE 1)
			 * 2) Session restore => we are here at this line of code (CASE 2). m_kmix exists, but still must be restored
			 *
			 */
			bool wasRestored = restoreSessionIfApplicable(hasArgKeepvisibility, reset);

			if (!wasRestored)
			{
				//
				// Use standard newInstances(), which shows and activates the main window. But skip it for the
				// special "restored" case, as we should not override the session rules.
				KUniqueApplication::newInstance();
			}
			// else: Do nothing, as session restore has done it.
		}
		else
		{
			/** CASE 3 ******************************************************
			 *
			 * KMix is running, AND launched again with --keepvisibilty
			 *
			 * Typical use case: Autostart
			 *
			 * =>  We don't want to change the visibiliy, thus we don't call show() here.
			 *
			 * Hint: --keepVisibility is used in kmix_autostart.desktop. It was used in history by KMilo
			 *       (see BKO 58901), but nowadays Mixer Applets nmight want to use it, though they should
			 *       use KMixD instead.
			 */
			qCDebug(KMIX_LOG)
			<< "KMixApp::newInstance() REGULAR_START _keepVisibility="
					<< hasArgKeepvisibility;
		}
	}

	creationLock.unlock();

	return 0;
}


#include "KMixApp.moc"
