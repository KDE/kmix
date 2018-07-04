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
#include <qcheckbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qgroupbox.h>

#include <kconfig.h>
#include <klocalizedstring.h>
#include <kmessagewidget.h>

#include "dialogbase.h"
#include "dialogstatesaver.h"

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

	dvc = 0;
	dvcSpacerBelow = 0;

	// general buttons
	m_generalTab = new QFrame(this);
	m_controlsTab = new QFrame(this);
	m_startupTab = new QFrame(this);

	createStartupTab();
	createGeneralTab();
	createControlsTab();
	updateWidgets(); // I thought KConfigDialog would call this, but I saw during a gdb session that it does not do so.

	generalPage = addPage(m_generalTab, i18n("General"), "configure");
	startupPage = addPage(m_startupTab, i18n("Startup"), "preferences-system-login");
	soundmenuPage = addPage(m_controlsTab, i18n("Volume Control"), "audio-volume-high");

	new DialogStateSaver(this);
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
		qCWarning(KMIX_LOG) << "Tried to activated unknown preferences page" << page;
		break;
	}
	show();
}


// --- TABS --------------------------------------------------------------------------------------------------
void KMixPrefDlg::createStartupTab()
{
	layoutStartupTab = new QVBoxLayout(m_startupTab);
	layoutStartupTab->setMargin(0);
	layoutStartupTab->setSpacing(DialogBase::verticalSpacing());

	allowAutostart = new QCheckBox(i18n("Start KMix on desktop startup"), m_startupTab);
	addWidgetToLayout(allowAutostart, layoutStartupTab, 10,
			  i18n("Start KMix automatically when the desktop starts."), "AutoStart");

	allowAutostartWarning = new KMessageWidget(
		i18n("Autostart will not work, because the autostart file kmix_autostart.desktop is missing. Check that KMix is installed correctly."), m_startupTab);
	allowAutostartWarning->setIcon(QIcon::fromTheme("dialog-error"));
	allowAutostartWarning->setMessageType(KMessageWidget::Error);
	allowAutostartWarning->setCloseButtonVisible(false);
	allowAutostartWarning->setWordWrap(true);
	allowAutostartWarning->setVisible(false);
	addWidgetToLayout(allowAutostartWarning, layoutStartupTab, 2, "", "");

	layoutStartupTab->addItem(new QSpacerItem(1, 2*DialogBase::verticalSpacing()));

	m_onLogin = new QCheckBox(i18n("Restore volumes on desktop startup"), m_startupTab);
	addWidgetToLayout(m_onLogin, layoutStartupTab, 10,
			  i18n("Restore all mixer volume levels and switches when the desktop starts."), "startkdeRestore");

	dynamicControlsRestoreWarning = new KMessageWidget(
		i18n("Dynamic controls from PulseAudio and MPRIS2 will not be restored."), m_startupTab);
	dynamicControlsRestoreWarning->setIcon(QIcon::fromTheme("dialog-warning"));
	dynamicControlsRestoreWarning->setMessageType(KMessageWidget::Warning);
	dynamicControlsRestoreWarning->setCloseButtonVisible(false);
	dynamicControlsRestoreWarning->setWordWrap(true);
	dynamicControlsRestoreWarning->setVisible(false);
	addWidgetToLayout(dynamicControlsRestoreWarning, layoutStartupTab, 2, "", "");

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
	//qrbHor->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	orientationGroup->addButton(qrbHor);
	orientationGroup->addButton(qrbVert);
	// Add both buttons and label to layout
	orientationLayout->addWidget(qlb, row, 0, Qt::AlignLeft);
	orientationLayout->addWidget(qrbHor, row, 1, Qt::AlignLeft);
	orientationLayout->addWidget(qrbVert, row, 2, Qt::AlignLeft);
	orientationLayout->addItem(new QSpacerItem(1,1,QSizePolicy::Expanding), row, 3);

	connect(qrbHor, SIGNAL(toggled(bool)), SLOT(updateButtons()));
	connect(qrbVert, SIGNAL(toggled(bool)), SLOT(updateButtons()));

    connect(button(QDialogButtonBox::Apply), SIGNAL(clicked()), SLOT(kmixConfigHasChangedEmitter()));
    connect(button(QDialogButtonBox::Ok), SIGNAL(clicked()), SLOT(kmixConfigHasChangedEmitter()));

//	connect(qrbHor, SIGNAL(toggled(bool)), SLOT(settingsChangedSlot()));
//	connect(qrbVert, SIGNAL(toggled(bool)), SLOT(settingsChangedSlot()));
}

