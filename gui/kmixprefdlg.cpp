/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#include "gui/kmixprefdlg.h"

#include <qbuttongroup.h>
#include <QCheckBox>
#include <QLabel>
#include <qradiobutton.h>
//#include <QtGlobal>

#include <kapplication.h>
#include <KConfig>
#include <KGlobal>
#include <klocale.h>
#include <KStandardDirs>

#include "gui/kmixerwidget.h"
#include "core/GlobalConfig.h"

KMixPrefDlg* KMixPrefDlg::instance = 0;

KMixPrefDlg* KMixPrefDlg::getInstance()
{
	return instance;
}

KMixPrefDlg* KMixPrefDlg::createInstance(QWidget *parent, GlobalConfig& config)
{
	if (instance == 0)
	{
		instance = new KMixPrefDlg(parent, config);
	}
	return instance;

}

KMixPrefDlg::KMixPrefDlg(QWidget *parent, GlobalConfig& config) :
	KConfigDialog(parent, i18n("Configure"), &config), dialogConfig(config)

{
	setFaceType(KPageDialog::List);
	//setCaption(i18n("Configure"));
	setButtons(Ok | Cancel | Apply);

	setDefaultButton(Ok);

	dvc = 0;

	// general buttons
	m_generalTab = new QFrame(this);
	m_controlsTab = new QFrame(this);
	m_startupTab = new QFrame(this);

	createStartupTab();
	createGeneralTab();
	createControlsTab();
	updateWidgets(); // I thought KConfigDialog would call this, but I saw during a gdb session that it does not do so.

	showButtonSeparator(true);

	generalPage = addPage(m_generalTab, i18n("General"), "configure");
	startupPage = addPage(m_startupTab, i18n("Start"), "preferences-system-login");
	soundmenuPage = addPage(m_controlsTab, i18n("Sound Menu"), "audio-volume-high");
}

KMixPrefDlg::~KMixPrefDlg()
{
}

/**
 * Switches to a specific page and shows it.
 * @param page
 */
void KMixPrefDlg::switchToPage(KMixPrefPage page)
{
	switch (page)
	{
	case PrefGeneral:
		setCurrentPage(generalPage);
		break;
	case PrefSoundMenu:
		setCurrentPage(soundmenuPage);
		break;
	case PrefStartup:
		setCurrentPage(startupPage);
		break;
	default:
		kWarning() << "Tried to activated unknown preferences page" << page;
		break;
	}
	show();
}

void KMixPrefDlg::setActiveMixersInDock(QSet<QString>& mixerIds)
{
	replaceBackendsInTab(mixerIds);
}


// --- TABS --------------------------------------------------------------------------------------------------
void KMixPrefDlg::createStartupTab()
{
	QBoxLayout* layoutStartupTab = new QVBoxLayout(m_startupTab);
	layoutStartupTab->setMargin(0);
	layoutStartupTab->setSpacing(KDialog::spacingHint());

	QLabel* label = new QLabel(i18n("Startup"), m_startupTab);
	layoutStartupTab->addWidget(label);

	m_onLogin = new QCheckBox(i18n("Restore volumes on login"), m_startupTab);
	addWidgetToLayout(m_onLogin, layoutStartupTab, 10, i18n("Restore all volume levels and switches."), "startkdeRestore");

	dynamicControlsRestoreWarning = new QLabel(
		i18n("Dynamic controls from Pulseaudio and MPRIS2 will not be restored."), m_startupTab);
	dynamicControlsRestoreWarning->setEnabled(false);
	addWidgetToLayout(dynamicControlsRestoreWarning, layoutStartupTab, 10, "", "");

	allowAutostart = new QCheckBox(i18n("Autostart"), m_startupTab);
	addWidgetToLayout(allowAutostart, layoutStartupTab, 10,
		i18n("Enables the KMix autostart service (kmix_autostart.desktop)"), "AutoStart");


	allowAutostartWarning = new QLabel(
		i18n("Autostart can not be enabled, as the autostart file kmix_autostart.desktop is not installed."),
		m_startupTab);
	addWidgetToLayout(allowAutostartWarning, layoutStartupTab, 10, "", "");
	layoutStartupTab->addStretch();
}

