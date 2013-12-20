/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#ifndef KMIXPREFDLG_H
#define KMIXPREFDLG_H

#include <kconfigdialog.h>
#include <kdialog.h>

class KMixPrefWidget;

class QBoxLayout;
class QCheckBox;
class QFrame;
#include <QGridLayout>
class QLabel;
class QRadioButton;
class QShowEvent;
class QWidget;

#include "core/GlobalConfig.h"
#include "gui/dialogchoosebackends.h"


class KMixPrefDlg: public KConfigDialog
{
Q_OBJECT

public:
	enum KMixPrefPage
	{
		PrefGeneral, PrefSoundMenu, PrefStartup
	};

	static KMixPrefDlg* createInstance(QWidget *parent, GlobalConfig& config);
	static KMixPrefDlg* getInstance();
	void switchToPage(KMixPrefPage page);

signals:
	void kmixConfigHasChanged();

private slots:
	void kmixConfigHasChangedEmitter();

protected:
	void showEvent(QShowEvent * event);
	/**
	 * Orientation is not supported by default => implement manually
	 * @Override
	 */
	void updateWidgets();
	/**
	 * Orientation is not supported by default => implement manually
	 * @Override
	 */
	void updateSettings();

	bool hasChanged();

private:
	static KMixPrefDlg* instance;

	KMixPrefDlg(QWidget *parent, GlobalConfig& config);
	virtual ~KMixPrefDlg();

	enum KMixPrefDlgPrefOrientationType
	{
		MainOrientation, TrayOrientation
	};

	GlobalConfig& dialogConfig;

	void addWidgetToLayout(QWidget* widget, QBoxLayout* layout, int spacingBefore, QString tooltip, QString kconfigName);

	void createStartupTab();
	void replaceBackendsInTab();
	void createGeneralTab();
	void createControlsTab();
	void createOrientationGroup(const QString& labelSliderOrientation, QGridLayout* orientationLayout, int row, KMixPrefDlgPrefOrientationType type);

	QFrame *m_generalTab;
	QFrame *m_startupTab;
	QFrame *m_controlsTab;

	QCheckBox *m_dockingChk;
	QLabel *dynamicControlsRestoreWarning;
	QCheckBox *m_showTicks;
	QCheckBox *m_showLabels;
	QCheckBox* m_showOSD;
	QCheckBox *m_onLogin;
	QCheckBox *allowAutostart;
	QLabel *allowAutostartWarning;
	QCheckBox *m_beepOnVolumeChange;
	QCheckBox *m_volumeOverdrive;
	QLabel *volumeFeedbackWarning;
	QLabel *volumeOverdriveWarning;

	QBoxLayout *layoutControlsTab;
	DialogChooseBackends* dvc;

	QRadioButton *_rbVertical;
	QRadioButton *_rbHorizontal;
	QRadioButton *_rbTraypopupVertical;
	QRadioButton *_rbTraypopupHorizontal;

	KPageWidgetItem* generalPage;
	KPageWidgetItem* soundmenuPage;
	KPageWidgetItem* startupPage;
};

#endif // KMIXPREFDLG_H
