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

#include "apps/kmix.h"

// include files for QT
#include <QCheckBox>
#include <QLabel>
#include <QDesktopWidget>
#include <QPushButton>
#include <qradiobutton.h>
#include <QCursor>
#include <QString>

// include files for KDE
#include <KConfigSkeleton>
#include <kcombobox.h>
#include <KGlobalAccel>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <ktoggleaction.h>
#include <kapplication.h>
#include <kstandardaction.h>
#include <khelpmenu.h>
#include <kdebug.h>
#include <kxmlguifactory.h>
#include <kglobal.h>
#include <kactioncollection.h>
#include <KProcess>
#include <KTabWidget>

// KMix
#include "gui/guiprofile.h"
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"
#include "core/MasterControl.h"
#include "core/MediaController.h"
#include "core/mixertoolbox.h"
#include "core/kmixdevicemanager.h"
#include "gui/kmixerwidget.h"
#include "gui/kmixprefdlg.h"
#include "gui/kmixdockwidget.h"
#include "gui/kmixtoolbox.h"
#include "core/version.h"
#include "gui/viewdockareapopup.h"
#include "gui/dialogaddview.h"
#include "gui/dialogselectmaster.h"
#include "dbus/dbusmixsetwrapper.h"
#ifdef X_KMIX_KF5_BUILD
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusPendingCall>
#else
#include "gui/osdwidget.h"
#endif

#ifdef X_KMIX_KF5_BUILD
#define CLASS_Action QAction
#include <QKeySequence>
#define CLASS_KShortcut QKeySequence
#else
#define CLASS_Action KAction
#define CLASS_KShortcut KShortcut
#define QStringLiteral QLatin1String
#endif

/* KMixWindow
 * Constructs a mixer window (KMix main window)
 */

KMixWindow::KMixWindow(bool invisible, bool reset) :
		KXmlGuiWindow(0, Qt::WindowFlags(
		KDE_DEFAULT_WINDOWFLAGS | Qt::WindowContextHelpButtonHint)), m_multiDriverMode(false), // -<- I never-ever want the multi-drivermode to be activated by accident
		m_autouseMultimediaKeys(true),
		m_dockWidget(), m_dsm(0), m_dontSetDefaultCardOnStart(false)
{
	setObjectName(QStringLiteral("KMixWindow"));
	// disable delete-on-close because KMix might just sit in the background waiting for cards to be plugged in
	setAttribute(Qt::WA_DeleteOnClose, false);

	initActions(); // init actions first, so we can use them in the loadConfig() already
	loadAndInitConfig(reset); // Load config before initMixer(), e.g. due to "MultiDriver" keyword
	initActionsLate(); // init actions that require a loaded config
	KGlobal::locale()->insertCatalog(QLatin1String("kmix-controls"));
	initWidgets();
	initPrefDlg();
	DBusMixSetWrapper::initialize(this, QStringLiteral("/Mixers"));
	MixerToolBox::instance()->initMixer(m_multiDriverMode, m_backendFilter, m_hwInfoString, true);
	KMixDeviceManager *theKMixDeviceManager = KMixDeviceManager::instance();
	initActionsAfterInitMixer(); // init actions that require initialized mixer backend(s).

	recreateGUI(false, reset);
	if (m_wsMixers->count() < 1)
	{
		// Something is wrong. Perhaps a hardware or driver or backend change. Let KMix search harder
		recreateGUI(false, QString(), true, reset);
	}

	if (!kapp->isSessionRestored() ) // done by the session manager otherwise
	setInitialSize();

	fixConfigAfterRead();
	theKMixDeviceManager->initHotplug();
	connect(theKMixDeviceManager, SIGNAL(plugged(const char*,QString,QString&)),
			SLOT(plugged(const char*,QString,QString&)));
	connect(theKMixDeviceManager, SIGNAL(unplugged(QString)), SLOT(unplugged(QString)));
	if (m_startVisible && !invisible)
		show(); // Started visible

	connect(kapp, SIGNAL(aboutToQuit()), SLOT(saveConfig()) );

	ControlManager::instance().addListener(
			QString(), // All mixers (as the Global master Mixer might change)
			(ControlChangeType::Type) (ControlChangeType::ControlList | ControlChangeType::MasterChanged), this,
			"KMixWindow");

	// Send an initial volume refresh (otherwise all volumes are 0 until the next change)
	ControlManager::instance().announce(QString(), ControlChangeType::Volume, "Startup");
}

KMixWindow::~KMixWindow()
{
	ControlManager::instance().removeListener(this);

	delete m_dsm;
#ifndef X_KMIX_KF5_BUILD
	delete osdWidget;
#endif
	// -1- Cleanup Memory: clearMixerWidgets
	while (m_wsMixers->count() != 0)
	{
		QWidget *mw = m_wsMixers->widget(0);
		m_wsMixers->removeTab(0);
		delete mw;
	}

	// -2- Mixer HW
	MixerToolBox::instance()->deinitMixer();

	// -3- Action collection (just to please Valgrind)
	actionCollection()->clear();

	// GUIProfile cache should be cleared very very late, as GUIProfile instances are used in the Views, which
	// means main window and potentially also in the tray popup (at least we might do so in the future).
	// This place here could be to early, if we would start to GUIProfile outside KMixWIndow, e.g. in the tray popup.
	// Until we do so, this is the best place to call clearCache(). Later, e.g. in main() would likely be problematic.

	GUIProfile::clearCache();

}

void KMixWindow::controlsChange(int changeType)
{
	ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
	switch (type)
	{
	case ControlChangeType::ControlList:
	case ControlChangeType::MasterChanged:
		updateDocking();
		break;

	default:
		ControlManager::warnUnexpectedChangeType(type, this);
		break;
	}

}


