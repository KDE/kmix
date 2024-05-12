/*
 * KMix -- KDE's full featured mini mixer
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

#ifndef KMIXWINDOW_H
#define KMIXWINDOW_H

// Qt
#include <qpointer.h>

// KDE
#include <kxmlguiwindow.h>

// KMix
#include "core/ControlManager.h"
#include "gui/kmixprefdlg.h"
#include "kmixapp.h"


class QTabWidget;

class KToggleAction;

class KMixDockWidget;
class KMixerWidget;
class KMixWindow;
class Mixer;
class DialogSelectMaster;


class KMixWindow : public KXmlGuiWindow
{
   Q_OBJECT

public:
   KMixWindow(KMixApp::StartupOptions startupOptions);
   virtual ~KMixWindow();

private:
   void saveBaseConfig();
   void saveViewConfig();
   void loadBaseConfig();

   void initPrefDlg();
   void initActions();
   void initActionsLate();
   void initActionsAfterInitMixer();
   void initWidgets();

   void fixConfigAfterRead();

protected:
    void closeEvent(QCloseEvent *ev) override;

public Q_SLOTS:
   void controlsChange(ControlManager::ChangeType changeType);
   void showSettings();
   void showHelp();
   void showAbout();
   void toggleMenuBar();
   void loadVolumes(const QString &postfix = QString());
   void saveVolumes(const QString &postfix = QString());
   void saveConfig();
   void recreateGUI(bool saveView, bool reset);
   void recreateGUI(bool saveConfig, const QString& mixerId, bool forceNewTab, bool reset);
   void recreateGUIwithSavingView();
   void newMixerShown(int tabIndex);
   void slotSelectMaster();

protected Q_SLOTS:
    void applyPrefs(KMixPrefDlg::PrefChanges changed);

private:
    KMixerWidget* findKMWforTab( const QString& tabId );
    void forkExec(const QStringList& args);
    KToggleAction* _actionShowMenubar;

    bool m_startVisible;
    bool m_autouseMultimediaKeys;   // Due to message freeze, not in config dialog in KDE4.4
    bool m_noDockWidget;

   QTabWidget *m_wsMixers;

   KMixDockWidget *m_dockWidget;
   QPointer<DialogSelectMaster> m_masterSelectDialog;

   QString m_defaultCardOnStart;
   bool m_dontSetDefaultCardOnStart;
   unsigned int m_configVersion;

private:
    void showVolumeDisplay();
    void increaseOrDecreaseVolume(bool increase);

    bool addMixerWidget(const QString& mixer_ID, QString guiprofId, int insertPosition);
    void setInitialSize();
    bool profileExists(QString guiProfileId);
    bool updateDocking();
    void removeDock();
    void updateTabsClosable();
    bool shouldShowDock() const;

private:
    static QString getKmixctrlRcFilename(const QString &postfix);

private Q_SLOTS:
   void slotKdeAudioSetupExec();
   void slotConfigureCurrentView();

   void plugged(const char *driverName, const QString &udi, int dev);
   void unplugged(const QString &udi);

   void slotIncreaseVolume();
   void slotDecreaseVolume();
   void slotMute();

   void newView();
   void saveAndCloseView(int);

   void loadVolumes1() { loadVolumes(QString("1")); }
   void loadVolumes2() { loadVolumes(QString("2")); }
   void loadVolumes3() { loadVolumes(QString("3")); }
   void loadVolumes4() { loadVolumes(QString("4")); }

   void saveVolumes1() { saveVolumes(QString("1")); }
   void saveVolumes2() { saveVolumes(QString("2")); }
   void saveVolumes3() { saveVolumes(QString("3")); }
   void saveVolumes4() { saveVolumes(QString("4")); }
};

#endif							// KMIXWINDOW_H
