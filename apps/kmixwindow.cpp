/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 1996-2014 The KMix authors. Maintainer: Christian Esken <esken@kde.org>
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

#include "kmixwindow.h"

// include files for Qt
#include <QApplication>
#include <QMenuBar>
#include <QTabWidget>
#include <QPointer>
#include <QHash>
#include <QTimer>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QCloseEvent>

// include files for KDE
#include <kxmlgui_version.h>
#include <kglobalaccel.h>
#include <kmessagebox.h>
#include <klocalizedstring.h>
#include <kstandardaction.h>
#include <kxmlguifactory.h>
#include <kprocess.h>
#include <kcoreaddons_version.h>

// KMix
#include "kmix_debug.h"
#include "core/mixertoolbox.h"
#include "core/kmixdevicemanager.h"
#include "gui/kmixerwidget.h"
#include "gui/kmixdockwidget.h"
#include "gui/kmixtoolbox.h"
#include "gui/dialogaddview.h"
#include "gui/dialogselectmaster.h"
#include "dbus/dbusmixsetwrapper.h"
#include "settings.h"

#ifdef HAVE_CANBERRA
#include "volumefeedback.h"
#endif


/* KMixWindow
 * Constructs a mixer window (KMix main window)
 */

KMixWindow::KMixWindow(KMixApp::StartupOptions options)
        : KXmlGuiWindow(nullptr),
	  m_autouseMultimediaKeys(true),
	  m_dockWidget(nullptr),
	  m_dontSetDefaultCardOnStart(false)
{
	setObjectName(QStringLiteral("KMixWindow"));
	// disable delete-on-close because KMix might just sit in the background waiting for cards to be plugged in
	setAttribute(Qt::WA_DeleteOnClose, false);

	m_noDockWidget = (options & KMixApp::NoSystemTray);
	m_startVisible = true;				// force on for failsafe reset

	const bool reset = (options & KMixApp::FailsafeReset);
	initActions(); // init actions first, so we can use them in the loadConfig() already
	if (!reset) loadBaseConfig(); // Load config before initMixer(), e.g. due to "MultiDriver" keyword
	initActionsLate(); // init actions that require a loaded config
	// TODO: Port to KF5, see MixerBackend::translateKernelToWhatsthis()
	//KGlobal::locale()->insertCatalog(QLatin1String("kmix-controls"));
	initWidgets();
	initPrefDlg();
	DBusMixSetWrapper::initialize(this, QStringLiteral("/Mixers"));
	MixerToolBox::initMixer(true); // with hotplugging enabled
	initActionsAfterInitMixer(); // init actions that require initialized mixer backend(s).

	recreateGUI(false, reset);
	if (m_wsMixers->count() < 1)
	{
		// Something is wrong. Perhaps a hardware or driver or backend change. Let KMix search harder
		recreateGUI(false, QString(), true, reset);
	}

	if (!qApp->isSessionRestored() ) // done by the session manager otherwise
		setInitialSize();

	fixConfigAfterRead();

	if (m_startVisible && !(options & KMixApp::KeepVisibility)) show();	// Started visible

	connect(qApp, &QCoreApplication::aboutToQuit, this, &KMixWindow::saveConfig);

	ControlManager::instance()->addListener(
			QString(), // All mixers (as the Global master Mixer might change)
			ControlManager::ControlList|ControlManager::MasterChanged, this,
			"KMixWindow");
#ifdef HAVE_CANBERRA
	VolumeFeedback::instance()->init();		// set up for volume feedback
#endif
	// Send an initial volume refresh (otherwise all volumes are 0 until the next change)
	ControlManager::instance()->announce(QString(), ControlManager::Volume, "Startup");
}

KMixWindow::~KMixWindow()
{
	ControlManager::instance()->removeListener(this);

	// -1- Cleanup Memory: clearMixerWidgets
	while (m_wsMixers->count() != 0)
	{
		QWidget *mw = m_wsMixers->widget(0);
		m_wsMixers->removeTab(0);
		delete mw;
	}

	// -2- Mixer HW
	MixerToolBox::deinitMixer();

	// -3- Action collection (just to please Valgrind)
	actionCollection()->clear();

	// GUIProfile cache should be cleared very very late, as GUIProfile instances are used in the Views, which
	// means main window and potentially also in the tray popup (at least we might do so in the future).
	// This place here could be to early, if we would start to GUIProfile outside KMixWIndow, e.g. in the tray popup.
	// Until we do so, this is the best place to call clearCache(). Later, e.g. in main() would likely be problematic.

	GUIProfile::clearCache();
}


void KMixWindow::controlsChange(ControlManager::ChangeType changeType)
{
	switch (changeType)
	{
	case ControlManager::ControlList:
	case ControlManager::MasterChanged:
		updateDocking();
		break;

	default:
		ControlManager::warnUnexpectedChangeType(changeType, this);
		break;
	}
}


