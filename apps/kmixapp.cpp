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

#include "kmixapp.h"

#include <qapplication.h>
#include <qcommandlineparser.h>

#include "kmix_debug.h"
#include "core/ControlManager.h"
#include "core/mixertoolbox.h"
#include "core/volume.h"
#include "apps/kmixwindow.h"
#include "settings.h"


static bool firstCaller = true;


// Originally this class was a subclass of KUniqueApplication.
// Now that a unique application is enforced earlier by KDBusService,
// the only purpose of this class is to receive the activateRequested()
// signal.  It can therefore be a simple QObject.

KMixApp::KMixApp()
{
	// We must disable QuitOnLastWindowClosed. Rationale:
	// 1) The normal state of KMix is to only have the dock icon shown.
	// 2a) The dock icon gets reconstructed, whenever a soundcard is hotplugged or unplugged.
	// 2b) The dock icon gets reconstructed, when the user selects a new master.
	// 3) During the reconstruction, it can easily happen that no window is present => KMix would quit
	// => disable QuitOnLastWindowClosed
	qApp->setQuitOnLastWindowClosed(false);

	m_startupOptions = KMixApp::StartupOptions();	// command line not parsed
}

KMixApp::~KMixApp()
{
	qCDebug(KMIX_LOG) << "Deleting KMixApp";
	ControlManager::instance()->shutdownNow();
	delete m_kmix;
	Settings::self()->save();
}

void KMixApp::createWindowOnce()
{
	// Create window, if it was not yet created (e.g. via autostart or manually)
	if (m_kmix.isNull())
	{
		qCDebug(KMIX_LOG) << "Creating new KMixWindow";
		m_kmix = new KMixWindow(m_startupOptions);
	}
}

bool KMixApp::restoreSessionIfApplicable()
{
	/**
	 * We should lock session creation. Rationale:
	 * KMix can be started multiple times during session start. By "autostart" and "session restore". The order is
	 * undetermined, as KMix will initialize in the background of KDE session startup (Hint: As a
	 * KUniqueApplication it decouples from the startkde process!).
	 *
	 * Now we must make sure that window creation is definitely done, before the "other" process comes, as it might
	 * want to restore the session. Working on a half-created window would not be smart! Why can this happen? It
	 * depends on implementation details inside Qt, which COULD potentially lead to the following scenarios:
	 * 1) "Autostart" and "session restore" run concurrently in 2 different Threads.
	 * 2) The current "main/gui" thread "pops up" a "session restore" message from the Qt event dispatcher.
	 *    This means that  "Autostart" and "session restore" run interleaved in a single Thread.
	 */
	creationLock.lock();

	// TODO: is it really worth supporting session restore in this application?
	// There can only be one instance of KMix, with one main window and
	// optionally a system tray icon.  It is started on desktop login via
	// autostart and persists.  Volumes are also restored via autostart.
	// So it is not clear what session restore actually achieves - maybe
	// just the open/hidden state of the window, even if that?  Eliminating
	// session restore would simplify a lot.

	const bool restore = qApp->isSessionRestored(); // && KMainWindow::canBeRestored(0);
	qCDebug(KMIX_LOG) << "Startup options" << m_startupOptions << "restore?" << restore;
	int createCount = 0;
	if (restore)
	{
		if (m_startupOptions & KMixApp::FailsafeReset)
		{
			qCWarning(KMIX_LOG) << "Reset cannot be performed while KMix is running. Please quit KMix and retry then.";
		}
		int n = 1;
		while (KMainWindow::canBeRestored(n))
		{
			qCDebug(KMIX_LOG) << "Restoring window" << n;
			if (n > 1)
			{
				// This code path is "impossible". It is here only for analyzing possible issues with session restoring.
				// KMix is a single-instance app. If more than one instance is created we have a bug.
				qCWarning(KMIX_LOG) << "KDE session management wants to restore multiple instances of KMix. Please report this as a bug.";
				break;
			}
			else
			{
				// Create the window, if it has not yet been created
				// (e.g. via autostart or manually).
				createWindowOnce();

				// Having done the above, we now know that m_kmix
				// is valid.  Its restore() is called with the 'show'
				// parameter as false, as KMixWindow makes its own
				// decision as to whether to do that.
				m_kmix->restore(n, false);
				createCount++;
				n++;
			}
		}
	}

	if (createCount == 0)
	{
		// Normal start, or if nothing could be restored
		createWindowOnce();
	}

	creationLock.unlock();
	return restore;
}


void KMixApp::newInstance(const QStringList &arguments, const QString &workingDirectory)
{
	qCDebug(KMIX_LOG);

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
	const bool first = firstCaller;
	firstCaller = false;

	if (first)
	{
		/** CASE 1 *******************************************************
		 *
		 * Typical case: Normal start. KMix was not running yet => create a new KMixWindow
		 */
		restoreSessionIfApplicable();
	}
	else
	{
		if (!(m_startupOptions & KMixApp::KeepVisibility))
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
			bool wasRestored = restoreSessionIfApplicable();
			if (!wasRestored)
			{
				//
				// Use standard newInstances(), which shows and activates the main window. But skip it for the
				// special "restored" case, as we should not override the session rules.
				// TODO: what should be done for KF5?
				//KUniqueApplication::newInstance();
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
			 * =>  We don't want to change the visibility, thus we don't call show() here.
			 *
			 * Hint: --keepVisibility is used in kmix_autostart.desktop. It was used in history by KMilo
			 *       (see BKO 58901), but nowadays Mixer Applets might want to use it, though they should
			 *       use KMixD instead.
			 */
			qCDebug(KMIX_LOG) << "Normal startup with options" << m_startupOptions;
		}
	}

	creationLock.unlock();
}


void KMixApp::parseOptions(const QCommandLineParser &parser)
{
	if (parser.isSet("keepvisibility")) m_startupOptions |= KMixApp::KeepVisibility;
	if (parser.isSet("failsafe")) m_startupOptions |= KMixApp::FailsafeReset;
	if (parser.isSet("nosystemtray")) m_startupOptions |= KMixApp::NoSystemTray;

	// Hidden configuration settings which affect the entire application
	// are also parsed and set here.  Therefore they do not need to be passed
	// all the way down to KMixWindow only to eventually end up in the same
	// place anyway.

	// The match expression for ignored mixer names.
	QString mixerIgnoreExpression = Settings::mixerIgnoreExpression();
	if (!mixerIgnoreExpression.isEmpty()) MixerToolBox::setMixerIgnoreExpression(mixerIgnoreExpression);

	// The global volume step setting.
	const int volumePercentageStep = Settings::volumePercentageStep();
	if (volumePercentageStep>0) Volume::setVolumeStep(volumePercentageStep);

	// The following log is very helpful in bug reports. Please keep it.
	QStringList backendFilter = Settings::backends();
	qCDebug(KMIX_LOG) << "Backends from settings =" << backendFilter;
	if (parser.isSet("backends"))
	{
		// The command line option overrides the configured setting.
		backendFilter = parser.value("backends").split(',');
		qCDebug(KMIX_LOG) << "Backends from command line =" << backendFilter;
	}
	if (!backendFilter.isEmpty()) MixerToolBox::setAllowedBackends(backendFilter);

	bool multiDriverMode = Settings::multiDriver();
	qCDebug(KMIX_LOG) << "Multi driver mode from settings =" << multiDriverMode;
	if (parser.isSet("multidriver"))
	{
		multiDriverMode = true;
		qCDebug(KMIX_LOG) << "Multi driver mode from command line =" << multiDriverMode;
	}

	if (multiDriverMode) MixerToolBox::setMultiDriverMode(true);
}
