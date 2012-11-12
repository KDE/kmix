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

#include <kapplication.h>
#include <KConfig>
#include <KGlobal>
#include <klocale.h>
#include <KStandardDirs>

#include "gui/kmixerwidget.h"

KMixPrefDlg::KMixPrefDlg(QWidget *parent) :
	KDialog(parent)
{
	setCaption(i18n("Configure"));
	setButtons(Ok | Cancel | Apply);
	setDefaultButton(Ok);

	// general buttons
	m_generalTab = new QFrame(this);
	setMainWidget(m_generalTab);

	QBoxLayout *layout = new QVBoxLayout(m_generalTab);
	layout->setMargin(0);
	layout->setSpacing(KDialog::spacingHint());

	// --- Behavior ---------------------------------------------------------
	QLabel *label = new QLabel(i18n("Behavior"), m_generalTab);
	layout->addWidget(label);

	m_dockingChk = new QCheckBox(i18n("&Dock in system tray"), m_generalTab);
	addWidgetToLayout(m_dockingChk, layout, 10, i18n("Docks the mixer into the KDE system tray"));
	connect(m_dockingChk, SIGNAL(stateChanged(int)), SLOT(dockIntoPanelChange(int)) );

	m_volumeChk = new QCheckBox(i18n("Enable system tray &volume control"), m_generalTab);
	addWidgetToLayout(m_volumeChk, layout, 20, i18n("Allows to control the volume from the system tray"));

	m_beepOnVolumeChange = new QCheckBox(i18n("Volume Feedback"), m_generalTab);
	addWidgetToLayout(m_beepOnVolumeChange, layout, 10, "");

	volumeFeedbackWarning = new QLabel(i18n("Volume feedback is only available for Pulseaudio."), m_generalTab);
	volumeFeedbackWarning->setEnabled(false);
	addWidgetToLayout(volumeFeedbackWarning, layout, 10, "");


	// --- Startup ---------------------------------------------------------
	label = new QLabel(i18n("Startup"), m_generalTab);
	layout->addWidget(label);

	m_onLogin = new QCheckBox(i18n("Restore volumes on login"), m_generalTab);
	addWidgetToLayout(m_onLogin, layout, 10, i18n("Restore all volume levels and switches."));

	dynamicControlsRestoreWarning = new QLabel(
		i18n("Dynamic controls from Pulseaudio and MPRIS2 will not be restored."), m_generalTab);
	dynamicControlsRestoreWarning->setEnabled(false);
	addWidgetToLayout(dynamicControlsRestoreWarning, layout, 10, "");

	allowAutostart = new QCheckBox(i18n("Autostart"), m_generalTab);
	addWidgetToLayout(allowAutostart, layout, 10, i18n("Enables the KMix autostart service (kmix_autostart.desktop)"));

	allowAutostartWarning = new QLabel(
		i18n("Autostart can not be enabled, as the autostart file kmix_autostart.desktop is not installed."),
		m_generalTab);
	addWidgetToLayout(allowAutostartWarning, layout, 10, "");


	// --- Visual ---------------------------------------------------------
	label = new QLabel(i18n("Visual"), m_generalTab);
	layout->addWidget(label);


	m_showTicks = new QCheckBox(i18n("Show &tickmarks"), m_generalTab);
	addWidgetToLayout(m_showTicks, layout, 10, i18n("Enable/disable tickmark scales on the sliders"));

	m_showLabels = new QCheckBox(i18n("Show &labels"), m_generalTab);
	addWidgetToLayout(m_showLabels, layout, 10, i18n("Enables/disables description labels above the sliders"));

	m_showOSD = new QCheckBox(i18n("Show On Screen Display (&OSD)"), m_generalTab);
	addWidgetToLayout(m_showOSD, layout, 10, "");

	// Slider orientation (main window)
	QBoxLayout *orientationLayout = new QHBoxLayout();
//	orientationLayout->addSpacing(10);
	layout->addItem(orientationLayout);
	QButtonGroup* orientationGroup = new QButtonGroup(m_generalTab);
	orientationGroup->setExclusive(true);
	QLabel* qlb = new QLabel(i18n("Slider orientation: "), m_generalTab);
	_rbHorizontal = new QRadioButton(i18n("&Horizontal"), m_generalTab);
	_rbVertical = new QRadioButton(i18n("&Vertical"), m_generalTab);
	orientationGroup->addButton(_rbHorizontal);
	orientationGroup->addButton(_rbVertical);

	orientationLayout->addWidget(qlb);
	orientationLayout->addWidget(_rbHorizontal);
	orientationLayout->addWidget(_rbVertical);

	orientationLayout->addStretch();

	// Slider orientation (tray popup). We use an extra setting
	QBoxLayout *orientation2Layout = new QHBoxLayout();
//	orientation2Layout->addSpacing(10);
	layout->addItem(orientation2Layout);
	QButtonGroup* orientation2Group = new QButtonGroup(m_generalTab);
	orientation2Group->setExclusive(true);
	QLabel* qlb2 = new QLabel(i18n("Slider orientation (System tray volume control):"), m_generalTab);
	_rbTraypopupHorizontal = new QRadioButton(i18n("&Horizontal"), m_generalTab);
	_rbTraypopupVertical = new QRadioButton(i18n("&Vertical"), m_generalTab);
	orientation2Group->addButton(_rbTraypopupVertical);
	orientation2Group->addButton(_rbTraypopupHorizontal);

	orientation2Layout->addWidget(qlb2);
	orientation2Layout->addWidget(_rbTraypopupVertical);
	orientation2Layout->addWidget(_rbTraypopupHorizontal);


	layout->addStretch();

	showButtonSeparator(true);

	connect(this, SIGNAL(applyClicked()), SLOT(apply()));
	connect(this, SIGNAL(okClicked()), SLOT(apply()));
}

