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

#include <iostream.h>

// include files for QT
#include <qdir.h>
#include <qapplication.h>
#include <qpopupmenu.h>

// include files for KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kpopmenu.h>
#include <khelpmenu.h>

// application specific includes
#include "kmix.h"
#include "kmixerwidget.h"
#include "kmixprefdlg.h"
#include "kmixdockwidget.h"
#include "version.h"


KMixApp::KMixApp()
   : m_dockWidget( 0L )
{
   initActions();
   initMenuBar();
   initView();
   initPrefDlg();

   sessionLoad( false );

   if ( m_showDockWidget && m_startHidden )
   {
      hide();
   }

   updateDocking();
}

KMixApp::~KMixApp()
{
   if (m_prefDlg)
   {
      delete m_prefDlg;
   }
}

void KMixApp::initActions()
{
   m_actions.Settings = KStdAction::preferences( this, SLOT(showSettings()), this );
   m_actions.Quit = KStdAction::quit( this, SLOT(quit()), this );
   m_actions.ToggleMenuBar = KStdAction::showMenubar( this, SLOT(toggleMenuBar()),	this )	;
   m_actions.Show = new KAction( i18n("Restore"), 0, this, SLOT(show()), this );
   m_actions.Hide = new KAction( i18n("Minimize"), 0, this, SLOT(hide()), this );
   m_actions.About = KStdAction::aboutApp( this, SLOT(showAboutApplication(void)), this );
   m_actions.Help = KStdAction::help( this, SLOT(appHelpActivated()), this );
}

void KMixApp::initMenuBar()
{
   m_fileMenu = new QPopupMenu();
   m_actions.Settings->plug( m_fileMenu );	
   m_fileMenu->insertSeparator();
   m_actions.Quit->plug( m_fileMenu );
  	
   m_viewMenu = new QPopupMenu();
   m_actions.ToggleMenuBar->plug( m_viewMenu );

   QString aboutMsg  = "KMix ";
   aboutMsg += APP_VERSION;
   aboutMsg += i18n(
      "\n"
      "(C) 1997-2000 by Christian Esken (esken@kde.org)\n\n"
      "Sound mixer panel for the KDE Desktop Environment.\n"
      "This program is in the GPL.\n"
      "GUI by Stefan Schimanski (1Stein@gmx.de)\n"
      "SGI Port done by Paul Kendall (paul@orion.co.nz).\n"
      "*BSD fixes by Sebestyen Zoltan (szoli@digo.inf.elte.hu)\n"
      "and Lennart Augustsson (augustss@cs.chalmers.se).\n"
      "ALSA port by Nick Lopez (kimo_sabe@usa.net).\n"
      "HP/UX port by Helge Deller (deller@gmx.de)." );

   m_helpMenu = helpMenu( aboutMsg, false );

   menuBar()->insertItem(i18n("&File"), m_fileMenu);
   menuBar()->insertItem(i18n("&View"), m_viewMenu);
   menuBar()->insertSeparator();
   menuBar()->insertItem(i18n("&Help"), m_helpMenu);	
}

void KMixApp::initView()
{
   Mixer *mixer = Mixer::getMixer();

   int mixerError = mixer->grab();
   if ( mixerError!=0 )
   {
      KMessageBox::error(0, mixer->errorText(mixerError), i18n("Mixer failure"));
      exit(1);
   }

   m_mixers.append( mixer );

   KMixerWidget *mixerWidget = new KMixerWidget( mixer, this );
   m_mixerWidgets.append( mixerWidget );
   setView( mixerWidget );

   connect( mixerWidget, SIGNAL(rightMouseClick()), this, SLOT(showContextMenu()));

   setCaption( mixerWidget->mixerName() );
}

void KMixApp::initPrefDlg()
{
   QWidget *tab = new KMixerPrefWidget( m_mixerWidgets.at(0) );
   m_prefDlg = new KMixPrefDlg;
   m_prefDlg->addTab( tab, i18n("Nixus") );

   connect( m_prefDlg, SIGNAL(signalApplied(KMixPrefDlg *)), this, SLOT(applyPrefs(KMixPrefDlg *)));
}

