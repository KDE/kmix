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
#include <qinputdialog.h>

// include files for KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kpopupmenu.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kdebug.h>

// application specific includes
#include "kmix.h"
#include "kmixerwidget.h"
#include "kmixprefdlg.h"
#include "kmixdockwidget.h"


#define MAXDEVICES 2
#define MAXCARDS 4


KMixApp::KMixApp()
   : m_dockWidget( 0L )
{
   initMixer();
   initActions();
   initWidgets();

   loadConfig();

   initPrefDlg();

   if ( m_showDockWidget && m_startHidden )
      hide();

   updateDocking();
}

KMixApp::~KMixApp()
{
}

void KMixApp::initActions()
{
   kdDebug() << "KMixApp::initActions" << endl;

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

   createGUI( "kmixui.rc" );
}

void KMixApp::initMixer()
{
   kdDebug() << "-> KMixApp::initMixer" << endl;
   QTimer *timer = new QTimer( this );
   timer->start( 500 );

   // get maximum values
   KConfig *config= new KConfig("kcmkmixrc", false);
   config->setGroup("Misc"); 
   int maxCards = config->readNumEntry( "maxCards", 2 );
   int maxDevices = config->readNumEntry( "maxDevices", 2 );
   delete config;

   for ( int dev=0; dev<maxDevices; dev++ )
      for ( int card=0; card<maxCards; card++ )
      {
	 Mixer *mixer = Mixer::getMixer( dev, card );
	 int mixerError = mixer->grab();
	 if ( mixerError!=0 )
	 {	    
	    delete mixer;	
	 } else
	 {
	    connect( timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
	    m_mixers.append( mixer );
	 }
      }

   kdDebug() << "<- KMixApp::initMixer" << endl;
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
   kdDebug() << "KMixApp::updateDocking" << endl;

   if (m_dockWidget)
   {
      delete m_dockWidget;
      m_dockWidget = 0L;
   }

   if (m_showDockWidget)
   {
      kdError() << "displaying dock icon" << endl;
      m_dockWidget = new KMixDockWidget( this );                     	
      m_dockWidget->setPixmap( BarIcon("kmixdocked") );
		
      QPopupMenu *menu = m_dockWidget->contextMenu();		
  	
      KAction *a = actionCollection()->action("options_configure");
      if (a) a->plug( menu );

      menu->insertSeparator();

      a = actionCollection()->action("help_about_app");
      if (a) a->plug( menu );

      a= actionCollection()->action("help");
      if (a) a->plug( menu );
		
      m_dockWidget->show();
   }
}

void KMixApp::saveConfig()
{	
   kdDebug() << "KMixApp::sessionSave" << endl;

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

   // save mixer widgets
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

   // save mixers
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
	 mixer->sessionSave( false );
}

void KMixApp::loadConfig()
{
   kdDebug() << "-> KMixApp::sessionLoad" << endl;

   KConfig *config = kapp->config();
   config->setGroup(0);

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
   int tabs = config->readNumEntry("Tabs", 1);

   // load mixer widgets
   m_mixerWidgets.clear();
   for (int n=0; n<tabs; n++)
   {
      QString grp;
      grp.sprintf( "Widget%i", n );
      config->setGroup(grp);

      Mixer *mixer = m_mixers.at( config->readNumEntry("Mixer", 0) );
      if (mixer)
      {
	 kdDebug() << "mixer=" << mixer << endl;
	 KMixerWidget *mw = new KMixerWidget( mixer, false, true, this );
	 mw->sessionLoad( grp, false );
	
	 insertMixerWidget( mw );
      }
   }

   // load mixer setting
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
      mixer->sessionLoad( false );

   kdDebug() << "<- KMixApp::sessionLoad" << endl;
}

void KMixApp::insertMixerWidget( KMixerWidget *mw )
{
   m_mixerWidgets.append( mw );

   KAction *a = actionCollection()->action( "file_close" );
   if ( a )
      a->setEnabled( m_mixerWidgets.count()>1 );

   m_tab->addTab( mw, mw->name() );

   mw->setTicks( m_showTicks );
   mw->setLabels( m_showLabels );
}

void KMixApp::removeMixerWidget( KMixerWidget *mw )
{
   m_tab->removePage( mw );
   m_mixerWidgets.remove( mw );
}	

void KMixApp::closeEvent ( QCloseEvent * e )
{
   if ( m_hideOnClose )
   {
      e->ignore();
      hide();
      return;
   }

   KTMainWindow::closeEvent( e );
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

      m_prefDlg->show();
   }
}

void KMixApp::showHelp()
{
   actionCollection()->action( "help_contents" )->activate();
}

void KMixApp::showAbout()
{
   actionCollection()->action( "help_about_app" )->activate();
}

void KMixApp::closeMixer()
{
   if (m_mixerWidgets.count()<=1) return;
   removeMixerWidget( (KMixerWidget *)m_tab->currentPage() );
}

void KMixApp::newMixer()
{
   QStringList lst;

   int n=1;
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
   {
      QString s;
      s.sprintf("%i. %s", n, mixer->mixerName().ascii());
      lst << s;
      n++;
   }

   bool ok = FALSE;
   QString res = QInputDialog::getItem( i18n("Mixers"), i18n( "Available mixers" ), lst,
					1, TRUE, &ok, this );
   if ( ok )
   {
      Mixer *mixer = m_mixers.at( lst.findIndex( res ) );
      if (!mixer)
      {
	 KMessageBox::sorry( this, i18n("Invalid mixer entered.") );
	 return;
      }
	
      QString name = QInputDialog::getText( i18n("Description"), i18n( "Description" ),
					    mixer->mixerName(), &ok, this );
      if ( ok )
      {
	 KMixerWidget *mw = new KMixerWidget( mixer, false, true, this );
	 mw->setName( name );
	 insertMixerWidget( mw );
      }
   }
}

void KMixApp::applyPrefs( KMixPrefDlg *prefDlg )
{
   cerr << "KMixApp::applyPrefs( KMixPrefDlg *prefDlg )" << endl;

   m_showDockWidget = prefDlg->m_dockingChk->isChecked();
   m_startHidden = prefDlg->m_startHiddenChk->isChecked();
   m_hideOnClose = prefDlg->m_hideOnCloseChk->isChecked();
   m_showTicks = prefDlg->m_showTicks->isChecked();
   m_showLabels = prefDlg->m_showLabels->isChecked();

   for (KMixerWidget *mw=m_mixerWidgets.first(); mw!=0; mw=m_mixerWidgets.next())
   {
      mw->setTicks( m_showTicks );
      mw->setLabels( m_showLabels );
   }

   updateDocking();

   // avoid invisible and unaccessible main window
   if( !m_showDockWidget && !isVisible()/* && m_applets.count()==0*/ )
      show();
}

void KMixApp::toggleMenuBar()
{
   cerr << "KMixApp::toggleMenuBar" << endl;
   if( menuBar()->isVisible() )
      menuBar()->hide();
   else
      menuBar()->show();
}

void KMixApp::toggleVisibility()
{
   if ( isVisible() ) hide(); else show();
}

#include "kmix.moc"