KMixPrefDlg::~KMixPrefDlg()
{
}

void KMixPrefDlg::addWidgetToLayout(QWidget* widget, QBoxLayout* layout, int spacingBefore, QString toopTipText)
{
	if ( !toopTipText.isEmpty() )
		widget->setToolTip(toopTipText);
	QBoxLayout *l = new QHBoxLayout();
	l->addSpacing(spacingBefore);
	l->addWidget(widget);
	layout->addItem(l);
}


void KMixPrefDlg::showEvent(QShowEvent * event)
{
	// As GUI can change, the warning will only been shown on demand
	dynamicControlsRestoreWarning->setVisible(Mixer::dynamicBackendsPresent());

	// Pulseaudio supports volume feedback. Disable the configuaration option for all other backends
	// and show a warning.
	bool volumeFeebackAvailable = Mixer::pulseaudioPresent();
	volumeFeedbackWarning->setVisible(!volumeFeebackAvailable);
	m_beepOnVolumeChange->setDisabled(!volumeFeebackAvailable);

	/*
	 //  KConfig* autostartConfig = new KConfig("kmix_autostart", KConfig::FullConfig, "autostart");
	 //  kDebug() << "accessMode = " << autostartConfig->accessMode();
	 //  bool autostartFileExists =  (autostartConfig->accessMode() == KConfigBase::NoAccess);
	 */
	QString autostartConfigFilename = KGlobal::dirs()->findResource("autostart", QString("kmix_autostart.desktop"));
	kDebug()
	<< "autostartConfigFilename = " << autostartConfigFilename;
	bool autostartFileExists = !autostartConfigFilename.isNull();

	allowAutostartWarning->setEnabled(autostartFileExists);
	allowAutostartWarning->setVisible(!autostartFileExists);
	allowAutostart->setEnabled(autostartFileExists);

	KDialog::showEvent(event);
}

void KMixPrefDlg::apply()
{
	emit signalApplied(this);
}

void KMixPrefDlg::dockIntoPanelChange(int state)
{
	if (state == Qt::Unchecked)
	{
		m_volumeChk->setDisabled(true);
	}
	else
	{
		m_volumeChk->setEnabled(true);
	}
}

#include "kmixprefdlg.moc"