void KMixPrefDlg::createOrientationGroup(const QString& labelSliderOrientation, QGridLayout* orientationLayout, int row, KMixPrefDlgPrefOrientationType prefType)
{
	QButtonGroup* orientationGroup = new QButtonGroup(m_generalTab);
	orientationGroup->setExclusive(true);
	QLabel* qlb = new QLabel(labelSliderOrientation, m_generalTab);

	QRadioButton* qrbHor = new QRadioButton(i18n("&Horizontal"), m_generalTab);
	QRadioButton* qrbVert = new QRadioButton(i18n("&Vertical"), m_generalTab);

	if (prefType == TrayOrientation)
	{
		_rbTraypopupHorizontal = qrbHor;
		_rbTraypopupVertical = qrbVert;
		orientationGroup->setObjectName("Orientation.TrayPopup");
	}
	else
	{
		_rbHorizontal = qrbHor;
		_rbVertical = qrbVert;
		orientationGroup->setObjectName("Orientation");
	}

	// Add both buttons to button group
	orientationGroup->addButton(qrbHor);
	orientationGroup->addButton(qrbVert);
	// Add both buttons and label to layout
	orientationLayout->addWidget(qlb, row, 0);
	orientationLayout->addWidget(qrbHor, row, 1);
	orientationLayout->addWidget(qrbVert, row, 2);

	connect(qrbHor, SIGNAL(toggled(bool)), SLOT(updateButtons()));
	connect(qrbVert, SIGNAL(toggled(bool)), SLOT(updateButtons()));

	connect(this, SIGNAL(applyClicked()), SLOT(kmixConfigHasChangedEmitter()));
	connect(this, SIGNAL(okClicked()), SLOT(kmixConfigHasChangedEmitter()));

//	connect(qrbHor, SIGNAL(toggled(bool)), SLOT(settingsChangedSlot()));
//	connect(qrbVert, SIGNAL(toggled(bool)), SLOT(settingsChangedSlot()));
}

void KMixPrefDlg::createGeneralTab()
{
	QBoxLayout* layout = new QVBoxLayout(m_generalTab);
	layout->setMargin(0);
	layout->setSpacing(KDialog::spacingHint());

	// --- Behavior ---------------------------------------------------------
	QLabel* label = new QLabel(i18n("Behavior"), m_generalTab);
	layout->addWidget(label);

	// [CONFIG]
	m_beepOnVolumeChange = new QCheckBox(i18n("Volume Feedback"), m_generalTab);
	addWidgetToLayout(m_beepOnVolumeChange, layout, 10, "", "VolumeFeedback");

	volumeFeedbackWarning = new QLabel(i18n("Volume feedback is only available for Pulseaudio."), m_generalTab);
	volumeFeedbackWarning->setEnabled(false);
	addWidgetToLayout(volumeFeedbackWarning, layout, 20, "", "");

	// [CONFIG]
	m_volumeOverdrive = new QCheckBox(i18n("Volume Overdrive"), m_generalTab);
	addWidgetToLayout(m_volumeOverdrive, layout, 10, "Raise volume maximum to 150% (PulseAudio only)", "VolumeOverdrive");
	volumeOverdriveWarning = new QLabel(i18n("You must restart KMix for this setting to take effect."), m_generalTab);
	volumeOverdriveWarning->setEnabled(false);
	addWidgetToLayout(volumeOverdriveWarning, layout, 20, "", "");

	// --- Visual ---------------------------------------------------------
	QLabel* label2 = new QLabel(i18n("Visual"), m_generalTab);
	layout->addWidget(label2);

	// [CONFIG]
	m_showTicks = new QCheckBox(i18n("Show &tickmarks"), m_generalTab);
	addWidgetToLayout(m_showTicks, layout, 10, i18n("Enable/disable tickmark scales on the sliders"), "Tickmarks");

	m_showLabels = new QCheckBox(i18n("Show &labels"), m_generalTab);
	addWidgetToLayout(m_showLabels, layout, 10, i18n("Enables/disables description labels above the sliders"),
		"Labels");

	// [CONFIG]
	m_showOSD = new QCheckBox(i18n("Show On Screen Display (&OSD)"), m_generalTab);
	addWidgetToLayout(m_showOSD, layout, 10, "", "showOSD");


	// [CONFIG] Slider orientation (main window)
	QGridLayout* orientationGrid = new QGridLayout();
	layout->addItem(orientationGrid);

	createOrientationGroup(i18n("Slider orientation: "), orientationGrid, 0, KMixPrefDlg::MainOrientation);

	// Slider orientation (tray popup). We use an extra setting
	QBoxLayout* orientation2Layout = new QHBoxLayout();
	layout->addItem(orientation2Layout);
	createOrientationGroup(i18n("Slider orientation (System tray volume control):"), orientationGrid, 1, KMixPrefDlg::TrayOrientation);

	// Push everything above to the top
	layout->addStretch();
}