void KMixPrefDlg::createGeneralTab()
{
	QBoxLayout* layout = new QVBoxLayout(m_generalTab);
	layout->setMargin(0);
	layout->setSpacing(DialogBase::verticalSpacing());

	// --- Behavior ---------------------------------------------------------
	QGroupBox *grp = new QGroupBox(i18n("Behavior"), m_generalTab);
	grp->setFlat(true);
	layout->addWidget(grp);

	// [CONFIG]
	m_beepOnVolumeChange = new QCheckBox(i18n("Volume feedback"), m_generalTab);
	addWidgetToLayout(m_beepOnVolumeChange, layout, 10, "", "VolumeFeedback");

	m_volumeOverdrive = new QCheckBox(i18n("Volume overdrive"), m_generalTab);
	addWidgetToLayout(m_volumeOverdrive, layout, 10, i18nc("@info:tooltip", "Raise the maximum volume to 150%"), "VolumeOverdrive");

	volumeFeedbackWarning = new KMessageWidget(
		i18n("Volume feedback and volume overdrive are only available for PulseAudio."), m_generalTab);
	volumeFeedbackWarning->setIcon(QIcon::fromTheme("dialog-warning"));
	volumeFeedbackWarning->setMessageType(KMessageWidget::Warning);
	volumeFeedbackWarning->setCloseButtonVisible(false);
	volumeFeedbackWarning->setWordWrap(true);
	volumeFeedbackWarning->setVisible(false);
	addWidgetToLayout(volumeFeedbackWarning, layout, 2, "", "");

	volumeOverdriveWarning = new KMessageWidget(
		i18n("KMix must be restarted for the Volume Overdrive setting to take effect."), m_generalTab);
	volumeOverdriveWarning->setIcon(QIcon::fromTheme("dialog-information"));
	volumeOverdriveWarning->setMessageType(KMessageWidget::Information);
	volumeOverdriveWarning->setCloseButtonVisible(false);
	volumeOverdriveWarning->setWordWrap(true);
	volumeOverdriveWarning->setVisible(false);
	addWidgetToLayout(volumeOverdriveWarning, layout, 2, "", "");

	// --- Visual ---------------------------------------------------------
	layout->addItem(new QSpacerItem(1, DialogBase::verticalSpacing()));
	grp = new QGroupBox(i18n("Visual"), m_generalTab);
	grp->setFlat(true);
	layout->addWidget(grp);

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
	layout->addItem(new QSpacerItem(1, DialogBase::verticalSpacing()));
	QGridLayout* orientationGrid = new QGridLayout();
	orientationGrid->setHorizontalSpacing(DialogBase::horizontalSpacing());
	layout->addItem(orientationGrid);

	// Slider orientation (main window, and tray popup separately).
	createOrientationGroup(i18n("Slider orientation (main window): "), orientationGrid, 0, KMixPrefDlg::MainOrientation);
	createOrientationGroup(i18n("Slider orientation (system tray popup):"), orientationGrid, 1, KMixPrefDlg::TrayOrientation);

	// Push everything above to the top
	layout->addStretch();
}

void KMixPrefDlg::createControlsTab()
{
	layoutControlsTab = new QVBoxLayout(m_controlsTab);
	layoutControlsTab->setMargin(0);
	layoutControlsTab->setSpacing(DialogBase::verticalSpacing());
	m_dockingChk = new QCheckBox(i18n("Dock in system tray"), m_controlsTab);

	addWidgetToLayout(m_dockingChk, layoutControlsTab, 10, i18n("Dock the mixer into the system tray. Click on it to open the popup volume control."),
		"AllowDocking");

	layoutControlsTab->addItem(new QSpacerItem(1, 2*DialogBase::verticalSpacing()));
	replaceBackendsInTab();
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
	if (dialogConfig.data.debugConfig)
		qCDebug(KMIX_LOG) << "";
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
	if (dialogConfig.data.debugConfig)
		qCDebug(KMIX_LOG) << "toplevelOrientation" << toplevelOrientation << ", _rbHorizontal->isChecked()" << _rbHorizontal->isChecked();
	dialogConfig.data.setToplevelOrientation(toplevelOrientation);

	Qt::Orientation trayOrientation = _rbTraypopupHorizontal->isChecked() ? Qt::Horizontal : Qt::Vertical;
	if (dialogConfig.data.debugConfig)
		qCDebug(KMIX_LOG) << "trayOrientation" << trayOrientation << ", _rbTraypopupHorizontal->isChecked()" << _rbTraypopupHorizontal->isChecked();
	dialogConfig.data.setTraypopupOrientation(trayOrientation);

    // Announcing MasterChanged, as the sound menu (aka ViewDockAreaPopup) primarily shows master volume(s).
    // In any case, ViewDockAreaPopup treats MasterChanged and ControlList the same, so it is better to announce
    // the "smaller" change.
    bool modified = dvc->getAndResetModifyFlag();
    if (modified)
    {
		GlobalConfig::instance().setMixersForSoundmenu(dvc->getChosenBackends());
		ControlManager::instance().announce(QString(), ControlChangeType::MasterChanged, QString("Select Backends Dialog"));
    }
}

