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
#include <qtabbar.h>

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
#include <kiconloader.h>

// application specific includes
#include "kmix.h"
#include "kmixerwidget.h"
#include "kmixprefdlg.h"
#include "kmixdockwidget.h"

KMixApp::KMixApp()
   : m_dockWidget( 0L )
{
   initActions();
   initMixer();
   initWidgets();
   
   loadConfig();

   initPrefDlg();

   if ( m_showDockWidget && m_startHidden )
      hide();
   
   updateDocking();
}

KMixApp::~KMixApp()
{
   if (m_prefDlg)
      delete m_prefDlg;
}

void KMixApp::initActions()
{
   // file menu
   KStdAction::openNew( this, SLOT(newMixer()), actionCollection());
   KStdAction::close( this, SLOT(closeMixer()), actionCollection());
   (void)new KAction( i18n("&Load volumes"), 0, this, SLOT(loadVolumes()), 
		      actionCollection(), "file_load_volume" );
   (void)new KAction( i18n("&Save volumes"), 0, this, SLOT(saveVolumes()), 
		      actionCollection(), "file_save_volume" );
   KStdAction::quit( this, SLOT(quit()), actionCollection());
      
   // settings menu
   KStdAction::showMenubar( this, SLOT(toggleMenuBar()), actionCollection());
   KStdAction::preferences( this, SLOT(showSettings()), actionCollection());

   // help menu
   /*KStdAction::aboutApp( this, SLOT(showAboutApplication(void)), actionCollection());
   KStdAction::help( this, SLOT(appHelpActivated()), actionCollection()); */

   createGUI( "kmixui.rc" );
}

void KMixApp::initMixer()
{
   kDebugInfo("-> KMixApp::initMixer");
   QTimer *timer = new QTimer( this );
   timer->start( 500 );

   Mixer *mixer = Mixer::getMixer();
   int mixerError = mixer->grab();
   if ( mixerError!=0 )
   {
      KMessageBox::error(0, mixer->errorText(mixerError), i18n("Mixer failure"));
      exit(1);
   }

   connect( timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
   m_mixers.append( mixer );
  
   kDebugInfo("<- KMixApp::initMixer");
}

void KMixApp::initPrefDlg()
{
   m_prefDlg = new KMixPrefDlg;
   connect( m_prefDlg, SIGNAL(signalApplied(KMixPrefDlg *)),
	    this, SLOT(applyPrefs(KMixPrefDlg *)) );
}

void KMixApp::initWidgets()
{
   m_tab = new QTabWidget( this );
   setView( m_tab );
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
  	
      actionCollection()->action("options_configure")->plug( menu );
      menu->insertSeparator();
      actionCollection()->action("help_about_app")->plug( menu );
      actionCollection()->action("help")->plug( menu );
		
      m_dockWidget->show();
   }
}

void KMixApp::saveConfig()
{	
   cerr << "KMixApp::sessionSave()" << endl;

   KConfig *config = kapp->config();
   config->setGroup(0);

   config->writeEntry( "Size", size() );
   config->writeEntry( "Position", pos() );
   config->writeEntry( "Menubar", menuBar()->isVisible() );
   config->writeEntry( "AllowDocking", m_showDockWidget );
   config->writeEntry( "StartHidden", m_startHidden );
   config->writeEntry( "HideOnClose", m_hideOnClose );
   config->writeEntry( "Tickmarks", m_showTicks );
   config->writeEntry( "Labels", m_showLabels );
   config->writeEntry( "Tabs", m_mixerWidgets.count() ); 
   config->writeEntry( "SaveVolumes", m_saveVolumes );
   config->writeEntry( "LoadVolumes", m_loadVolumes );

   int n = 0;   
   for (KMixerWidget *mw=m_mixerWidgets.first(); mw!=0; mw=m_mixerWidgets.next())
   {
      QString grp;
      grp.sprintf( "Widget%i", n );

      config->setGroup( grp );
      config->writeEntry( "Mixer", m_mixers.find( mw->mixer() ) );

      mw->sessionSave( grp, false ); 
      n++;
   }

   if ( m_saveVolumes ) saveVolumes();
}

