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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KMIX_H
#define KMIX_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstring.h>
#include <qmap.h>

class QTimer;
class QWidgetStack;

// include files for KDE
#include <kmainwindow.h>

class KAccel;
class KComboBox;
class KMixerWidget;
class KMixerPrefWidget;
class KMixPrefDlg;
class KMixDockWidget;
class KMixWindow;
class Mixer;

#include "mixer.h"


class 
KMixWindow : public KMainWindow
{
   Q_OBJECT

  public:
   KMixWindow();
   ~KMixWindow();

  protected slots:
   void saveConfig();
  protected:
   void loadConfig();

   void initMixer();
   void initPrefDlg();
   void initActions();
   void initWidgets();
   void initMixerWidgets();

   void updateDocking();

	bool queryClose();
   void showEvent( QShowEvent * );
   void hideEvent( QHideEvent * );

  public slots:
   void quit();
   void showSettings();
   void showHelp();
   void showAbout();
   void toggleMenuBar();
   void loadVolumes();
   void saveVolumes();
   virtual void applyPrefs( KMixPrefDlg *prefDlg );
   void updateDockIcon();
   void updateDockTip(Volume vol);
   void stopVisibilityUpdates();
   void slotConfigureKeys();

  private:
   KAccel *m_keyAccel;
   QPopupMenu *m_fileMenu;
   QPopupMenu *m_viewMenu;
   QPopupMenu *m_helpMenu;

   bool m_showDockWidget;
   bool m_volumeWidget;
   bool m_hideOnClose;
   bool m_showTicks;
   bool m_showLabels;
   bool m_startVisible;
   bool m_showMenubar;
   bool m_isVisible;
   bool m_visibilityUpdateAllowed;
   int m_maxId;

   QPtrList<Mixer> m_mixers;
   QPtrList<KMixerWidget> m_mixerWidgets;

   KComboBox *m_cMixer;
   QWidgetStack *m_wsMixers;
   KMixPrefDlg *m_prefDlg;
   KMixDockWidget *m_dockWidget;
   QTimer *timer;	// Timer for reading volume from HW
   QString m_hwInfoString;

   //bool isCategoryUsed(Mixer* mixer, MixDevice::DeviceCategory categoryMask);

  private slots:
   //void removeMixerWidget( KMixerWidget *mw );
   //void updateLayout();
   void dockMute();
   void slotHWInfo();
	void showSelectedMixer( int mixer );
};

#endif // KMIX_H