void KMixPrefDlg::kmixConfigHasChangedEmitter()
{
	emit(kmixConfigHasChanged());
}



/**
 * Returns whether the custom widgets (orientation checkboxes) has changed.
 * <p>
 * Hint: this get internally called by KConfigDialog from updateButtons().
 * @return
 */
bool KMixPrefDlg::hasChanged()
{
	bool orientationFromConfigIsHor = dialogConfig.data.getToplevelOrientation() == Qt::Horizontal;
	bool orientationFromWidgetIsHor = _rbHorizontal->isChecked();
	if (dialogConfig.data.debugConfig)
		qCDebug(KMIX_LOG) << "Orientation MAIN fromConfig=" << (orientationFromConfigIsHor ? "Hor" : "Vert") << ", fromWidget=" << (orientationFromWidgetIsHor ? "Hor" : "Vert");

	bool changed = orientationFromConfigIsHor ^ orientationFromWidgetIsHor;
	if (!changed)
	{
		bool orientationFromConfigIsHor = dialogConfig.data.getTraypopupOrientation() == Qt::Horizontal;
		orientationFromWidgetIsHor = _rbTraypopupHorizontal->isChecked();
		if (dialogConfig.data.debugConfig)
			qCDebug(KMIX_LOG) << "Orientation TRAY fromConfig=" << (orientationFromConfigIsHor ? "Hor" : "Vert") << ", fromWidget=" << (orientationFromWidgetIsHor ? "Hor" : "Vert");

		changed = orientationFromConfigIsHor ^ orientationFromWidgetIsHor;
	}
	if (!changed)
	{
		changed = dvc->getModifyFlag();
	}

	if (dialogConfig.data.debugConfig)
		qCDebug(KMIX_LOG) << "hasChanged=" << changed;

	return changed;
}


void KMixPrefDlg::showEvent(QShowEvent * event)
{
	// -1- Replace widgets ------------------------------------------------------------
	// Hotplug can change mixers or backends => recreate tab
	replaceBackendsInTab();

	KConfigDialog::showEvent(event);

	// -2- Change visibility and enable status (of the new widgets) ----------------------

	// As GUI can change, the warning will only been shown on demand
	dynamicControlsRestoreWarning->setVisible(Mixer::dynamicBackendsPresent());

	QString autostartConfigFilename =
		QStandardPaths::locate(QStandardPaths::GenericConfigLocation, "/autostart/kmix_autostart.desktop");
	if (dialogConfig.data.debugConfig)
	    qCDebug(KMIX_LOG) << "autostartConfigFilename = " << autostartConfigFilename;
	bool autostartFileExists = !autostartConfigFilename.isEmpty();
	allowAutostartWarning->setVisible(!autostartFileExists);
	allowAutostart->setEnabled(autostartFileExists);

	// Only PulseAudio supports volume feedback and volume overdrive.
	// Disable those configuration options for other backends, and
	// show a warning message.
	const bool pulseAudioAvailable = Mixer::pulseaudioPresent();
	if (!pulseAudioAvailable)
	{
		m_beepOnVolumeChange->setChecked(false);
		m_beepOnVolumeChange->setEnabled(false);
		m_volumeOverdrive->setChecked(false);
		m_volumeOverdrive->setEnabled(false);
		volumeFeedbackWarning->setVisible(true);
		volumeOverdriveWarning->setVisible(false);
	}
	else
	{
		volumeFeedbackWarning->setVisible(false);
		volumeOverdriveWarning->setVisible(true);
	}
}


void KMixPrefDlg::replaceBackendsInTab()
{
	if (dvc != 0)
	{
		layoutControlsTab->removeWidget(dvc);
		delete dvc;
		layoutControlsTab->removeItem(dvcSpacerBelow);
		delete dvcSpacerBelow;
	}

	QSet<QString> backendsFromConfig = GlobalConfig::instance().getMixersForSoundmenu();
	dvc = new DialogChooseBackends(0, backendsFromConfig);
	connect(dvc, SIGNAL(backendsModified()), SLOT(updateButtons()));

	layoutControlsTab->addWidget(dvc);
	dvc->show();

	// Push everything above to the top
//	layoutControlsTab->addStretch();

	dvcSpacerBelow = new QSpacerItem(1,1);
	layoutControlsTab->addItem(dvcSpacerBelow);
}