void KMixWindow::initActions()
{
	// file menu
	KStandardAction::quit(this, SLOT(quit()), actionCollection());

	// settings menu
	_actionShowMenubar = KStandardAction::showMenubar(this, SLOT(toggleMenuBar()), actionCollection());
	//actionCollection()->addAction(QStringLiteral( a->objectName()), a );
	KStandardAction::preferences(this, SLOT(showSettings()), actionCollection());
	KStandardAction::keyBindings(guiFactory(), SLOT(configureShortcuts()), actionCollection());
	CLASS_Action* action = actionCollection()->addAction(QStringLiteral("launch_kdesoundsetup"));
	action->setText(i18n("Audio Setup"));
	connect(action, SIGNAL(triggered(bool)), SLOT(slotKdeAudioSetupExec()));

	action = actionCollection()->addAction(QStringLiteral("hwinfo"));
	action->setText(i18n("Hardware &Information"));
	connect(action, SIGNAL(triggered(bool)), SLOT(slotHWInfo()));
	action = actionCollection()->addAction(QStringLiteral("hide_kmixwindow"));
	action->setText(i18n("Hide Mixer Window"));
	connect(action, SIGNAL(triggered(bool)), SLOT(hideOrClose()));
	action->setShortcut(QKeySequence(Qt::Key_Escape));
	action = actionCollection()->addAction(QStringLiteral("toggle_channels_currentview"));
	action->setText(i18n("Configure &Channels..."));
	connect(action, SIGNAL(triggered(bool)), SLOT(slotConfigureCurrentView()));
	action = actionCollection()->addAction(QStringLiteral("select_master"));
	action->setText(i18n("Select Master Channel..."));
	connect(action, SIGNAL(triggered(bool)), SLOT(slotSelectMaster()));

	action = actionCollection()->addAction(QStringLiteral("save_1"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_1));
	action->setText(i18n("Save volume profile 1"));
	connect(action, SIGNAL(triggered(bool)), SLOT(saveVolumes1()));

	action = actionCollection()->addAction(QStringLiteral("save_2"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_2));
	action->setText(i18n("Save volume profile 2"));
	connect(action, SIGNAL(triggered(bool)), SLOT(saveVolumes2()));

	action = actionCollection()->addAction(QStringLiteral("save_3"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_3));
	action->setText(i18n("Save volume profile 3"));
	connect(action, SIGNAL(triggered(bool)), SLOT(saveVolumes3()));

	action = actionCollection()->addAction(QStringLiteral("save_4"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_4));
	action->setText(i18n("Save volume profile 4"));
	connect(action, SIGNAL(triggered(bool)), SLOT(saveVolumes4()));

	action = actionCollection()->addAction(QStringLiteral("load_1"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::Key_1));
	action->setText(i18n("Load volume profile 1"));
	connect(action, SIGNAL(triggered(bool)), SLOT(loadVolumes1()));

	action = actionCollection()->addAction(QStringLiteral("load_2"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::Key_2));
	action->setText(i18n("Load volume profile 2"));
	connect(action, SIGNAL(triggered(bool)), SLOT(loadVolumes2()));

	action = actionCollection()->addAction(QStringLiteral("load_3"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::Key_3));
	action->setText(i18n("Load volume profile 3"));
	connect(action, SIGNAL(triggered(bool)), SLOT(loadVolumes3()));

	action = actionCollection()->addAction(QStringLiteral("load_4"));
	action->setShortcut(CLASS_KShortcut(Qt::CTRL + Qt::Key_4));
	action->setText(i18n("Load volume profile 4"));
	connect(action, SIGNAL(triggered(bool)), SLOT(loadVolumes4()));

#ifndef X_KMIX_KF5_BUILD
	osdWidget = new OSDWidget();
#endif

	createGUI(QLatin1String("kmixui.rc"));
}

void KMixWindow::initActionsLate()
{
	if (m_autouseMultimediaKeys)
	{
		CLASS_Action* globalAction = actionCollection()->addAction(QStringLiteral("increase_volume"));
		globalAction->setText(i18n("Increase Volume"));

#ifdef X_KMIX_KF5_BUILD
		KGlobalAccel::setGlobalShortcut(globalAction, Qt::Key_VolumeUp);
#else
		globalAction->setGlobalShortcut(CLASS_KShortcut(Qt::Key_VolumeUp));
#endif

		connect(globalAction, SIGNAL(triggered(bool)), SLOT(slotIncreaseVolume()));

		globalAction = actionCollection()->addAction(QStringLiteral("decrease_volume"));
		globalAction->setText(i18n("Decrease Volume"));
#ifdef X_KMIX_KF5_BUILD
		KGlobalAccel::setGlobalShortcut(globalAction, Qt::Key_VolumeDown);
#else
		globalAction->setGlobalShortcut(CLASS_KShortcut(Qt::Key_VolumeDown));
#endif
		connect(globalAction, SIGNAL(triggered(bool)), SLOT(slotDecreaseVolume()));

		globalAction = actionCollection()->addAction(QStringLiteral("mute"));
		globalAction->setText(i18n("Mute"));
#ifdef X_KMIX_KF5_BUILD
		KGlobalAccel::setGlobalShortcut(globalAction, Qt::Key_VolumeMute);
#else
		globalAction->setGlobalShortcut(CLASS_KShortcut(Qt::Key_VolumeMute));
#endif
		connect(globalAction, SIGNAL(triggered(bool)), SLOT(slotMute()));
	}
}

