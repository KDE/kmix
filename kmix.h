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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef KMIX_H
#define KMIX_H
 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>

// include files for KDE 
#include <kapp.h>
#include <ktmainwindow.h>
#include <kaccel.h>
#include <kaction.h>

#include "mixer.h"

class KMixerWidget;
class KMixerPrefWidget;
class KMixPrefDlg;
class KMixDockWidget;

class KMixApp : public KTMainWindow
{
   Q_OBJECT

  public:
   KMixApp();
   ~KMixApp();

  protected:
   void sessionSave( bool sessionConfig );
   void sessionLoad( bool sessionConfig );

   void initMenuBar();
   void initMixer();
   void initView();
   void initPrefDlg();
   void initActions();

   void updateDocking();	

   virtual bool queryExit();
   virtual void saveProperties(KConfig *_cfg);
   virtual void readProperties(KConfig *_cfg);

  public slots:
   void quit();
   void showSettings();
   void showContextMenu();
   void toggleMenuBar();	
   virtual void applyPrefs( KMixPrefDlg *prefDlg );

  private:
   KAccel *m_keyAccel;
   QPopupMenu *m_fileMenu;
   QPopupMenu *m_viewMenu;
   QPopupMenu *m_helpMenu;

   bool m_showDockWidget;
   bool m_startHidden;
   bool m_hideOnClose;
   bool m_showTicks;

   QList<Mixer> m_mixers;
   QList<KMixerWidget> m_mixerWidgets;

   KMixPrefDlg *m_prefDlg;	
   KMixDockWidget *m_dockWidget;

   struct
   {
	 KAction *Quit;
	 KAction *Settings;
	 KToggleAction *ToggleMenuBar;
	 KAction *Show;
	 KAction *Hide;
	 KAction *Help;
	 KAction *About;
   } m_actions;
};
 
#endif // KMIX_H
