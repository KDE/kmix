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

class KMixPrefWidget;
class DialogChooseBackends;

class QBoxLayout;
class QCheckBox;
class QSpinBox;
class QFrame;
class QGridLayout;
class QRadioButton;
class QShowEvent;
class QWidget;

class KMessageWidget;


class KMixPrefDlg: public KConfigDialog
{
	Q_OBJECT

public:
	enum PrefPage
	{
		PageGeneral,
		PageStartup,
		PageVolumeControl
	};

	enum PrefChanged
	{
		ChangedNone = 0x00,
		ChangedAny = 0x01,
		ChangedControls = 0x02,
		ChangedGui = 0x04,
		ChangedMaster = 0x08
	};
	Q_DECLARE_FLAGS(PrefChanges, PrefChanged);

	static KMixPrefDlg *instance(QWidget *parent = nullptr);
	void showAtPage(KMixPrefDlg::PrefPage page);

signals:
	void kmixConfigHasChanged(KMixPrefDlg::PrefChanges changed);

protected:
	void showEvent(QShowEvent *event) override;
	/**
	 * Orientation is not supported by default => implement manually
	 * @Override
	 */
	void updateWidgets() override;
	/**
	 * Orientation is not supported by default => implement manually
	 * @Override
	 */
	void updateSettings() override;

	bool hasChanged() override;

private slots:
	void settingChanged(KMixPrefDlg::PrefChanged changes = KMixPrefDlg::ChangedAny);

private:
	explicit KMixPrefDlg(QWidget *parent = nullptr);
	virtual ~KMixPrefDlg() = default;

	enum KMixPrefDlgPrefOrientationType
	{
		MainOrientation, TrayOrientation
	};

	void createStartupTab();
	void replaceBackendsInTab();
	void createGeneralTab();
	void createControlsTab();

	void addWidgetToLayout(QWidget *widget, QBoxLayout *layout, int spacingBefore, const QString &tooltip);
	void createOrientationGroup(const QString &labelSliderOrientation, QGridLayout *orientationLayout, int row, KMixPrefDlgPrefOrientationType type);
	void setOrientationTooltip(QGridLayout *orientationLayout, int row, const QString &tooltip);

	QFrame *m_generalTab;
	QFrame *m_startupTab;
	QFrame *m_controlsTab;

	KMixPrefDlg::PrefChanges m_controlsChanged;

	QCheckBox *m_dockingChk;
	KMessageWidget *dynamicControlsRestoreWarning;
	QCheckBox *m_showTicks;
	QCheckBox *m_showLabels;
	QCheckBox* m_showOSD;
	QCheckBox *m_onLogin;
	QCheckBox *allowAutostart;
	KMessageWidget *allowAutostartWarning;
	QCheckBox *m_beepOnVolumeChange;
	QCheckBox *m_volumeOverdrive;
	QSpinBox *m_volumeStep;
	KMessageWidget *m_pulseOnlyWarning;
	KMessageWidget *m_restartWarning;

	QBoxLayout *layoutControlsTab;
	QBoxLayout *layoutStartupTab;
	DialogChooseBackends *dvc;

	QRadioButton *_rbVertical;
	QRadioButton *_rbHorizontal;
	QRadioButton *_rbTraypopupVertical;
	QRadioButton *_rbTraypopupHorizontal;

	KPageWidgetItem *m_generalPage;
	KPageWidgetItem *m_soundmenuPage;
	KPageWidgetItem *m_startupPage;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KMixPrefDlg::PrefChanges)


#endif							// KMIXPREFDLG_H