void KMixWindow::initActionsAfterInitMixer()
{
	// Only show the new tab widget if Pulseaudio is not used. Hint: The Pulseaudio backend always
	// runs with 4 fixed Tabs.
	if (!Mixer::pulseaudioPresent())
	{
		QPixmap cornerNewPM = KIconLoader::global()->loadIcon("tab-new", KIconLoader::Toolbar,
				IconSize(KIconLoader::Toolbar));
		QPushButton* _cornerLabelNew = new QPushButton();
		_cornerLabelNew->setIcon(cornerNewPM);
		//cornerLabelNew->setSizePolicy(QSizePolicy());
		m_wsMixers->setCornerWidget(_cornerLabelNew, Qt::TopLeftCorner);
		connect(_cornerLabelNew, SIGNAL(clicked()), SLOT(newView()));
	}
}

void KMixWindow::initPrefDlg()
{
	KMixPrefDlg* prefDlg = KMixPrefDlg::createInstance(this, GlobalConfig::instance());
	connect(prefDlg, SIGNAL(kmixConfigHasChanged()), SLOT(applyPrefs()));
}

void KMixWindow::initWidgets()
{
	m_wsMixers = new KTabWidget();
	m_wsMixers->setDocumentMode(true);
	setCentralWidget(m_wsMixers);
	m_wsMixers->setTabsClosable(false);
	connect(m_wsMixers, SIGNAL(tabCloseRequested(int)), SLOT(saveAndCloseView(int)));

	QPixmap cornerNewPM = KIconLoader::global()->loadIcon("tab-new", KIconLoader::Toolbar,
			IconSize(KIconLoader::Small));

	connect(m_wsMixers, SIGNAL(currentChanged(int)), SLOT(newMixerShown(int)));

	// show menubar if the actions says so (or if the action does not exist)
	menuBar()->setVisible((_actionShowMenubar == 0) || _actionShowMenubar->isChecked());
}

void KMixWindow::setInitialSize()
{
#ifdef X_KMIX_KF5_BUILD
	KConfigGroup config(KSharedConfig::openConfig(), "Global");
#else
	KConfigGroup config(KGlobal::config(), "Global");
#endif

	// HACK: QTabWidget will bound its sizeHint to 200x200 unless scrollbuttons
	// are disabled, so we disable them, get a decent sizehint and enable them
	// back
	m_wsMixers->setUsesScrollButtons(false);
	QSize defSize = sizeHint();
	m_wsMixers->setUsesScrollButtons(true);
	QSize size = config.readEntry("Size", defSize);
	if (!size.isEmpty())
		resize(size);

	QPoint defPos = pos();
	QPoint pos = config.readEntry("Position", defPos);
	move(pos);
}

void KMixWindow::removeDock()
{
	if (m_dockWidget)
	{
		m_dockWidget->deleteLater();
		m_dockWidget = 0;
	}
}

/**
 * Creates or deletes the KMixDockWidget, depending on whether there is a Mixer instance available.
 *
 * @returns true, if the docking succeeded. Failure usually means that there
 *    was no suitable mixer control selected.
 */
bool KMixWindow::updateDocking()
{
	GlobalConfigData& gcd = GlobalConfig::instance().data;

	if (!gcd.showDockWidget || Mixer::mixers().isEmpty())
	{
		removeDock();
		return false;
	}
	if (!m_dockWidget)
	{
		m_dockWidget = new KMixDockWidget(this);
	}
	return true;
}
void KMixWindow::saveConfig()
{
	saveBaseConfig();
	saveViewConfig();
	saveVolumes();
#ifdef __GNUC_
#warn We must Sync here, or we will lose configuration data. The reson for that is unknown.
#endif

	// TODO cesken The reason for not writing might be that we have multiple cascaded KConfig objects. I must migrate to KSharedConfig !!!
	KGlobal::config()->sync();
	kDebug()
	<< "Saved config ... sync finished";
}

void KMixWindow::saveBaseConfig()
{
	GlobalConfig::instance().writeConfig();

	KConfigGroup config(KGlobal::config(), "Global");

	config.writeEntry("Size", size());
	config.writeEntry("Position", pos());
	// Cannot use isVisible() here, as in the "aboutToQuit()" case this widget is already hidden.
	// (Please note that the problem was only there when quitting via Systray - esken).
	// Using it again, as internal behaviour has changed with KDE4
	config.writeEntry("Visible", isVisible());
	config.writeEntry("Menubar", _actionShowMenubar->isChecked());
	config.writeEntry("Soundmenu.Mixers", GlobalConfig::instance().getMixersForSoundmenu().toList());

	config.writeEntry("DefaultCardOnStart", m_defaultCardOnStart);
	config.writeEntry("ConfigVersion", KMIX_CONFIG_VERSION);
	config.writeEntry("AutoUseMultimediaKeys", m_autouseMultimediaKeys);

	MasterControl& master = Mixer::getGlobalMasterPreferred(false);
	config.writeEntry("MasterMixer", master.getCard());
	config.writeEntry("MasterMixerDevice", master.getControl());

	QString mixerIgnoreExpression = MixerToolBox::instance()->mixerIgnoreExpression();
	config.writeEntry("MixerIgnoreExpression", mixerIgnoreExpression);

	kDebug()
	<< "Base configuration saved";
}