void KMixApp::loadConfig()
{
   cerr << "KMixApp::sessionLoad()" << endl;

   KConfig *config = kapp->config();
   config->setGroup(0);

   // bar status settings
   bool bViewMenubar = config->readBoolEntry("Menubar", true);
   if (bViewMenubar) 
      menuBar()->show();
   else
      menuBar()->hide();
   	
   QSize defSize = size();
   QSize size = config->readSizeEntry("Size", &defSize );
   if(!size.isEmpty()) resize(size);	

   QPoint defPos = pos();
   QPoint pos = config->readPointEntry("Position", &defPos);
   move(pos);

   m_showDockWidget = config->readBoolEntry("AllowDocking", true);
   m_startHidden = config->readBoolEntry("StartHidden", true);
   m_hideOnClose = config->readBoolEntry("HideOnClose", true);
   m_showTicks = config->readBoolEntry("Tickmarks", false);
   m_showLabels = config->readBoolEntry("Labels", false);
   m_loadVolumes = config->readBoolEntry("LoadCVolumes", true);
   m_saveVolumes = config->readBoolEntry("SaveVolumes", true);

   m_mixerWidgets.clear();
   int num = config->readNumEntry("Tabs", 1);
   for (int n=0; n<num; n++)
   {
      kDebugInfo("load %i", n);
      QString grp;
      grp.sprintf( "Widget%i", n );

      Mixer *mixer = m_mixers.at( config->readNumEntry("Mixer", 0) );
      if (mixer)
      {
	 kDebugInfo("mixer=%x", mixer);
	 KMixerWidget *mw = new KMixerWidget( mixer, this );
	 insertMixerWidget( mw );

	 mw->sessionLoad( grp, false ); 
      }
   }
   
   if ( m_loadVolumes ) loadVolumes();
}

void KMixApp::insertMixerWidget( KMixerWidget *mw )
{
   m_mixerWidgets.append( mw );
   m_tab->addTab( mw, mw->name() );

   mw->setTicks( m_showTicks );
   mw->setLabels( m_showLabels );
}

void KMixApp::removeMixerWidget( KMixerWidget *mw )
{
   m_tab->removePage( mw );
   m_mixerWidgets.remove( mw );
}	

void KMixApp::loadVolumes()
{
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
	 mixer->sessionLoad( false );   
}

void KMixApp::saveVolumes()
{
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
	 mixer->sessionSave( false );
}

bool KMixApp::queryExit()
{
   saveConfig();
   return true;
}

void KMixApp::quit()
{
   saveConfig();   
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
      m_prefDlg->m_showLabels->setChecked( m_showLabels );
      m_prefDlg->m_loadVolumes->setChecked( m_loadVolumes );
      m_prefDlg->m_saveVolumes->setChecked( m_saveVolumes );

      m_prefDlg->show();
   }
}

void KMixApp::closeMixer()
{
   if (m_mixerWidgets.count()<=1)
   {
      KMessageBox::error( this, i18n("There must be at least on mixer. You can't close the last one.") );
   } else
   {
      removeMixerWidget( (KMixerWidget *)m_tab->currentPage() );
   }
}

void KMixApp::newMixer()
{
   KMixerWidget *mw = new KMixerWidget( m_mixers.at(0), this );
   insertMixerWidget( mw );
}

void KMixApp::applyPrefs( KMixPrefDlg *prefDlg )
{
   cerr << "KMixApp::applyPrefs( KMixPrefDlg *prefDlg )" << endl;

   m_showDockWidget = prefDlg->m_dockingChk->isChecked();
   m_startHidden = prefDlg->m_startHiddenChk->isChecked();
   m_hideOnClose = prefDlg->m_hideOnCloseChk->isChecked();
   m_showTicks = prefDlg->m_showTicks->isChecked();
   m_showLabels = prefDlg->m_showLabels->isChecked();
   m_loadVolumes = prefDlg->m_loadVolumes->isChecked();
   m_saveVolumes = prefDlg->m_saveVolumes->isChecked();

   for (KMixerWidget *mw=m_mixerWidgets.first(); mw!=0; mw=m_mixerWidgets.next())
   {
      mw->setTicks( m_showTicks );
      mw->setLabels( m_showLabels );
   }

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

