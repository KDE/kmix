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

#include <qdbusinterface.h>
#include <qdbusmessage.h>

#include "kmix_debug.h"
#include "core/ControlManager.h"
#include "core/mixertoolbox.h"
#include "core/volume.h"
#include "apps/kmixwindow.h"
#include "settings.h"


static bool firstCaller = true;


// Originally this class was a subclass of KUniqueApplication.
// Since, now that a unique application is enforced earlier by KDBusService,
// the only purpose of this class is to receive the activateRequested()
// signal.  It can therefore be a simple QObject.

KMixApp::KMixApp()
	: QObject(),
      m_kmix(nullptr)
{
	// We must disable QuitOnLastWindowClosed. Rationale:
	// 1) The normal state of KMix is to only have the dock icon shown.
	// 2a) The dock icon gets reconstructed, whenever a soundcard is hotplugged or unplugged.
	// 2b) The dock icon gets reconstructed, when the user selects a new master.
	// 3) During the reconstruction, it can easily happen that no window is present => KMix would quit
	// => disable QuitOnLastWindowClosed
	qApp->setQuitOnLastWindowClosed(false);
}

KMixApp::~KMixApp()
{
	qCDebug(KMIX_LOG) << "Deleting KMixApp";
	ControlManager::instance()->shutdownNow();
	delete m_kmix;
	m_kmix = nullptr;
	Settings::self()->save();
}

void KMixApp::createWindowOnce(bool hasArgKeepvisibility, bool reset)
{
	// Create window, if it was not yet created (e.g. via autostart or manually)
	if (m_kmix==nullptr)
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
	 * depends on implementation details inside Qt, which COULD potentially lead to the following scenarios:
	 * 1) "Autostart" and "session restore" run concurrently in 2 different Threads.
	 * 2) The current "main/gui" thread "pops up" a "session restore" message from the Qt event dispatcher.
	 *    This means that  "Autostart" and "session restore" run interleaved in a single Thread.
	 */
	creationLock.lock();

	bool restore = qApp->isSessionRestored(); // && KMainWindow::canBeRestored(0);
	qCDebug(KMIX_LOG) << "Starting KMix using keepvisibility=" << hasArgKeepvisibility << ", failsafe=" << reset << ", sessionRestore=" << restore;
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
				// This code path is "impossible". It is here only for analyzing possible issues with session restoring.
				// KMix is a single-instance app. If more than one instance is created we have a bug.
				qCWarning(KMIX_LOG) << "KDE session management wants to restore multiple instances of KMix. Please report this as a bug.";
				break;
			}
			else
			{
				// Create window, if it was not yet created (e.g. via autostart or manually)
				createWindowOnce(hasArgKeepvisibility, reset);
				// #restore() is called with the parameter of "show == false", as KMixWindow itself decides on it.
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


// If this application ("Classic KMix") is running, then there is no
// point in the KDED module also running because device hotplug is
// handled by us.  So, if that KDED module is loaded, tell KDED to
// unload it and also set it to not automatically start in future.

static void disableKdedModule(const QString &moduleName)
{
	qCDebug(KMIX_LOG) << "Checking KDED" << moduleName << "module";

	QDBusInterface kded("org.kde.kded5", "/kded", "org.kde.kded5", QDBusConnection::sessionBus());
	if (!kded.isValid())
	{
		qCWarning(KMIX_LOG) << "cannot connect to KDED";
		return;
	}

	QDBusMessage reply = kded.call("loadedModules");
	const QStringList loaded = reply.arguments().at(0).toStringList();
	if (loaded.contains(moduleName))
	{
		qCDebug(KMIX_LOG) << "Module is currently loaded, unloading it";
		reply = kded.call("unloadModule", moduleName);
		if (reply.type()==QDBusMessage::ErrorMessage)
		{
			qCWarning(KMIX_LOG) << "cannot unload module," << reply.errorMessage();
		}

		reply = kded.call("isModuleAutoloaded", moduleName);
		if (reply.type()==QDBusMessage::ErrorMessage)
		{
			qCWarning(KMIX_LOG) << "cannot check whether module is autoloaded," << reply.errorMessage();
			return;
		}

		if (reply.arguments().at(0).toBool())
		{
			qCDebug(KMIX_LOG) << "Disabling module autoload";
			reply = kded.call("setModuleAutoloading", moduleName, false);
			if (reply.type()==QDBusMessage::ErrorMessage)
			{
				qCWarning(KMIX_LOG) << "cannot disable autoload," << reply.errorMessage();
			}
		}
	}

	qCDebug(KMIX_LOG) << "KDED module checks done";
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
		restoreSessionIfApplicable(m_hasArgKeepvisibility, m_hasArgReset);

		// Check for and disable the "kmixd" KDED module if necessary.
		disableKdedModule("kmixd");
	}
	else
	{
		if (!m_hasArgKeepvisibility)
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
			bool wasRestored = restoreSessionIfApplicable(m_hasArgKeepvisibility, m_hasArgReset);
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
			qCDebug(KMIX_LOG)
			<< "KMixApp::newInstance() REGULAR_START _keepVisibility="
					<< m_hasArgKeepvisibility;
		}
	}

	creationLock.unlock();
}


void KMixApp::parseOptions(const QCommandLineParser &parser)
{
	m_hasArgKeepvisibility = parser.isSet("keepvisibility");
	m_hasArgReset = parser.isSet("failsafe");

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