void KMixWindow::saveViewConfig()
{
	QMap<QString, QStringList> mixerViews;

	// The following loop is necessary for the case that the user has hidden all views for a Mixer instance.
	// Otherwise we would not save the Meta information (step -2- below for that mixer.
	// We also do not save dynamic mixers (e.g. PulseAudio)
	foreach ( Mixer* mixer, Mixer::mixers() )
	{
		mixerViews[mixer->id()]; // just insert a map entry
	}

// -1- Save the views themselves
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		QWidget *w = m_wsMixers->widget(i);
		if (w->inherits("KMixerWidget"))
		{
			KMixerWidget* mw = (KMixerWidget*) w;
			// Here also Views are saved. even for Mixers that are closed. This is necessary when unplugging cards.
			// Otherwise the user will be confused afer re-plugging the card (as the config was not saved).
			mw->saveConfig(KGlobal::config().data());
			// add the view to the corresponding mixer list, so we can save a views-per-mixer list below
//			if (!mw->mixer()->isDynamic())
//			{
				QStringList& qsl = mixerViews[mw->mixer()->id()];
				qsl.append(mw->getGuiprof()->getId());
//			}
		}
	}

	// -2- Save Meta-Information (which views, and in which order). views-per-mixer list
	KConfigGroup pconfig(KGlobal::config(), "Profiles");
	QMap<QString, QStringList>::const_iterator itEnd = mixerViews.constEnd();
	for (QMap<QString, QStringList>::const_iterator it = mixerViews.constBegin(); it != itEnd; ++it)
	{
		const QString& mixerProfileKey = it.key(); // this is actually some mixer->id()
		const QStringList& qslProfiles = it.value();
		pconfig.writeEntry(mixerProfileKey, qslProfiles);
		kDebug()
		<< "Save Profile List for " << mixerProfileKey << ", number of views is " << qslProfiles.count();
	}

	kDebug()
	<< "View configuration saved";
}

/**
 * Stores the volumes of all mixers  Can be restored via loadVolumes() or
 * the kmixctrl application.
 */
void KMixWindow::saveVolumes()
{
	saveVolumes(QString());
}

void KMixWindow::saveVolumes(QString postfix)
{
	const QString& kmixctrlRcFilename = getKmixctrlRcFilename(postfix);
	KConfig *cfg = new KConfig(kmixctrlRcFilename);
	for (int i = 0; i < Mixer::mixers().count(); ++i)
	{
		Mixer *mixer = (Mixer::mixers())[i];
		if (mixer->isOpen())
		{ // protect from unplugged devices (better do *not* save them)
			mixer->volumeSave(cfg);
		}
	}
	cfg->sync();
	delete cfg;
	kDebug()
	<< "Volume configuration saved";
}

QString KMixWindow::getKmixctrlRcFilename(QString postfix)
{
	QString kmixctrlRcFilename("kmixctrlrc");
	if (!postfix.isEmpty())
	{
		kmixctrlRcFilename.append(".").append(postfix);
	}
	return kmixctrlRcFilename;
}

void KMixWindow::loadAndInitConfig(bool reset)
{
	if (!reset)
	{
		loadBaseConfig();
	}

	//loadViewConfig(); // mw->loadConfig() explicitly called always after creating mw.
	//loadVolumes(); // not in use

	// create an initial snapshot, so we have a reference of the state before changes through the preferences dialog
	configDataSnapshot = GlobalConfig::instance().data;
}

void KMixWindow::loadBaseConfig()
{
	KConfigGroup config(KGlobal::config(), "Global");

	GlobalConfigData& gcd = GlobalConfig::instance().data;

	QList<QString> preferredMixersInSoundMenu;
	preferredMixersInSoundMenu = config.readEntry("Soundmenu.Mixers", preferredMixersInSoundMenu);
	GlobalConfig::instance().setMixersForSoundmenu(preferredMixersInSoundMenu.toSet());

	m_startVisible = config.readEntry("Visible", false);
	m_multiDriverMode = config.readEntry("MultiDriver", false);
	m_defaultCardOnStart = config.readEntry("DefaultCardOnStart", "");
	m_configVersion = config.readEntry("ConfigVersion", 0);
	// WARNING Don't overwrite m_configVersion with the "correct" value, before having it
	// evaluated. Better only write that in saveBaseConfig()
	m_autouseMultimediaKeys = config.readEntry("AutoUseMultimediaKeys", true);
	QString mixerMasterCard = config.readEntry("MasterMixer", "");
	QString masterDev = config.readEntry("MasterMixerDevice", "");
	Mixer::setGlobalMaster(mixerMasterCard, masterDev, true);
	QString mixerIgnoreExpression = config.readEntry("MixerIgnoreExpression", "Modem");
	MixerToolBox::instance()->setMixerIgnoreExpression(mixerIgnoreExpression);

	// --- Advanced options, without GUI: START -------------------------------------
	QString volumePercentageStepString = config.readEntry("VolumePercentageStep");
	if (!volumePercentageStepString.isNull())
	{
		float volumePercentageStep = volumePercentageStepString.toFloat();
		if (volumePercentageStep > 0 && volumePercentageStep <= 100)
			Volume::VOLUME_STEP_DIVISOR = (100 / volumePercentageStep);
	}

	// --- Advanced options, without GUI: END -------------------------------------

	// The following log is very helpful in bug reports. Please keep it.
	m_backendFilter = config.readEntry<>("Backends", QList<QString>());
	kDebug()
	<< "Backends: " << m_backendFilter;

	// show/hide menu bar
	bool showMenubar = config.readEntry("Menubar", true);

	if (_actionShowMenubar)
		_actionShowMenubar->setChecked(showMenubar);
}

/**
 * Loads the volumes of all mixers from kmixctrlrc.
 * In other words:
 * Restores the default voumes as stored via saveVolumes() or the
 * execution of "kmixctrl --save"
 */

void KMixWindow::loadVolumes()
{
	loadVolumes(QString());
}

