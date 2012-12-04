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
class KTabWidget;

// KDE
class KAccel;
class KAction;
#include <kxmlguiwindow.h>

// KMix
class KMixPrefDlg;
class KMixDockWidget;
class KMixerWidget;
class KMixWindow;
class Mixer;
#include "core/mixer.h"

class OSDWidget;

class
KMixWindow : public KXmlGuiWindow
{
   Q_OBJECT

  public:
   KMixWindow(bool invisible);
   ~KMixWindow();

  private:
   void saveBaseConfig();
   void saveViewConfig();
   void loadConfig();
   void loadBaseConfig();

   void initPrefDlg();
   void initActions();
   void initActionsLate();
   void initActionsAfterInitMixer();
   //void recreateGUI();
   void initWidgets();
   //void setErrorMixerWidget();

   void setBeepOnVolumeChange(bool beep);
   void fixConfigAfterRead();

   virtual bool queryClose();

  public slots:
   void controlsChange(int changeType);
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
   virtual void applyPrefs( KMixPrefDlg *prefDlg );
   void recreateGUI(bool saveView);
   void recreateGUI(bool saveConfig, const QString& mixerId, bool forceNewTab);
   void recreateGUIwithSavingView();
   void newMixerShown(int tabIndex);

    private:
        KMixerWidget* findKMWforTab( const QString& tabId );

        void forkExec(const QStringList& args);
        void errorPopup(const QString& msg);

   KAccel *m_keyAccel;
   KAction* _actionShowMenubar;

   // move many of the following to a central static configuration object
   // (they come from the KMix config file, so they are really "static".
   bool m_showDockWidget;
   bool trayVolumePopupEnabled;
   
private:
   bool m_onLogin;
   bool allowAutostart;
   bool m_beepOnVolumeChange;
   bool m_startVisible;
   bool m_visibilityUpdateAllowed;
   bool m_multiDriverMode;         // Not officially supported.
   bool m_autouseMultimediaKeys;   // Due to message freeze, not in config dialog in KDE4.4

   bool forceNotifierRebuild;

   KTabWidget *m_wsMixers;

   KMixPrefDlg *m_prefDlg;
   KMixDockWidget *m_dockWidget;
   QString m_hwInfoString;
   QString m_defaultCardOnStart;
   bool m_dontSetDefaultCardOnStart;
   QLabel      *m_errorLabel;
   QList<QString> m_backendFilter;
   unsigned int m_configVersion;
   void showVolumeDisplay();
   void increaseOrDecreaseVolume(bool increase);

   OSDWidget* osdWidget;

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
   void slotSelectMaster();
   void plugged( const char* driverName, const QString& udi, QString& dev);
   void unplugged( const QString& udi);
   void hideOrClose();
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

#endif // KMIX_H
