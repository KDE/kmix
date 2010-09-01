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
#include <QProgressBar>
#include <QTimer>
class QWidgetAction;
class KTabWidget;

// KDE
class KAccel;
class KAction;
#include <kxmlguiwindow.h>

// KMix
class GUIProfile;
class ProfTab;
class KMixPrefDlg;
class KMixDockWidget;
class KMixerWidget;
class KMixWindow;
class Mixer;
class ViewDockAreaPopup;
#include "mixer.h"

class OSDWidget;

class
KMixWindow : public KXmlGuiWindow
{
   Q_OBJECT

  public:
   KMixWindow(bool invisible);
   ~KMixWindow();

   bool updateDocking();

  private:
   void saveBaseConfig();
   void saveViewConfig();
   void loadConfig();
   void loadBaseConfig();

   void initPrefDlg();
   void initActions();
   void initActionsLate();
   //void recreateGUI();
   void initWidgets();
   //void setErrorMixerWidget();

   void clearMixerWidgets();
   void fixConfigAfterRead();

   virtual bool queryClose();

  public slots:
   void quit();
   void showSettings();
   void showHelp();
   void showAbout();
   void toggleMenuBar();
   //void loadVolumes();
   void saveVolumes();
   virtual void applyPrefs( KMixPrefDlg *prefDlg );
   void recreateGUI(bool saveView);
   void recreateGUIwithSavingView();
   void recreateGUIwithoutSavingView();
   void redrawMixer( const QString& mixer_ID );


   //void stopVisibilityUpdates();
   //void showEvent( QShowEvent * );
   //void hideEvent( QHideEvent * );

   void newMixerShown(int tabIndex);

    private:
        KMixerWidget* findKMWforTab( QString tabId );

   KAccel *m_keyAccel;
   KAction* _actionShowMenubar;

   bool m_showDockWidget;
   bool m_volumeWidget;
   bool m_showTicks;
   bool m_showLabels;
   bool m_onLogin;
   bool m_startVisible;
//   bool m_isVisible;
   bool m_visibilityUpdateAllowed;
   bool m_multiDriverMode;         // Not officially supported.
   bool m_autouseMultimediaKeys;   // Due to message freeze, not in config dialog in KDE4.4
   Qt::Orientation m_toplevelOrientation;

   KTabWidget *m_wsMixers;
   KMixPrefDlg *m_prefDlg;
   KMixDockWidget *m_dockWidget;
   QString m_hwInfoString;
   QString m_defaultCardOnStart;
   bool m_dontSetDefaultCardOnStart;
   QVBoxLayout *m_widgetsLayout;
   QLabel      *m_errorLabel;
   ViewDockAreaPopup *_dockAreaPopup;
   QWidgetAction *_volWA;
   unsigned int m_configVersion;
   void showVolumeDisplay();
   void increaseOrDecreaseVolume(bool increase);

   OSDWidget* osdWidget;

  private slots:
   void saveConfig();
   void slotHWInfo();
   void slotConfigureCurrentView();
   void slotSelectMaster();
   void addMixerWidget(const QString& mixer_ID, GUIProfile *guiprof, ProfTab *profileTab, int insertPosition);
   void plugged( const char* driverName, const QString& udi, QString& dev);
   void unplugged( const QString& udi);
   void hideOrClose();
   void slotIncreaseVolume();
   void slotDecreaseVolume();
   void slotMute();
};

#endif // KMIX_H