void KMixWindow::loadVolumes(QString postfix)
{
	kDebug()
	<< "About to load config (Volume)";
	const QString& kmixctrlRcFilename = getKmixctrlRcFilename(postfix);

	KConfig *cfg = new KConfig(kmixctrlRcFilename);
	for (int i = 0; i < Mixer::mixers().count(); ++i)
	{
		Mixer *mixer = (Mixer::mixers())[i];
		mixer->volumeLoad(cfg);
	}
	delete cfg;
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
	QMap<Mixer*, bool> mixerHasProfile;

// -2a- Build a list of all active profiles in the main window (that means: from all tabs)
	QList<GUIProfile*> activeGuiProfiles;
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		KMixerWidget* kmw = dynamic_cast<KMixerWidget*>(m_wsMixers->widget(i));
		if (kmw)
		{
			activeGuiProfiles.append(kmw->getGuiprof());
		}
	}

	foreach ( GUIProfile* guiprof, activeGuiProfiles)
	{
		Mixer *mixer = Mixer::findMixer( guiprof->getMixerId() );
		if ( mixer == 0 )
		{
			kError() << "MixerToolBox::find() hasn't found the Mixer for the profile " << guiprof->getId();
			continue;
		}
		mixerHasProfile[mixer] = true;

		KMixerWidget* kmw = findKMWforTab(guiprof->getId());
		if ( kmw == 0 )
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
	KConfigGroup pconfig(KGlobal::config(), "Profiles");
	foreach ( Mixer *mixer, Mixer::mixers())
	{
		if ( mixerHasProfile.contains(mixer))
		{
			continue;  // OK, this mixer already has a profile => skip it
		}


		// =========================================================================================
		// No TAB YET => This should mean KMix is just started, or the user has just plugged in a card

		{
			GUIProfile* guiprof = 0;
			if (reset)
			{
				guiprof = GUIProfile::find(mixer, QString("default"), false, true); // ### Card unspecific profile ###
			}

			if ( guiprof != 0 )
			{
				guiprof->setDirty();  // All fallback => dirty
				addMixerWidget(mixer->id(), guiprof->getId(), -1);
				continue;
			}
		}


		// =========================================================================================
		// The trivial cases have not added anything => Look at [Profiles] in config file

		QStringList	profileList = pconfig.readEntry( mixer->id(), QStringList() );
		bool allProfilesRemovedByUser = pconfig.hasKey(mixer->id()) && profileList.isEmpty();
		if (allProfilesRemovedByUser)
		{
			continue; // User has explicitly hidden the views => do no further checks
		}

		{
			bool atLeastOneProfileWasAdded = false;

			foreach ( QString profileId, profileList)
			{
				// This handles the profileList form the kmixrc
				kDebug() << "Now searching for profile: " << profileId;
				GUIProfile* guiprof = GUIProfile::find(mixer, profileId, true, false);// ### Card specific profile ###
				if ( guiprof != 0 )
				{
					addMixerWidget(mixer->id(), guiprof->getId(), -1);
					atLeastOneProfileWasAdded = true;
				}
				else
				{
					kError() << "Cannot load profile " << profileId << " . It was removed by the user, or the KMix config file is defective.";
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
		bool mixerIdMatch = mixerId.isEmpty() || (mixer->id() == mixerId);
		bool thisMixerShouldBeForced = forceNewTab && mixerIdMatch;
		bool we_need_a_fallback = mixerIdMatch && thisMixerShouldBeForced;
		if ( we_need_a_fallback )
		{
			// The profileList was empty or nothing could be loaded
			//     (Hint: This means the user cannot hide a device completely

			// Lets try a bunch of fallback strategies:
			kDebug() << "Attempting to find a card-specific GUI Profile for the mixer " << mixer->id();
			GUIProfile* guiprof = GUIProfile::find(mixer, QString("default"), false, false);// ### Card specific profile ###
			if ( guiprof == 0 )
			{
				kDebug() << "Not found. Attempting to find a generic GUI Profile for the mixer " << mixer->id();
				guiprof = GUIProfile::find(mixer, QString("default"), false, true); // ### Card unspecific profile ###
			}
			if ( guiprof == 0)
			{
				kDebug() << "Using fallback GUI Profile for the mixer " << mixer->id();
				// This means there is neither card specific nor card unspecific profile
				// This is the case for some backends (as they don't ship profiles).
				guiprof = GUIProfile::fallbackProfile(mixer);
			}

			if ( guiprof != 0 )
			{
				guiprof->setDirty();  // All fallback => dirty
				addMixerWidget(mixer->id(), guiprof->getId(), -1);
			}
			else
			{
				kError() << "Cannot use ANY profile (including Fallback) for mixer " << mixer->id() << " . This is impossible, and thus this mixer can NOT be used.";
			}

		}
	}
	mixerHasProfile.clear();

	// -4- FINALIZE **********************************
	if (m_wsMixers->count() > 0)
	{
		if (oldTabPosition >= 0)
		{
			m_wsMixers->setCurrentIndex(oldTabPosition);
		}
		bool dockingSucceded = updateDocking();
		if (!dockingSucceded && !Mixer::mixers().empty())
		{
			show(); // avoid invisible and unaccessible main window
		}
	}
	else
	{
		// No soundcard found. Do not complain, but sit in the background, and wait for newly plugged soundcards.
		updateDocking();  // -<- removes the DockIcon
		hide();
	}

}

KMixerWidget*
KMixWindow::findKMWforTab(const QString& kmwId)
{
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		KMixerWidget* kmw = (KMixerWidget*) m_wsMixers->widget(i);
		if (kmw->getGuiprof()->getId() == kmwId)
		{
			return kmw;
		}
	}
	return 0;
}

void KMixWindow::newView()
{
	if (Mixer::mixers().empty())
	{
		kError() << "Trying to create a View, but no Mixer exists";
		return; // should never happen
	}

	Mixer *mixer = Mixer::mixers()[0];
	QPointer<DialogAddView> dav = new DialogAddView(this, mixer);
	int ret = dav->exec();

	if (QDialog::Accepted == ret)
	{
		QString profileName = dav->getresultViewName();
		QString mixerId = dav->getresultMixerId();
		mixer = Mixer::findMixer(mixerId);
		kDebug()
		<< ">>> mixer = " << mixerId << " -> " << mixer;

		GUIProfile* guiprof = GUIProfile::find(mixer, profileName, false, false);
		if (guiprof == 0)
		{
			guiprof = GUIProfile::find(mixer, profileName, false, true);
		}

		if (guiprof == 0)
		{
			static const QString msg(i18n("Cannot add view - GUIProfile is invalid."));
			errorPopup(msg);
		}
		else
		{
			bool ret = addMixerWidget(mixer->id(), guiprof->getId(), -1);
			if (ret == false)
			{
				errorPopup(i18n("View already exists. Cannot add View."));
			}
		}

		delete dav;
	}

	//kDebug() << "Exit";
}

/**
 * Save the view and close it
 *
 * @arg idx The index in the TabWidget
 */
void KMixWindow::saveAndCloseView(int idx)
{
	kDebug()
	<< "Enter";
	QWidget *w = m_wsMixers->widget(idx);
	KMixerWidget* kmw = ::qobject_cast<KMixerWidget*>(w);
	if (kmw)
	{
		kmw->saveConfig(KGlobal::config().data()); // -<- This alone is not enough, as I need to save the META information as well. Thus use saveViewConfig() below
		m_wsMixers->removeTab(idx);
		updateTabsClosable();
		saveViewConfig();
		delete kmw;
	}
	kDebug()
	<< "Exit";
}

void KMixWindow::fixConfigAfterRead()
{
	KConfigGroup grp(KGlobal::config(), "Global");
	unsigned int configVersion = grp.readEntry("ConfigVersion", 0);
	if (configVersion < 3)
	{
		// Fix the "double Base" bug, by deleting all groups starting with "View.Base.Base.".
		// The group has been copied over by KMixToolBox::loadView() for all soundcards, so
		// we should be fine now
		QStringList cfgGroups = KGlobal::config()->groupList();
		QStringListIterator it(cfgGroups);
		while (it.hasNext())
		{
			QString groupName = it.next();
			if (groupName.indexOf("View.Base.Base") == 0)
			{
				kDebug(67100)
				<< "Fixing group " << groupName;
				KConfigGroup buggyDevgrpCG = KGlobal::config()->group(groupName);
				buggyDevgrpCG.deleteGroup();
			} // remove buggy group
		} // for all groups
	} // if config version < 3
}

void KMixWindow::plugged(const char* driverName, const QString& udi, QString& dev)
{
	kDebug()
	<< "Plugged: dev=" << dev << "(" << driverName << ") udi=" << udi << "\n";
	QString driverNameString;
	driverNameString = driverName;
	int devNum = dev.toInt();
	Mixer *mixer = new Mixer(driverNameString, devNum);
	if (mixer != 0)
	{
		kDebug()
		<< "Plugged: dev=" << dev << "\n";
		if (MixerToolBox::instance()->possiblyAddMixer(mixer))
			recreateGUI(true, mixer->id(), true, false);
	}
}

void KMixWindow::unplugged(const QString& udi)
{
	kDebug()
	<< "Unplugged: udi=" << udi << "\n";
	for (int i = 0; i < Mixer::mixers().count(); ++i)
	{
		Mixer *mixer = (Mixer::mixers())[i];
		//         kDebug(67100) << "Try Match with:" << mixer->udi() << "\n";
		if (mixer->udi() == udi)
		{
			kDebug()
			<< "Unplugged Match: Removing udi=" << udi << "\n";
			//KMixToolBox::notification("MasterFallback", "aaa");
			bool globalMasterMixerDestroyed = (mixer == Mixer::getGlobalMasterMixer());
			// Part 1) Remove Tab
			for (int i = 0; i < m_wsMixers->count(); ++i)
			{
				QWidget *w = m_wsMixers->widget(i);
				KMixerWidget* kmw = ::qobject_cast<KMixerWidget*>(w);
				if (kmw && kmw->mixer() == mixer)
				{
					saveAndCloseView(i);
					i = -1; // Restart loop from scratch (indices are most likely invalidated at removeTab() )
				}
			}
			MixerToolBox::instance()->removeMixer(mixer);
			// Check whether the Global Master disappeared, and select a new one if necessary
			shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
			if (globalMasterMixerDestroyed || md.get() == 0)
			{
				// We don't know what the global master should be now.
				// So lets play stupid, and just select the recommended master of the first device
				if (Mixer::mixers().count() > 0)
				{
					shared_ptr<MixDevice> master = ((Mixer::mixers())[0])->getLocalMasterMD();
					if (master.get() != 0)
					{
						QString localMaster = master->id();
						Mixer::setGlobalMaster(((Mixer::mixers())[0])->id(), localMaster, false);

						QString text;
						text =
								i18n(
										"The soundcard containing the master device was unplugged. Changing to control %1 on card %2.",
										master->readableName(), ((Mixer::mixers())[0])->readableName());
						KMixToolBox::notification("MasterFallback", text);
					}
				}
			}
			if (Mixer::mixers().count() == 0)
			{
				QString text;
				text = i18n("The last soundcard was unplugged.");
				KMixToolBox::notification("MasterFallback", text);
			}
			recreateGUI(true, false);
			break;
		}
	}

}

/**
 * Create a widget with an error message
 * This widget shows an error message like "no mixers detected.
 void KMixWindow::setErrorMixerWidget()
 {
 QString s = i18n("Please plug in your soundcard.No soundcard found. Probably you have not set it up or are missing soundcard drivers. Please check your operating system manual for installing your soundcard."); // !! better text
 m_errorLabel = new QLabel( s,this  );
 m_errorLabel->setAlignment( Qt::AlignCenter );
 m_errorLabel->setWordWrap(true);
 m_errorLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
 m_wsMixers->addTab( m_errorLabel, i18n("No soundcard found") );
 }
 */

/**
 *
 */
bool KMixWindow::profileExists(QString guiProfileId)
{
	for (int i = 0; i < m_wsMixers->count(); ++i)
	{
		KMixerWidget* kmw = dynamic_cast<KMixerWidget*>(m_wsMixers->widget(i));
		if (kmw && kmw->getGuiprof()->getId() == guiProfileId)
			return true;
	}
	return false;
}

bool KMixWindow::addMixerWidget(const QString& mixer_ID, QString guiprofId, int insertPosition)
{
	kDebug()
	<< "Add " << guiprofId;
	GUIProfile* guiprof = GUIProfile::find(guiprofId);
	if (guiprof != 0 && profileExists(guiprof->getId())) // TODO Bad place. Should be checked in the add-tab-dialog
		return false; // already present => don't add again
	Mixer *mixer = Mixer::findMixer(mixer_ID);
	if (mixer == 0)
		return false; // no such Mixer

	//       kDebug(67100) << "KMixWindow::addMixerWidget() " << mixer_ID << " is being added";
	ViewBase::ViewFlags vflags = ViewBase::HasMenuBar;
	if ((_actionShowMenubar == 0) || _actionShowMenubar->isChecked())
		vflags |= ViewBase::MenuBarVisible;
	if (GlobalConfig::instance().data.getToplevelOrientation() == Qt::Vertical)
		vflags |= ViewBase::Horizontal;
	else
		vflags |= ViewBase::Vertical;

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

	kmw->loadConfig(KGlobal::config().data());
	// Now force to read for new tabs, especially after hotplug. Note: Doing it here is bad design and possibly
	// obsolete, as the backend should take care of upating itself.
	kmw->mixer()->readSetFromHWforceUpdate();
	return true;
}

void KMixWindow::updateTabsClosable()
{
	// Pulseaudio runs with 4 fixed tabs - don't allow to close them.
	// Also do not allow to close the last view
	m_wsMixers->setTabsClosable(!Mixer::pulseaudioPresent() && m_wsMixers->count() > 1);
}

bool KMixWindow::queryClose()
{
	GlobalConfigData& gcd = GlobalConfig::instance().data;
	if (gcd.showDockWidget && !kapp->sessionSaving() )
	{
		// Hide (don't close and destroy), if docking is enabled. Except when session saving (shutdown) is in process.
		hide();
		return false;
	}
	else
	{
		// Accept the close, if:
		//     The user has disabled docking
		// or  SessionSaving() is running
		//         kDebug(67100) << "close";
		return true;
	}
}

void KMixWindow::hideOrClose()
{
	GlobalConfigData& gcd = GlobalConfig::instance().data;
	if (gcd.showDockWidget && m_dockWidget != 0)
	{
		// we can hide if there is a dock widget
		hide();
	}
	else
	{
		//  if there is no dock widget, we will quit
		quit();
	}
}

// internal helper to prevent code duplication in slotIncreaseVolume and slotDecreaseVolume
void KMixWindow::increaseOrDecreaseVolume(bool increase)
{
	Mixer* mixer = Mixer::getGlobalMasterMixer(); // only needed for the awkward construct below
	if (mixer == 0)
		return; // e.g. when no soundcard is available
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
	if (md.get() == 0)
		return; // shouldn't happen, but lets play safe

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
	Mixer* mixer = Mixer::getGlobalMasterMixer();
	if (mixer == 0)
		return; // e.g. when no soundcard is available
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
	if (md.get() == 0)
		return; // shouldn't happen, but lets play safe

#ifdef X_KMIX_KF5_BUILD
    if (GlobalConfig::instance().data.showOSD) {
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
#else
    if (GlobalConfig::instance().data.showOSD) {
        // Setting volume not required here anymore, as the OSD updates it by itself
        osdWidget->show();
        osdWidget->activateOSD(); //Enable the hide timer
    }

    //Center the OSD
    QRect rect = KApplication::kApplication()->desktop()->screenGeometry(QCursor::pos());
    QSize size = osdWidget->sizeHint();
    int posX = rect.x() + (rect.width() - size.width()) / 2;
    int posY = rect.y() + 4 * rect.height() / 5;
    osdWidget->setGeometry(posX, posY, size.width(), size.height());
#endif
}

/**
 * Mutes the global master. (SLOT)
 */
void KMixWindow::slotMute()
{
	Mixer* mixer = Mixer::getGlobalMasterMixer();
	if (mixer == 0)
		return; // e.g. when no soundcard is available
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
	if (md.get() == 0)
		return; // shouldn't happen, but lets play safe
	md->toggleMute();
	mixer->commitVolumeChange(md);
	showVolumeDisplay();
}

void KMixWindow::quit()
{
	//     kDebug(67100) << "quit";
	kapp->quit();
}

/**
 * Shows the configuration dialog, with the "general" tab opened.
 */
void KMixWindow::showSettings()
{
	KMixPrefDlg::getInstance()->switchToPage(KMixPrefDlg::PrefGeneral);
	KMixPrefDlg::getInstance()->show();
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
 * the corresponding announcemnts are made.
 */
void KMixWindow::applyPrefs()
{
	// -1- Determine what has changed ------------------------------------------------------------------
	GlobalConfigData& config = GlobalConfig::instance().data;
	GlobalConfigData& configBefore = configDataSnapshot;

	bool labelsHasChanged = config.showLabels ^ configBefore.showLabels;
	bool ticksHasChanged = config.showTicks ^ configBefore.showTicks;

	bool dockwidgetHasChanged = config.showDockWidget ^ configBefore.showDockWidget;

	bool toplevelOrientationHasChanged = config.getToplevelOrientation() != configBefore.getToplevelOrientation();
	bool traypopupOrientationHasChanged = config.getTraypopupOrientation() != configBefore.getTraypopupOrientation();
	kDebug()
	<< "toplevelOrientationHasChanged=" << toplevelOrientationHasChanged << ", config="
			<< config.getToplevelOrientation() << ", configBefore=" << configBefore.getToplevelOrientation();
	kDebug()
	<< "trayOrientationHasChanged=" << traypopupOrientationHasChanged << ", config=" << config.getTraypopupOrientation()
			<< ", configBefore=" << configBefore.getTraypopupOrientation();

	// -2- Determine what effect the changes have ------------------------------------------------------------------

	if (dockwidgetHasChanged || toplevelOrientationHasChanged || traypopupOrientationHasChanged)
	{
		// These might need a complete relayout => announce a ControlList change to rebuild everything
		ControlManager::instance().announce(QString(), ControlChangeType::ControlList, QString("Preferences Dialog"));
	}
	else if (labelsHasChanged || ticksHasChanged)
	{
		ControlManager::instance().announce(QString(), ControlChangeType::GUI, QString("Preferences Dialog"));
	}
	// showOSD does not require any information. It reads on-the-fly from GlobalConfig.

	// -3- Apply all changes ------------------------------------------------------------------

//	this->repaint(); // make KMix look fast (saveConfig() often uses several seconds)
	kapp->processEvents();

	configDataSnapshot = GlobalConfig::instance().data; // create a new snapshot as all current changes are applied now

	// Remove saveConfig() IF aa changes have been migrated to GlobalConfig.
	// Currently there is still stuff like "show menu bar".
	saveConfig();
}

void KMixWindow::toggleMenuBar()
{
	menuBar()->setVisible(_actionShowMenubar->isChecked());
}

void KMixWindow::slotHWInfo()
{
	KMessageBox::information(0, m_hwInfoString, i18n("Mixer Hardware Information"));
}

void KMixWindow::slotKdeAudioSetupExec()
{
	QStringList args;
#ifdef X_KMIX_KF5_BUILD
    args << "kcmshell5"
#else
    args << "kcmshell4"
#endif
         << "kcm_phonon";
	forkExec(args);
}

void KMixWindow::forkExec(const QStringList& args)
{
	int pid = KProcess::startDetached(args);
	if (pid == 0)
	{
		static const QString startErrorMessage(i18n("The helper application is either not installed or not working."));
		QString msg;
		msg += startErrorMessage;
		msg += "\n(";
		msg += args.join(QLatin1String(" "));
		msg += ')';
		errorPopup(msg);
	}

}

void KMixWindow::errorPopup(const QString& msg)
{
	QPointer<KDialog> dialog = new KDialog(this);
	dialog->setButtons(KDialog::Ok);
	dialog->setCaption(i18n("Error"));
	QLabel* qlbl = new QLabel(msg);
	dialog->setMainWidget(qlbl);
	dialog->exec();
	delete dialog;
	kWarning() << msg;
}

void KMixWindow::slotConfigureCurrentView()
{
	KMixerWidget* mw = (KMixerWidget*) m_wsMixers->currentWidget();
	ViewBase* view = 0;
	if (mw)
		view = mw->currentView();
	if (view)
		view->configureView();
}

void KMixWindow::slotSelectMasterClose(QObject*)
{
	m_dsm = 0;
}

void KMixWindow::slotSelectMaster()
{
	Mixer *mixer = Mixer::getGlobalMasterMixer();
	if (mixer != 0)
	{
		if (!m_dsm)
		{
			m_dsm = new DialogSelectMaster(Mixer::getGlobalMasterMixer(), this);
			connect(m_dsm, SIGNAL(destroyed(QObject*)), this, SLOT(slotSelectMasterClose(QObject*)));
			m_dsm->setAttribute(Qt::WA_DeleteOnClose, true);
			m_dsm->show();
		}
		m_dsm->raise();
		m_dsm->activateWindow();
	}
	else
	{
		KMessageBox::error(0, i18n("No sound card is installed or currently plugged in."));
	}
}

void KMixWindow::newMixerShown(int /*tabIndex*/)
{
	KMixerWidget* kmw = (KMixerWidget*) m_wsMixers->currentWidget();
	if (kmw)
	{
		// I am using the app name as a PREFIX, as KMix is a single window app, and it is
		// more helpful to the user to see "KDE Mixer" in a window list than a possibly cryptic
		// soundcard name like "HDA ATI SB"
		setWindowTitle(i18n("KDE Mixer") + " - " + kmw->mixer()->readableName());
		if (!m_dontSetDefaultCardOnStart)
			m_defaultCardOnStart = kmw->getGuiprof()->getId();
		// As switching the tab does NOT mean switching the master card, we do not need to update dock icon here.
		// It would lead to unnecesary flickering of the (complete) dock area.

		// We only show the "Configure Channels..." menu item if the mixer is not dynamic
		ViewBase* view = kmw->currentView();
		QAction* action = actionCollection()->action("toggle_channels_currentview");
		if (view && action)
			action->setVisible(!view->isDynamic());
	}
}

#include "kmix.moc"