void KMixWindow::initActions()
{
	// file menu
	KStandardAction::quit(this, &QCoreApplication::quit, actionCollection());

	QAction *action = actionCollection()->addAction(QStringLiteral("hide_kmixwindow"));
	action->setText(i18n("Hide Mixer Window"));
	connect(action, &QAction::triggered, this, &QWidget::close);
	actionCollection()->setDefaultShortcut(action, Qt::Key_Escape);

	// settings menu
	_actionShowMenubar = KStandardAction::showMenubar(this, &KMixWindow::toggleMenuBar, actionCollection());
	KStandardAction::preferences(this, &KMixWindow::showSettings, actionCollection());
	KStandardAction::keyBindings(guiFactory(), &KXMLGUIFactory::showConfigureShortcutsDialog, actionCollection());

	action = actionCollection()->addAction(QStringLiteral("launch_kdesoundsetup"));
	action->setText(i18n("Audio Setup..."));
	action->setIcon(QIcon::fromTheme("speaker"));
	connect(action, &QAction::triggered, this, &KMixWindow::slotKdeAudioSetupExec);

	action = actionCollection()->addAction(QStringLiteral("toggle_channels_currentview"));
	action->setText(i18n("Configure &Channels..."));
	action->setIcon(QIcon::fromTheme("settings-channels"));
	connect(action, &QAction::triggered, this, &KMixWindow::slotConfigureCurrentView);

	action = actionCollection()->addAction(QStringLiteral("select_master"));
	action->setText(i18n("Select Master Channel..."));
	action->setIcon(QIcon::fromTheme("settings-master"));
	connect(action, &QAction::triggered, this, &KMixWindow::slotSelectMaster);

	action = actionCollection()->addAction(QStringLiteral("save_1"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::SHIFT|Qt::Key_1);
	action->setText(i18n("Save volume profile 1"));
	connect(action, &QAction::triggered, this, &KMixWindow::saveVolumes1);

	action = actionCollection()->addAction(QStringLiteral("save_2"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::SHIFT|Qt::Key_2);
	action->setText(i18n("Save volume profile 2"));
	connect(action, &QAction::triggered, this, &KMixWindow::saveVolumes2);

	action = actionCollection()->addAction(QStringLiteral("save_3"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::SHIFT|Qt::Key_3);
	action->setText(i18n("Save volume profile 3"));
	connect(action, &QAction::triggered, this, &KMixWindow::saveVolumes3);

	action = actionCollection()->addAction(QStringLiteral("save_4"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::SHIFT|Qt::Key_4);
	action->setText(i18n("Save volume profile 4"));
	connect(action, &QAction::triggered, this, &KMixWindow::saveVolumes4);

	action = actionCollection()->addAction(QStringLiteral("load_1"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::Key_1);
	action->setText(i18n("Load volume profile 1"));
	connect(action, &QAction::triggered, this, &KMixWindow::loadVolumes1);

	action = actionCollection()->addAction(QStringLiteral("load_2"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::Key_2);
	action->setText(i18n("Load volume profile 2"));
	connect(action, &QAction::triggered, this, &KMixWindow::loadVolumes2);

	action = actionCollection()->addAction(QStringLiteral("load_3"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::Key_3);
	action->setText(i18n("Load volume profile 3"));
	connect(action, &QAction::triggered, this, &KMixWindow::loadVolumes3);

	action = actionCollection()->addAction(QStringLiteral("load_4"));
	actionCollection()->setDefaultShortcut(action, Qt::CTRL|Qt::Key_4);
	action->setText(i18n("Load volume profile 4"));
	connect(action, &QAction::triggered, this, &KMixWindow::loadVolumes4);

	createGUI(QLatin1String("kmixui.rc"));
}

void KMixWindow::initActionsLate()
{
	if (m_autouseMultimediaKeys)
	{
		QAction* globalAction = actionCollection()->addAction(QStringLiteral("increase_volume"));
		globalAction->setText(i18n("Increase Volume"));

		KGlobalAccel::setGlobalShortcut(globalAction, Qt::Key_VolumeUp);

		connect(globalAction, &QAction::triggered, this, &KMixWindow::slotIncreaseVolume);

		globalAction = actionCollection()->addAction(QStringLiteral("decrease_volume"));
		globalAction->setText(i18n("Decrease Volume"));
		KGlobalAccel::setGlobalShortcut(globalAction, Qt::Key_VolumeDown);
		connect(globalAction, &QAction::triggered, this, &KMixWindow::slotDecreaseVolume);

		globalAction = actionCollection()->addAction(QStringLiteral("mute"));
		globalAction->setText(i18n("Mute"));
		KGlobalAccel::setGlobalShortcut(globalAction, Qt::Key_VolumeMute);
		connect(globalAction, &QAction::triggered, this, &KMixWindow::slotMute);
	}
}

void KMixWindow::initActionsAfterInitMixer()
{
	// Only show the new tab widget if PulseAudio is not in use.
	// PulseAudio always shows 4 fixed GUI tabs.
	if (!MixerToolBox::pulseaudioPresent())
	{
		QPushButton *_cornerLabelNew = new QPushButton(this);
		_cornerLabelNew->setIcon(QIcon::fromTheme("tab-new"));
		_cornerLabelNew->setToolTip(i18n("Add new view"));
		m_wsMixers->setCornerWidget(_cornerLabelNew, Qt::TopLeftCorner);
		connect(_cornerLabelNew, &QAbstractButton::clicked, this, &KMixWindow::newView);
	}

	// If PulseAudio is in use, then that handles device hotplugging itself.
	// Therefore there is no need to watch for or handle hotplug events
	// ourselves, because that would lead to an ALSA driver being opened
	// for a device which is also being handled by PulseAudio.
	//
	// This is assumed to be be required, though, in the experimental and
	// untested multi-driver mode.
	if (!MixerToolBox::pulseaudioPresent() || MixerToolBox::isMultiDriverMode())
	{
		KMixDeviceManager *theKMixDeviceManager = KMixDeviceManager::instance();
		connect(theKMixDeviceManager, &KMixDeviceManager::plugged, this, &KMixWindow::plugged);
		connect(theKMixDeviceManager, &KMixDeviceManager::unplugged, this, &KMixWindow::unplugged);
		theKMixDeviceManager->initHotplug();
	}
}


//  This needs to done on initialisation, so that the
//  signal can be connected.
void KMixWindow::initPrefDlg()
{
	KMixPrefDlg *prefDlg = KMixPrefDlg::instance(this);
	connect(prefDlg, &KMixPrefDlg::kmixConfigHasChanged, this, &KMixWindow::applyPrefs);
}


void KMixWindow::initWidgets()
{
	m_wsMixers = new QTabWidget();
	m_wsMixers->setDocumentMode(true);
	setCentralWidget(m_wsMixers);
	m_wsMixers->setTabsClosable(false);
	connect(m_wsMixers, &QTabWidget::tabCloseRequested, this, &KMixWindow::saveAndCloseView);
	// TODO: connect this signal after all tabs are added, then there
	// will be no need for m_dontSetDefaultCardOnStart
	connect(m_wsMixers, &QTabWidget::currentChanged, this, &KMixWindow::newMixerShown);

	// show menubar if the actions says so (or if the action does not exist)
	menuBar()->setVisible((_actionShowMenubar==nullptr) || _actionShowMenubar->isChecked());
}

void KMixWindow::setInitialSize()
{
	// HACK: QTabWidget will bound its sizeHint to 200x200 unless scrollbuttons
	// are disabled, so we disable them, get a decent sizehint and enable them
	// back
	m_wsMixers->setUsesScrollButtons(false);
	QSize defSize = sizeHint();
	m_wsMixers->setUsesScrollButtons(true);
	QSize size = Settings::size();
	if (size.isNull()) size = defSize;
	if (!size.isNull()) resize(size);

	QPoint pos = Settings::position();
	if (!pos.isNull()) move(pos);
}


void KMixWindow::removeDock()
{
	if (m_dockWidget!=nullptr) m_dockWidget->deleteLater();
	m_dockWidget = nullptr;
}


bool KMixWindow::shouldShowDock() const
{
	return (!m_noDockWidget && Settings::showDockWidget());
}


/**
 * Creates or deletes the KMixDockWidget, depending on whether there is a Mixer instance available.
 *
 * @returns true, if the docking succeeded. Failure usually means that there
 *    was no suitable mixer control selected.
 */
bool KMixWindow::updateDocking()
{
	const bool showDock = shouldShowDock();

	// If there is no system tray icon, then disable this action
	// to indicate that closing the window will also quit KMix.
	QAction *act = actionCollection()->action(QStringLiteral("hide_kmixwindow"));
	if (act!=nullptr) act->setEnabled(showDock);

	if (!showDock) removeDock();
	else
	{
		if (m_dockWidget==nullptr) m_dockWidget = new KMixDockWidget(this);
	}

	return (showDock);
}


void KMixWindow::saveConfig()
{
	saveBaseConfig();
	saveViewConfig();
	saveVolumes();

	// TODO cesken The reason for not writing might be that we have multiple cascaded KConfig objects. I must migrate to KSharedConfig !!!
	KSharedConfig::openConfig()->sync();
	qCDebug(KMIX_LOG)
	<< "Saved config ... sync finished";
}

void KMixWindow::saveBaseConfig()
{
	Settings::setConfigVersion(KMIX_CONFIG_VERSION);

	Settings::setSize(size());
	Settings::setPosition(pos());
	// Cannot use isVisible() here, as in the "aboutToQuit()" case this widget is already hidden.
	// (Please note that the problem was only there when quitting via Systray - esken).
	// Using it again, as internal behaviour has changed with KDE4
	Settings::setVisible(isVisible());
	Settings::setMenubar(_actionShowMenubar->isChecked());

	// TODO: check whether the next line is needed
	Settings::setDefaultCardOnStart(m_defaultCardOnStart);
	Settings::setAutoUseMultimediaKeys(m_autouseMultimediaKeys);

	const MasterControl &master = MixerToolBox::getGlobalMasterPreferred(false);
	Settings::setMasterMixer(master.getCard());
	Settings::setMasterMixerDevice(master.getControl());

	// There is no point in saving this, it cannot be changed within KMix.
	//const QString mixerIgnoreExpression = MixerToolBox::mixerIgnoreExpression();
	//Settings::setMixerIgnoreExpression(mixerIgnoreExpression);

	Settings::self()->save();
	qCDebug(KMIX_LOG) << "Base configuration saved";
}

void KMixWindow::saveViewConfig()
{
	QMap<QString, QStringList> mixerViews;

	// The following loop is necessary for the case that the user has hidden all views for a Mixer instance.
	// Otherwise we would not save the Meta information (step -2- below for that mixer.
	// We also do not save dynamic mixers (e.g. PulseAudio)
	for (const Mixer *mixer : std::as_const(MixerToolBox::mixers()))
	{
		mixerViews[mixer->id()];		// just insert a map entry
	}

// -1- Save the views themselves
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		QWidget *w = m_wsMixers->widget(i);
		KMixerWidget *mw = qobject_cast<KMixerWidget *>(w);
		if (mw!=nullptr)
		{
			// Here also Views are saved. even for Mixers that are closed. This is necessary when unplugging cards.
			// Otherwise the user will be confused afer re-plugging the card (as the config was not saved).
			mw->saveConfig(Settings::self()->config());
			// add the view to the corresponding mixer list, so we can save a views-per-mixer list below
//			if (!mw->mixer()->isDynamic())
//			{
				QStringList& qsl = mixerViews[mw->mixer()->id()];
				qsl.append(mw->getGuiprof()->getId());
//			}
		}
	}

	// -2- Save Meta-Information (which views, and in which order). views-per-mixer list
	KConfigGroup pconfig(KSharedConfig::openConfig(), "Profiles");
	QMap<QString, QStringList>::const_iterator itEnd = mixerViews.constEnd();
	for (QMap<QString, QStringList>::const_iterator it = mixerViews.constBegin(); it != itEnd; ++it)
	{
		const QString& mixerProfileKey = it.key(); // this is actually some mixer->id()
		const QStringList& qslProfiles = it.value();
		pconfig.writeEntry(mixerProfileKey, qslProfiles);
		qCDebug(KMIX_LOG)
		<< "Save Profile List for " << mixerProfileKey << ", number of views is " << qslProfiles.count();
	}

	qCDebug(KMIX_LOG)
	<< "View configuration saved";
}

/**
 * Stores the volumes of all mixers  Can be restored via loadVolumes() or
 * the kmixctrl application.
 */

void KMixWindow::saveVolumes(const QString &postfix)
{
	const QString& kmixctrlRcFilename = getKmixctrlRcFilename(postfix);
	KConfig cfg(kmixctrlRcFilename);

	for (const Mixer *mixer : std::as_const(MixerToolBox::mixers()))
	{
		// Protect from unplugged devices - better to *not* save them
		if (mixer->isOpen()) mixer->volumeSave(&cfg);
	}

	cfg.sync();
	qCDebug(KMIX_LOG) << "Volume configuration saved";
}


QString KMixWindow::getKmixctrlRcFilename(const QString &postfix)
{
	QString kmixctrlRcFilename("kmixctrlrc");
	if (!postfix.isEmpty())
	{
		kmixctrlRcFilename.append(".").append(postfix);
	}
	return kmixctrlRcFilename;
}


void KMixWindow::loadBaseConfig()
{
	m_startVisible = Settings::visible();
	m_defaultCardOnStart = Settings::defaultCardOnStart();
	m_configVersion = Settings::configVersion();
	// WARNING Don't overwrite m_configVersion with the "correct" value, before having it
	// evaluated. Better only write that in saveBaseConfig()
	m_autouseMultimediaKeys = Settings::autoUseMultimediaKeys();
	QString mixerMasterCard = Settings::masterMixer();
	QString masterDev = Settings::masterMixerDevice();
	MixerToolBox::setGlobalMaster(mixerMasterCard, masterDev, true);

	// show/hide menu bar
	bool showMenubar = Settings::menubar();
	if (_actionShowMenubar!=nullptr) _actionShowMenubar->setChecked(showMenubar);
}

/**
 * Loads the volumes of all mixers from kmixctrlrc.
 * In other words:
 * Restores the default volumes as stored via saveVolumes() or the
 * execution of "kmixctrl --save"
 */

void KMixWindow::loadVolumes(const QString &postfix)
{
	qCDebug(KMIX_LOG) << "About to load config (Volume)";
	const QString &kmixctrlRcFilename = getKmixctrlRcFilename(postfix);
	const KConfig cfg(kmixctrlRcFilename);

	for (Mixer *mixer : std::as_const(MixerToolBox::mixers()))
	{
		mixer->volumeLoad(&cfg);
	}
}


void KMixWindow::recreateGUIwithSavingView()
{
	recreateGUI(true, false);
}

void KMixWindow::recreateGUI(bool saveConfig, bool reset)
{
	recreateGUI(saveConfig, QString(), false, reset);
}


/**
 * Create or recreate the Mixer GUI elements
 *
 * @param saveConfig  Whether to save all View configurations before recreating
 * @param forceNewTab To enforce opening a new tab, even when the profileList in the kmixrc is empty.
 *                    It should only be set to "true" in case of a Hotplug (because then the user definitely expects a new Tab to show).
 */
void KMixWindow::recreateGUI(bool saveConfig, const QString& mixerId, bool forceNewTab, bool reset)
{
	// -1- Remember which of the tabs is currently selected for restoration for re-insertion
	int oldTabPosition = m_wsMixers->currentIndex();

	if (!reset && saveConfig)
		saveViewConfig();  // save the state before recreating

	// -2- RECREATE THE ALREADY EXISTING TABS **********************************
	QHash<const Mixer *, bool> mixerHasProfile;

// -2a- Build a list of all active profiles in the main window (that means: from all tabs)
	QList<GUIProfile*> activeGuiProfiles;
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		const KMixerWidget *kmw = qobject_cast<const KMixerWidget *>(m_wsMixers->widget(i));
		if (kmw!=nullptr) activeGuiProfiles.append(kmw->getGuiprof());
	}

	for (const GUIProfile *guiprof : std::as_const(activeGuiProfiles))
	{
		const Mixer *mixer = MixerToolBox::findMixer(guiprof->getMixerId());
		if (mixer==nullptr)
		{
			qCCritical(KMIX_LOG) << "No mixer for the profile" << guiprof->getId();
			continue;
		}
		mixerHasProfile[mixer] = true;

		KMixerWidget* kmw = findKMWforTab(guiprof->getId());
		if (kmw==nullptr)
		{
			// does not yet exist => create
			addMixerWidget(mixer->id(), guiprof->getId(), -1);
		}
		else
		{
			// did exist => remove and insert new guiprof at old position
			int indexOfTab = m_wsMixers->indexOf(kmw);
			if ( indexOfTab != -1 ) m_wsMixers->removeTab(indexOfTab);
			delete kmw;
			addMixerWidget(mixer->id(), guiprof->getId(), indexOfTab);
		}
	} // Loop over all GUIProfile's



	// -3- ADD TABS FOR Mixer instances that have no tab yet **********************************
	KConfigGroup pconfig(KSharedConfig::openConfig(), "Profiles");
	for (const Mixer *mixer : std::as_const(MixerToolBox::mixers()))
	{
		if ( mixerHasProfile.contains(mixer))
		{
			continue;  // OK, this mixer already has a profile => skip it
		}


		// =========================================================================================
		// No TAB YET => This should mean KMix is just started, or the user has just plugged in a card

		{
			GUIProfile *guiprof = nullptr;
			if (reset)
			{
				guiprof = GUIProfile::find(mixer, QString("default"), false, true); // ### Card unspecific profile ###
			}

			if ( guiprof!=nullptr)
			{
				guiprof->setDirty();  // All fallback => dirty
				addMixerWidget(mixer->id(), guiprof->getId(), -1);
				continue;
			}
		}


		// =========================================================================================
		// The trivial cases have not added anything => Look at [Profiles] in config file

		const QStringList profileList = pconfig.readEntry( mixer->id(), QStringList() );
		const bool allProfilesRemovedByUser = pconfig.hasKey(mixer->id()) && profileList.isEmpty();
		if (allProfilesRemovedByUser)
		{
			continue; // User has explicitly hidden the views => do no further checks
		}

		// PulseAudio tabs are fixed and cannot be hidden by the user,
		// unless the configuration file key is present but has a blank
		// profile list (as tested by 'allProfilesRemovedByUser' above).
		if (mixer->getDriverName()=="PulseAudio") forceNewTab = true;

		{
			bool atLeastOneProfileWasAdded = false;

			for (const QString &profileId : std::as_const(profileList))
			{
				// This handles the profileList form the kmixrc
				qCDebug(KMIX_LOG) << "Searching for GUI profile" << profileId;
				GUIProfile* guiprof = GUIProfile::find(mixer, profileId, true, false);// ### Card specific profile ###

				if (guiprof==nullptr)
				{
					qCWarning(KMIX_LOG) << "Cannot load profile" << profileId;
					if (profileId.startsWith(QLatin1String("MPRIS2.")))
					{
						const QString fallbackProfileId = "MPRIS2.default";
						qCDebug(KMIX_LOG) << "For MPRIS2 falling back to" << fallbackProfileId;
						guiprof = GUIProfile::find(mixer, fallbackProfileId, true, false);
					}
				}

				if (guiprof!=nullptr)
				{
					addMixerWidget(mixer->id(), guiprof->getId(), -1);
					atLeastOneProfileWasAdded = true;
				}
			}

			if (atLeastOneProfileWasAdded)
			{
				// Continue
				continue;
			}
		}

		// =========================================================================================
		// Neither trivial cases have added something, nor the anything => Look at [Profiles] in config file

		// The we_need_a_fallback case is a bit tricky. Please ask the author (cesken) before even considering to change the code.
		const bool mixerIdMatch = mixerId.isEmpty() || (mixer->id() == mixerId);
		const bool thisMixerShouldBeForced = forceNewTab && mixerIdMatch;
		const bool we_need_a_fallback = mixerIdMatch && thisMixerShouldBeForced;
		if ( we_need_a_fallback )
		{
			// The profileList was empty or nothing could be loaded
			//     (Hint: This means the user cannot hide a device completely

			// Lets try a bunch of fallback strategies:
			qCDebug(KMIX_LOG) << "Attempting to find a card-specific GUI Profile for the mixer " << mixer->id();
			GUIProfile *guiprof = GUIProfile::find(mixer, QString("default"), false, false);// ### Card specific profile ###
			if (guiprof==nullptr)
			{
				qCDebug(KMIX_LOG) << "Not found. Attempting to find a generic GUI Profile for the mixer " << mixer->id();
				guiprof = GUIProfile::find(mixer, QString("default"), false, true); // ### Card unspecific profile ###
			}
			if (guiprof==nullptr)
			{
				qCDebug(KMIX_LOG) << "Using fallback GUI Profile for the mixer " << mixer->id();
				// This means there is neither card specific nor card unspecific profile
				// This is the case for some backends (as they don't ship profiles).
				guiprof = GUIProfile::fallbackProfile(mixer);
			}

			if (guiprof!=nullptr)
			{
				guiprof->setDirty();  // All fallback => dirty
				addMixerWidget(mixer->id(), guiprof->getId(), -1);
			}
			else
			{
				qCCritical(KMIX_LOG) << "Cannot use ANY profile (including Fallback) for mixer " << mixer->id() << " . This is impossible, and thus this mixer can NOT be used.";
			}

		}
	}
	mixerHasProfile.clear();

	// -4- FINALIZE **********************************


	// Show the system tray icon, if it is enabled and there
	// is at least one sound card.
	const bool dockingSucceded = updateDocking();
	if (reset)
	{
		// Always show the main window, along with the system tray
		// icon if enabled, on a GUI reset.
		show();
	}
	else if (m_wsMixers->count()>0)			// at least one tab present
	{
		if (oldTabPosition>=0) m_wsMixers->setCurrentIndex(oldTabPosition);

		// If there is no system tray icon, then ensure that the
		// main window is shown.  Originally this also checked
		// for '!MixerToolBox::mixers().empty()', but if that is
		// the case then there should be at least one tab.
		if (!dockingSucceded) show();
	}
	else
	{
		// TODO: is this correct?  This means that there is no GUI
		// way to quit KMix if there are no sound cards.
		//
		// No sound card was found.  Do not complain, but just sit
		// in the background and wait for newly plugged soundcards.
		// The updateDocking() below will also remove the system
		// tray icon until there is a sound card available.
		updateDocking();
		hide();
	}
}


KMixerWidget *KMixWindow::findKMWforTab(const QString &kmwId)
{
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		KMixerWidget *kmw = qobject_cast<KMixerWidget *>(m_wsMixers->widget(i));
		if (kmw->getGuiprof()->getId() == kmwId) return (kmw);
	}
	return (nullptr);
}

void KMixWindow::newView()
{
	const QList<Mixer *> &mixers = MixerToolBox::mixers();
	if (mixers.isEmpty())
	{
		qCCritical(KMIX_LOG) << "Trying to create a View, but no Mixer exists";
		return; // should never happen
	}

	Mixer *mixer = mixers.first();
	DialogAddView dav(this, mixer);

	const int ret = dav.exec();
	if (ret==QDialog::Accepted)
	{
		QString profileName = dav.getresultViewName();
		QString mixerId = dav.getresultMixerId();
		mixer = MixerToolBox::findMixer(mixerId);
		qCDebug(KMIX_LOG) << ">>> mixer = " << mixerId << " -> " << mixer;

		GUIProfile* guiprof = GUIProfile::find(mixer, profileName, false, false);
		if (guiprof == nullptr)
		{
			guiprof = GUIProfile::find(mixer, profileName, false, true);
		}

		if (guiprof == nullptr)
		{
			KMessageBox::error(this, i18n("Cannot add view - GUIProfile is invalid."), i18n("Error"));
		}
		else
		{
			const bool added = addMixerWidget(mixer->id(), guiprof->getId(), -1);
			if (!added)
			{
				KMessageBox::error(this, i18n("Cannot add view - View already exists."), i18n("Error"));
			}
		}
	}
}

/**
 * Save the view and close it
 *
 * @arg idx The index in the TabWidget
 */
void KMixWindow::saveAndCloseView(int idx)
{
	qCDebug(KMIX_LOG)
	<< "Enter";
	QWidget *w = m_wsMixers->widget(idx);
	KMixerWidget *kmw = qobject_cast<KMixerWidget *>(w);
	if (kmw!=nullptr)
	{
		kmw->saveConfig(Settings::self()->config()); // -<- This alone is not enough, as I need to save the META information as well. Thus use saveViewConfig() below
		m_wsMixers->removeTab(idx);
		updateTabsClosable();
		saveViewConfig();
		delete kmw;
	}

	qCDebug(KMIX_LOG) << "Exit";
}

void KMixWindow::fixConfigAfterRead()
{
	unsigned int configVersion = Settings::configVersion();
	if (configVersion < 3)
	{
		// Fix the "double Base" bug, by deleting all groups starting with "View.Base.Base.".
		// The group has been copied over by KMixToolBox::loadView() for all soundcards, so
		// we should be fine now
		QStringList cfgGroups = KSharedConfig::openConfig()->groupList();
		QStringListIterator it(cfgGroups);
		while (it.hasNext())
		{
			QString groupName = it.next();
			if (groupName.indexOf("View.Base.Base") == 0)
			{
				qCDebug(KMIX_LOG) << "Fixing group " << groupName;
				KConfigGroup buggyDevgrpCG(KSharedConfig::openConfig(), groupName);
				buggyDevgrpCG.deleteGroup();
			} // remove buggy group
		} // for all groups
	} // if config version < 3
}


void KMixWindow::plugged(const char *driverName, const QString &udi, int dev)
{
	qCDebug(KMIX_LOG) << "driver" << driverName << "dev" << dev << "UDI" << udi;
	Mixer *mixer = new Mixer(QString::fromLocal8Bit(driverName), dev);
	if (mixer==nullptr) return;

	mixer->setHotplugId(udi);			// record UDI for unplugging
	const QString mixerId = mixer->id();		// note for announce later
	if (MixerToolBox::possiblyAddMixer(mixer))
	{
		qCDebug(KMIX_LOG) << "adding mixer id" << mixer->id() << mixer->readableName();
		recreateGUI(true, mixer->id(), true, false);
	}
	else qCWarning(KMIX_LOG) << "Cannot add mixer to GUI";

	KMixToolBox::notification("CardHotplugged", i18n("The sound device '%1' was connected.",
							 mixer->readableName()));

	ControlManager::instance()->announce(mixerId, ControlManager::ControlList, objectName());
}


void KMixWindow::unplugged(const QString &udi)
{
	qCDebug(KMIX_LOG) << "UDI" << udi;

	// This assumes that there can be at most one mixer in the list
	// with the given UDI.
	Mixer *unpluggedMixer = nullptr;
	for (Mixer *mixer : std::as_const(MixerToolBox::mixers()))
	{
		if (mixer->hotplugId()==udi)
		{
			unpluggedMixer = mixer;
			break;
		}
	}

	if (unpluggedMixer==nullptr)
	{
		qCDebug(KMIX_LOG) << "No mixer present with that UDI";
		return;
	}

	const QString mixerId = unpluggedMixer->id();	// note for announce later

	qCDebug(KMIX_LOG) << "Removing mixer";
	const bool globalMasterMixerDestroyed = (unpluggedMixer==MixerToolBox::getGlobalMasterMixer());

	// Part 1: Remove tab from GUI
	//
	// Although there is assumed to be only one mixer with
	// the given UDI, there can be more than one GUI tab for it.
	for (int i = 0; i<m_wsMixers->count(); ++i)
	{
		const KMixerWidget *kmw = qobject_cast<const KMixerWidget *>(m_wsMixers->widget(i));
		if (kmw!=nullptr && kmw->mixer()==unpluggedMixer)
		{
			saveAndCloseView(i);
			// Restart the loop from scratch - indexes are
			// most likely invalidated by removeTab().
			i = -1;
		}
	}

	// Part 2: Remove the mixer from the known list.
	// First send the unplugged notifiation, while the unplugged
	// mixer and its readable name are still available.
	KMixToolBox::notification("CardUnplugged", i18n("The sound device '%1' was disconnected.",
							unpluggedMixer->readableName()));
	MixerToolBox::removeMixer(unpluggedMixer);

	// Part 3: Check whether the Global Master disappeared,
	// and select a new one if necessary
	shared_ptr<MixDevice> md = MixerToolBox::getGlobalMasterMD();
	if (globalMasterMixerDestroyed || md==nullptr)
	{
		// We don't know what the global master should be now.
		// So lets play stupid, and just select the recommended master
		// of the first device.

		// Re-fetch the list of mixers, since the unplugged one one was
		// removed above.
		const QList<Mixer *> &mixers = MixerToolBox::mixers();
		if (!mixers.isEmpty())
		{
			shared_ptr<MixDevice> master = mixers.first()->getLocalMasterMD();
			if (master!=nullptr)
			{
				Mixer *mixer = mixers.first();
				QString localMaster = master->id();
				MixerToolBox::setGlobalMaster(mixer->id(), localMaster, false);

				QString text = i18n("The current master device was disconnected. Changing master to '%1' on device '%2'.",
						    master->readableName(), mixer->readableName());
				KMixToolBox::notification("MasterFallback", text);
			}
		}
		else
		{
			QString text = i18n("The last sound device was disconnected. No sound devices are available.");
			KMixToolBox::notification("MasterFallback", text);
		}

		recreateGUI(true, false);
	}

	ControlManager::instance()->announce(mixerId, ControlManager::ControlList, objectName());
}


/**
 *
 */
bool KMixWindow::profileExists(QString guiProfileId)
{
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		const KMixerWidget *kmw = qobject_cast<KMixerWidget *>(m_wsMixers->widget(i));
		if (kmw!=nullptr && kmw->getGuiprof()->getId()==guiProfileId) return (true);
	}
	return (false);
}

bool KMixWindow::addMixerWidget(const QString& mixer_ID, QString guiprofId, int insertPosition)
{
	qCDebug(KMIX_LOG)
	<< "Add " << guiprofId;
	GUIProfile* guiprof = GUIProfile::find(guiprofId);
	if (guiprof!=nullptr && profileExists(guiprof->getId())) // TODO Bad place. Should be checked in the add-tab-dialog
		return (false); // already present => don't add again
	Mixer *mixer = MixerToolBox::findMixer(mixer_ID);
	if (mixer==nullptr) return (false);		// no such Mixer

	//       qCDebug(KMIX_LOG) << "KMixWindow::addMixerWidget() " << mixer_ID << " is being added";
	ViewBase::ViewFlags vflags = ViewBase::HasMenuBar;
	if ((_actionShowMenubar==nullptr) || _actionShowMenubar->isChecked())
		vflags |= ViewBase::MenuBarVisible;
	KMixerWidget *kmw = new KMixerWidget(mixer, this, vflags, guiprofId, actionCollection());
	/* A newly added mixer will automatically added at the top
	 * and thus the window title is also set appropriately */

	/*
	 * Skip the name from the profile for now. I would at least have to do the '&' quoting for the tab label. But I am
	 * also not 100% sure whether the current name from the profile is any good - it does (likely) not even contain the
	 * card ID. This means you cannot distinguish between cards with an identical name.
	 */
//  QString tabLabel = guiprof->getName();
//  if (tabLabel.isEmpty())
//	  QString tabLabel = kmw->mixer()->readableName(true);
	QString tabLabel = kmw->mixer()->readableName(true);

	m_dontSetDefaultCardOnStart = true; // inhibit implicit setting of m_defaultCardOnStart

	if (insertPosition == -1)
		m_wsMixers->addTab(kmw, tabLabel);
	else
		m_wsMixers->insertTab(insertPosition, kmw, tabLabel);

	if (kmw->getGuiprof()->getId() == m_defaultCardOnStart)
	{
		m_wsMixers->setCurrentWidget(kmw);
	}

	updateTabsClosable();
	m_dontSetDefaultCardOnStart = false;

	kmw->loadConfig(Settings::self()->config());
	// Now force to read for new tabs, especially after hotplug. Note: Doing it here is bad design and possibly
	// obsolete, as the backend should take care of updating itself.
	kmw->mixer()->readSetFromHWforceUpdate();

	return (true);
}

void KMixWindow::updateTabsClosable()
{
	// PulseAudio runs with 4 fixed tabs - don't allow to close them.
	// Also do not allow to close the last view
	m_wsMixers->setTabsClosable(!MixerToolBox::pulseaudioPresent() && m_wsMixers->count() > 1);
}


void KMixWindow::closeEvent(QCloseEvent *ev)
{
	if (m_dockWidget!=nullptr && !qApp->isSavingSession())
	{
		// The system tray icon is present, so just hide the
		// main window and do not close it.  The test for
		// whether m_dockWidget exists will have also taken
		// account of shouldShowDock().
		hide();
		ev->ignore();
	}
	else
	{
		// There is no system tray icon, so closing this
		// window quits the application.  Because we cannot
		// set WA_DeleteOnClose for this KMixWindow (because
		// KMixApp just keeps a simple pointer to its m_kmix
		// and so cannot know when it gets deleted), it is
		// necessary to explicitly quit the application here.
		QCoreApplication::quit();
		ev->accept();
	}
}


// internal helper to prevent code duplication in slotIncreaseVolume and slotDecreaseVolume
void KMixWindow::increaseOrDecreaseVolume(bool increase)
{
	Mixer* mixer = MixerToolBox::getGlobalMasterMixer(); // only needed for the awkward construct below
	if (mixer==nullptr) return;			     // e.g. when no soundcard is available
	shared_ptr<MixDevice> md = MixerToolBox::getGlobalMasterMD();
	if (md.get()==nullptr) return;			// shouldn't happen, but lets play safe

	Volume::VolumeTypeFlag volumeType = md->playbackVolume().hasVolume() ? Volume::Playback : Volume::Capture;
	md->increaseOrDecreaseVolume(!increase, volumeType);
	md->mixer()->commitVolumeChange(md);

	showVolumeDisplay();
}

void KMixWindow::slotIncreaseVolume()
{
	increaseOrDecreaseVolume(true);
}

void KMixWindow::slotDecreaseVolume()
{
	increaseOrDecreaseVolume(false);
}

void KMixWindow::showVolumeDisplay()
{
	Mixer* mixer = MixerToolBox::getGlobalMasterMixer();
	if (mixer==nullptr) return;			// e.g. when no soundcard is available
	shared_ptr<MixDevice> md = MixerToolBox::getGlobalMasterMD();
	if (md.get()==nullptr) return;			// shouldn't happen, but lets play safe

	if (Settings::showOSD())
	{
        QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.kde.plasmashell",
            "/org/kde/osdService",
            "org.kde.osdService",
            "volumeChanged"
        );

        int currentVolume = 0;
        if (!md->isMuted()) {
            currentVolume = md->playbackVolume().getAvgVolumePercent(Volume::MALL);
        }

        msg.setArguments(QList<QVariant>() << currentVolume);

        QDBusConnection::sessionBus().asyncCall(msg);
    }
}

/**
 * Mutes the global master.
 */
void KMixWindow::slotMute()
{
	Mixer* mixer = MixerToolBox::getGlobalMasterMixer();
	if (mixer==nullptr) return;			// e.g. when no soundcard is available
	shared_ptr<MixDevice> md = MixerToolBox::getGlobalMasterMD();
	if (md.get()==nullptr) return;			// shouldn't happen, but lets play safe
	md->toggleMute();
	mixer->commitVolumeChange(md);
	showVolumeDisplay();
}

/**
 * Shows the configuration dialog, with the "General" tab opened.
 */
void KMixWindow::showSettings()
{
	KMixPrefDlg::instance()->showAtPage(KMixPrefDlg::PageGeneral);
}


void KMixWindow::showHelp()
{
	actionCollection()->action("help_contents")->trigger();
}

void KMixWindow::showAbout()
{
	actionCollection()->action("help_about_app")->trigger();
}


/**
 * Apply the Preferences from the preferences dialog. Depending on what has been changed,
 * the corresponding announcements are made.
 */
void KMixWindow::applyPrefs(KMixPrefDlg::PrefChanges changes)
{
	qCDebug(KMIX_LOG) << "changes" << changes;

	if (changes & KMixPrefDlg::ChangedControls)
	{
		// These might need a complete relayout => announce a ControlList change to rebuild everything
		ControlManager::instance()->announce(QString(), ControlManager::ControlList, QString("Preferences Dialog"));
	}
	else if (changes & KMixPrefDlg::ChangedMaster)
	{
		// This announce was originally made in KMixPrefDlg::updateSettings().
		// It is treated as equivalent to ControlManager::ControlList by
		// the system tray popup, hence the 'else' here.
		ControlManager::instance()->announce(QString(), ControlManager::MasterChanged, QString("Select Backends Dialog"));
	}
	if (changes & KMixPrefDlg::ChangedGui)
	{
		ControlManager::instance()->announce(QString(), ControlManager::GUI, QString("Preferences Dialog"));
	}

	//this->repaint(); // make KMix look fast (saveConfig() often uses several seconds)
	qApp->processEvents();

	// Remove saveConfig() IF aa changes have been migrated to GlobalConfig.
	// Currently there is still stuff like "show menu bar".
	saveConfig();
}


void KMixWindow::toggleMenuBar()
{
	menuBar()->setVisible(_actionShowMenubar->isChecked());
}


void KMixWindow::slotKdeAudioSetupExec()
{
    forkExec(QStringList() << QString("kcmshell%1").arg(KCOREADDONS_VERSION_MAJOR) << "kcm_pulseaudio");
}


void KMixWindow::forkExec(const QStringList& args)
{
   int pid = KProcess::startDetached(args);
   if (pid == 0)
   {
       KMessageBox::error(this, i18n("The helper application is either not installed or not working.\n\n%1",
                         args.join(QLatin1String(" "))));
   }
}

void KMixWindow::slotConfigureCurrentView()
{
	const KMixerWidget *mw = qobject_cast<const KMixerWidget  *>(m_wsMixers->currentWidget());
	if (mw==nullptr) return;
	ViewBase *view = mw->currentView();
	if (view!=nullptr) view->configureView();
}


void KMixWindow::slotSelectMaster()
{
	const Mixer *mixer = MixerToolBox::getGlobalMasterMixer();
	if (mixer!=nullptr)
	{
		// m_masterSelectDialog will probably always be NULL here,
		// because closing the dialogue deletes itself (because of
		// the WA_DeleteOnClose set below) and the QPointer tracks
		// that deletion.
		if (m_masterSelectDialog.isNull())
		{
			m_masterSelectDialog = new DialogSelectMaster(mixer, this);
			m_masterSelectDialog->setAttribute(Qt::WA_DeleteOnClose, true);
			m_masterSelectDialog->show();
		}

		m_masterSelectDialog->raise();
		m_masterSelectDialog->activateWindow();
	}
	else
	{
		KMessageBox::error(nullptr, KMixToolBox::noDevicesWarningString());
	}
}


void KMixWindow::newMixerShown(int /*tabIndex*/)
{
	const KMixerWidget *kmw = qobject_cast<const KMixerWidget *>(m_wsMixers->currentWidget());
	if (kmw==nullptr) return;			// no current mixer

	// I am using the app name as a PREFIX, as KMix is a single window
	// application and it is more helpful to the user to see "KDE Mixer"
	// in a window list than a possibly cryptic soundcard name like "HDA ATI SB".
	// Reformatted for KF5 so as to not say "KDE" and so that there are not
	// two different dashes.
	setWindowTitle(i18n("Mixer (%1)", kmw->mixer()->readableName()));

	if (!m_dontSetDefaultCardOnStart)
		m_defaultCardOnStart = kmw->getGuiprof()->getId();

	// As switching the tab does NOT mean switching the master card,
	// we do not need to update the dock icon here.  It would lead to
	// unnecesary flickering of the (complete) dock area.

	// The "Configure Channels..." action is only applicable if the
	// current mixer is not dynamic.  In order to keep the presence
	// or absence of the action consistent between tabs, if the current
	// mixer is PulseAudio then the action is hidden because it will
	// never be applicable.  Otherwise the action is shown, and enabled
	// unless the current mixer is dynamic - where the only other dynamic
	// mixer is MPRIS2.
	const ViewBase *view = kmw->currentView();
	QAction *action = actionCollection()->action("toggle_channels_currentview");
	if (view!=nullptr && action!=nullptr)
	{
		if (kmw->mixer()->getDriverName()=="PulseAudio") action->setVisible(false);
		else
		{
			action->setVisible(true);
			action->setEnabled(!view->isDynamic());
		}
	}
}