void KMixApp::updateDocking()
{
   cerr << "updateDocking()" << endl;

   if (m_dockWidget)
   {
      delete m_dockWidget;
      m_dockWidget = 0L;
   }

   if (m_showDockWidget)
   {
      cerr << "displaying dock icon" << endl;
      m_dockWidget = new KMixDockWidget( this );                     	
      m_dockWidget->setPixmap( BarIcon("kmixdocked") );
		
      QPopupMenu *menu = m_dockWidget->contextMenu();		
  	
      m_actions.Settings->plug( menu );
      menu->insertSeparator();
//		m_actions.About->plug( menu );
      m_actions.Help->plug( menu );
		
      m_dockWidget->show();
   }
}

void KMixApp::sessionSave( bool sessionConfig )
{	
   cerr << "KMixApp::sessionSave()" << endl;

   KConfig *config = kapp->config();

   config->setGroup(0);

   config->writeEntry("Size", size());
   config->writeEntry("Position", pos());
   config->writeEntry("Menubar", menuBar()->isVisible());
   config->writeEntry("AllowDocking", m_showDockWidget);
   config->writeEntry("StartHidden", m_startHidden);
   config->writeEntry("HideOnClose", m_hideOnClose);
   config->writeEntry("Tickmarks", m_showTicks);

   m_mixerWidgets.at(0)->sessionSave( sessionConfig );
}

void KMixApp::sessionLoad( bool sessionConfig )
{
   cerr << "KMixApp::sessionLoad()" << endl;

   KConfig *config = kapp->config();

   config->setGroup(0);

   // bar status settings
   bool bViewMenubar = config->readBoolEntry("Menubar", true);
   m_actions.ToggleMenuBar->setChecked( bViewMenubar );
   if(!bViewMenubar)
   {
      menuBar()->hide();
   }
	
   QSize size = config->readSizeEntry("Size");
   if(!size.isEmpty()) resize(size);	

   QPoint pos = config->readPointEntry("Position");
   move(pos); // TODO: check whether pos is stored at all in config

   m_showDockWidget = config->readBoolEntry("AllowDocking", true);
   m_startHidden = config->readBoolEntry("StartHidden", true);
   m_hideOnClose = config->readBoolEntry("HideOnClose", true);
   m_showTicks = config->readBoolEntry("Tickmarks", false);

   m_mixerWidgets.at(0)->sessionLoad( sessionConfig );
}

void KMixApp::saveProperties(KConfig *_cfg)
{
   sessionSave( true );
}


void KMixApp::readProperties(KConfig* _cfg)
{
   sessionSave( true );
}		

bool KMixApp::queryExit()
{
   sessionSave( false );
   return true;
}

void KMixApp::quit()
{
   sessionSave( false );
   kapp->quit();
}

void KMixApp::showSettings()
{
   cerr << "KMixApp::showSettings()" << endl;
   if (!m_prefDlg->isVisible())
   {
      m_prefDlg->m_dockingChk->setChecked( m_showDockWidget );
      m_prefDlg->m_startHiddenChk->setChecked( m_startHidden );
      m_prefDlg->m_hideOnCloseChk->setChecked( m_hideOnClose );
      m_prefDlg->m_showTicks->setChecked( m_showTicks );

      m_prefDlg->show();
   }
}

void KMixApp::applyPrefs( KMixPrefDlg *prefDlg )
{
   cerr << "KMixApp::applyPrefs( KMixPrefDlg *prefDlg )" << endl;

   m_showDockWidget = prefDlg->m_dockingChk->isChecked();
   m_startHidden    = prefDlg->m_startHiddenChk->isChecked();
   m_hideOnClose    = prefDlg->m_hideOnCloseChk->isChecked();

   m_showTicks	= prefDlg->m_showTicks->isChecked();
   m_mixerWidgets.at(0)->setTicks( m_showTicks );

   updateDocking();
}

void KMixApp::toggleMenuBar()
{
   cerr << "KMixApp::toggleMenuBar" << endl;
   if( menuBar()->isVisible() )
      menuBar()->hide();
   else
      menuBar()->show();
}

void KMixApp::showContextMenu()
{
   QPopupMenu *qpm = new QPopupMenu;	

   if ( m_showDockWidget )
   {		
      m_actions.Hide->plug( qpm );	
   }

   m_actions.ToggleMenuBar->plug( qpm );
   qpm->insertSeparator();
   m_actions.Settings->plug( qpm );
   qpm->insertSeparator();
   //	m_actions.About->plug( qpm );
   m_actions.Help->plug( qpm );

   if (qpm)
   {
      QPoint KCMpopup_point = QCursor::pos();
      qpm->popup(KCMpopup_point);
   }
}