void KMixPrefDlg::todoRemoveThisMethodAndInitBackendsCorrectly()
{
	replaceBackendsInTab(GlobalConfig::instance().getMixersForSoundmenu());
//	GlobalConfig::instance().getMixersForSoundmenu();
//	QSet<QString> emptyBackendList;
//	replaceBackendsInTab(emptyBackendList);
}

void KMixPrefDlg::createControlsTab()
{
	layoutControlsTab = new QVBoxLayout(m_controlsTab);
	layoutControlsTab->setMargin(0);
	layoutControlsTab->setSpacing(KDialog::spacingHint());
	m_dockingChk = new QCheckBox(i18n("&Dock in system tray"), m_controlsTab);

	addWidgetToLayout(m_dockingChk, layoutControlsTab, 10, i18n("Docks the mixer into the KDE system tray"),
		"AllowDocking");

	todoRemoveThisMethodAndInitBackendsCorrectly();
}



// --- Helper --------------------------------------------------------------------------------------------------

/**
 * Register widget with correct name for KConfigDialog, then add it to the given layout
 *
 * @param widget
 * @param layout
 * @param spacingBefore
 * @param toopTipText
 * @param objectName
 */
void KMixPrefDlg::addWidgetToLayout(QWidget* widget, QBoxLayout* layout, int spacingBefore, QString tooltip, QString kconfigName)
{
	if (!kconfigName.isEmpty())
	{
		// Widget to be registered for KConfig
		widget->setObjectName("kcfg_" + kconfigName);
	}

	if ( !tooltip.isEmpty() )
	{
		widget->setToolTip(tooltip);
	}

	QBoxLayout *l = new QHBoxLayout();
	l->addSpacing(spacingBefore);
	l->addWidget(widget);
	layout->addItem(l);
}

// --- KConfigDialog CUSTOM WIDGET management ------------------------------------------------------------------------


/**
 * Update Widgets from config.
 * <p>
 * Hint: this get internally called by KConfigdialog on initialization and reset.
 */
void KMixPrefDlg::updateWidgets()
{
	kDebug() << "";
	bool toplevelHorizontal = dialogConfig.data.getToplevelOrientation() == Qt::Horizontal;
	_rbHorizontal->setChecked(toplevelHorizontal);
	_rbVertical->setChecked(!toplevelHorizontal);

	bool trayHorizontal = dialogConfig.data.getTraypopupOrientation() == Qt::Horizontal;
	_rbTraypopupHorizontal->setChecked(trayHorizontal);
	_rbTraypopupVertical->setChecked(!trayHorizontal);
}

/**
 * Updates config from the widgets. And emits the signal kmixConfigHasChanged().
 * <p>
 * Hint: this get internally called by KConfigDialog after pressing the OK or Apply button.
 */
