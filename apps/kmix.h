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

#ifndef KMIX_H
#define KMIX_H


#include <config.h>

// Qt
#include <QString>

class QLabel;
#include <qlist.h>
#include <QVBoxLayout>
class QPushButton;
#include <QTimer>
class QTabWidget;

// KDE
class KAccel;
class KToggleAction;
#include <kxmlguiwindow.h>

// KMix
#include "core/GlobalConfig.h"
#include "core/ControlManager.h"

class KMixDockWidget;
class KMixerWidget;
class KMixWindow;
class Mixer;
#include "core/mixer.h"

class DialogSelectMaster;

class
KMixWindow : public KXmlGuiWindow
{
   Q_OBJECT

  public:
   KMixWindow(bool invisible, bool reset);
   ~KMixWindow();

  private:
   void saveBaseConfig();
   void saveViewConfig();
   void loadAndInitConfig(bool reset);
   void loadBaseConfig();

   void initPrefDlg();
   void initActions();
   void initActionsLate();
   void initActionsAfterInitMixer();
   void initWidgets();

   void fixConfigAfterRead();

   bool queryClose() Q_DECL_OVERRIDE;

  public slots:
   void controlsChange(ControlManager::ChangeType changeType);
   void quit();
   void showSettings();
   void showHelp();
   void showAbout();
   void toggleMenuBar();
   void loadVolumes();
   void loadVolumes(QString postfix);
   void saveVolumes();
   void saveVolumes(QString postfix);
   void saveConfig();
   virtual void applyPrefs();
   void recreateGUI(bool saveView, bool reset);
   void recreateGUI(bool saveConfig, const QString& mixerId, bool forceNewTab, bool reset);
   void recreateGUIwithSavingView();
   void newMixerShown(int tabIndex);
   void slotSelectMaster();

    private:
        KMixerWidget* findKMWforTab( const QString& tabId );

        void forkExec(const QStringList& args);

   KAccel *m_keyAccel;
   KToggleAction* _actionShowMenubar;

private:
   /**
    * configSnapshot is used to hold the original state before modifications in the preferences dialog
    */
   GlobalConfigData configDataSnapshot;

   bool m_startVisible;
   bool m_visibilityUpdateAllowed;
   bool m_multiDriverMode;         // Not officially supported.
   bool m_autouseMultimediaKeys;   // Due to message freeze, not in config dialog in KDE4.4

   QTabWidget *m_wsMixers;

   KMixDockWidget *m_dockWidget;
   DialogSelectMaster *m_dsm;

   QString m_hwInfoString;
   QString m_defaultCardOnStart;
   bool m_dontSetDefaultCardOnStart;
   QLabel      *m_errorLabel;
   QList<QString> m_backendFilter;
   unsigned int m_configVersion;
   void showVolumeDisplay();
   void increaseOrDecreaseVolume(bool increase);

   bool addMixerWidget(const QString& mixer_ID, QString guiprofId, int insertPosition);
   void setInitialSize();

    private:
    static QString getKmixctrlRcFilename(QString postfix);
	bool profileExists(QString guiProfileId);
	bool updateDocking();
	void removeDock();
	void updateTabsClosable();

  private slots:
   void slotHWInfo();
   void slotKdeAudioSetupExec();
   void slotConfigureCurrentView();

   void plugged(const char *driverName, const QString &udi, int dev);
   void unplugged(const QString &udi);

   void hideOrClose();
   void slotIncreaseVolume();
   void slotDecreaseVolume();
   void slotMute();
   void slotSelectMasterClose(QObject*);

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

#endif // KMIX_H