void KMixPrefDlg::updateSettings()
{
	Qt::Orientation toplevelOrientation = _rbHorizontal->isChecked() ? Qt::Horizontal : Qt::Vertical;
	kDebug() << "toplevelOrientation" << toplevelOrientation << ", _rbHorizontal->isChecked()" << _rbHorizontal->isChecked();
	dialogConfig.data.setToplevelOrientation(toplevelOrientation);

	Qt::Orientation trayOrientation = _rbTraypopupHorizontal->isChecked() ? Qt::Horizontal : Qt::Vertical;
	kDebug() << "trayOrientation" << trayOrientation << ", _rbTraypopupHorizontal->isChecked()" << _rbTraypopupHorizontal->isChecked();
	dialogConfig.data.setTraypopupOrientation(trayOrientation);

    // Announcing MasterChanged, as the sound menu (aka ViewDockAreaPopup) primarily shows master volume(s).
    // In any case, ViewDockAreaPopup treats MasterChanged and ControlList the same, so it is better to announce
    // the "smaller" change.
    GlobalConfig::instance().setMixersForSoundmenu(dvc->getChosenBackends());
	ControlManager::instance().announce(QString(), ControlChangeType::MasterChanged, QString("Select Backends Dialog"));

//	emit(kmixConfigHasChanged());
}

void KMixPrefDlg::kmixConfigHasChangedEmitter()
{
	emit(kmixConfigHasChanged());
}



/**
 * Returns whether the custom widgets (orientation checkboxes) have changed.
 * <p>
 * Hint: this get internally called by KConfigDialog from updateButtons().
 * @return
 */
bool KMixPrefDlg::hasChanged()
{
	bool changed = ((dialogConfig.data.getToplevelOrientation() == Qt::Vertical) ^ _rbHorizontal->isChecked());
	if (!changed)
	{
		changed = ((dialogConfig.data.getTraypopupOrientation() == Qt::Vertical) ^ _rbTraypopupHorizontal->isChecked());
	}
	kDebug() << "hasChanged=" << changed;

	return changed;
}


void KMixPrefDlg::showEvent(QShowEvent * event)
{
	// -1- Replace widgets ------------------------------------------------------------
	// Hotplug can change mixers or backends => recreate tab
	todoRemoveThisMethodAndInitBackendsCorrectly();

	// -2- Change visibility and enable status (of the new widgets) ----------------------

	// As GUI can change, the warning will only been shown on demand
	dynamicControlsRestoreWarning->setVisible(Mixer::dynamicBackendsPresent());

	// Pulseaudio supports volume feedback. Disable the configuaration option for all other backends
	// and show a warning.
	bool volumeFeebackAvailable = Mixer::pulseaudioPresent();
	volumeFeedbackWarning->setVisible(!volumeFeebackAvailable);
	m_beepOnVolumeChange->setDisabled(!volumeFeebackAvailable);

	bool overdriveAvailable = volumeFeebackAvailable; // "shortcut" for Mixer::pulseaudioPresent() (see above)
	m_volumeOverdrive->setVisible(overdriveAvailable);
	volumeOverdriveWarning->setVisible(overdriveAvailable);

	QString autostartConfigFilename = KGlobal::dirs()->findResource("autostart", QString("kmix_autostart.desktop"));
	kDebug()
	<< "autostartConfigFilename = " << autostartConfigFilename;
	bool autostartFileExists = !autostartConfigFilename.isNull();

	//allowAutostartWarning->setEnabled(autostartFileExists);
	allowAutostartWarning->setVisible(!autostartFileExists);
	allowAutostart->setEnabled(autostartFileExists);

	KDialog::showEvent(event);
}


void KMixPrefDlg::replaceBackendsInTab(const QSet<QString>& backends)
{
	if (dvc != 0)
	{
		layoutControlsTab->removeWidget(dvc);
		delete dvc;
	}

	QSet<QString> backendsFromConfig = GlobalConfig::instance().getMixersForSoundmenu();
	dvc = new DialogChooseBackends(0, backendsFromConfig);
	dvc->show();
	layoutControlsTab->addWidget(dvc);

	// Push everything above to the top
	layoutControlsTab->addStretch();
}




#include "kmixprefdlg.moc"
