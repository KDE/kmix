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


KMixApp::KMixApp()
   : m_maxId( 0 ), m_dockWidget( 0L )
{
   initMixer();
   initActions();
   initWidgets();

   loadConfig();
   
   // first time setup
   if ( m_mixerWidgets.count()==0 )
   {
      int mixerNum = 0;
      for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
      {
	 KMixerWidget *mw = new KMixerWidget( m_maxId, mixer, 
					      mixer->mixerName(), mixerNum,
                                              false, true, this );         
         mw->setName( mixer->mixerName() );
         insertMixerWidget( mw );

	 m_maxId++;
	 mixerNum++;
      }
   }

   initPrefDlg();

   updateDocking();

   if ( m_startVisible )
       show();
   else
       hide();
}

KMixApp::~KMixApp()
{
    kdDebug() << "-> KMixApp::~KMixApp" << endl;
    saveConfig();
    kdDebug() << "<- KMixApp::~KMixApp" << endl;
}

void KMixApp::initActions()
{
   kdDebug() << "KMixApp::initActions" << endl;

   // file menu
   KStdAction::openNew( this, SLOT(newMixer()), actionCollection());
   KStdAction::close( this, SLOT(closeMixer()), actionCollection());
   (void)new KAction( i18n("&Restore default volumes"), 0, this, SLOT(loadVolumes()),
                      actionCollection(), "file_load_volume" );
   (void)new KAction( i18n("&Save current volumes as default"), 0, this, SLOT(saveVolumes()),
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

      a = actionCollection()->action("help");
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
   config->writeEntry( "Visible", isVisible() );
   config->writeEntry( "Menubar", menuBar()->isVisible() );
   config->writeEntry( "AllowDocking", m_showDockWidget );
   config->writeEntry( "HideOnClose", m_hideOnClose );
   config->writeEntry( "Tickmarks", m_showTicks );
   config->writeEntry( "Labels", m_showLabels );

   // save mixer widgets
   QStringList tabs;
   for (KMixerWidget *mw=m_mixerWidgets.first(); mw!=0; mw=m_mixerWidgets.next())
   {
      QString grp;
      grp.sprintf( "%i", mw->id() );
      tabs << grp;

      config->setGroup( grp );
      config->writeEntry( "Mixer", mw->mixerNum() );
      config->writeEntry( "MixerName", mw->mixerName() );

      mw->saveConfig( config, grp );
   }

   config->setGroup(0);
   config->writeEntry( "Tabs", tabs );
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

   if ( !kapp->isRestored() ) // done by the session manager
   {
       QSize defSize = size();
       QSize size = config->readSizeEntry("Size", &defSize );
       if(!size.isEmpty()) resize(size);

       QPoint defPos = pos();
       QPoint pos = config->readPointEntry("Position", &defPos);
       move(pos);
   }

   m_showDockWidget = config->readBoolEntry("AllowDocking", true);
   m_hideOnClose = config->readBoolEntry("HideOnClose", true);
   m_showTicks = config->readBoolEntry("Tickmarks", false);
   m_showLabels = config->readBoolEntry("Labels", false);
   m_startVisible = config->readBoolEntry("Visible", true);

   QString tabsStr = config->readEntry( "Tabs" );
   QStringList tabs = QStringList::split( ',', tabsStr );

   // load mixer widgets
   m_mixerWidgets.clear();
   for (QStringList::Iterator tab=tabs.begin(); tab!=tabs.end(); ++tab)
   {
      config->setGroup(*tab);
      int id = (*tab).toInt();
      if ( id>=m_maxId ) m_maxId = id+1;

      int mixerNum = config->readNumEntry( "Mixer", -1 );
      QString mixerName = config->readEntry( "MixerName", QString::null );
      Mixer *mixer = 0;
      if ( mixerNum>=0 )
      {
         int m = mixerNum+1;
         for (mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
         {
            if ( mixer->mixerName()==mixerName ) m--;
            if ( m==0 ) break;
         }
      }

      kdDebug() << "mixer=" << mixer << endl;
      KMixerWidget *mw = new KMixerWidget( id, mixer, mixerName, mixerNum, false, true, this );
      mw->loadConfig( config, *tab );
      insertMixerWidget( mw );
   }

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
   mw->show();
}

void KMixApp::removeMixerWidget( KMixerWidget *mw )
{
   m_tab->removePage( mw );
   m_mixerWidgets.remove( mw );

   KAction *a = actionCollection()->action( "file_close" );
   if ( a )
      a->setEnabled( m_mixerWidgets.count()>1 );
}

void KMixApp::closeEvent ( QCloseEvent * e )
{
   if ( m_hideOnClose && m_showDockWidget )
   {
      e->ignore();
      hide();
      return;
   }

   KTMainWindow::closeEvent( e );
}

void KMixApp::quit()
{
   kapp->quit();
}

void KMixApp::showSettings()
{
   cerr << "KMixApp::showSettings()" << endl;
   if (!m_prefDlg->isVisible())
   {
      m_prefDlg->m_dockingChk->setChecked( m_showDockWidget );
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
   QString res = QInputDialog::getItem( i18n("Mixers"),
					i18n( "Available mixers" ), lst,
                                        1, TRUE, &ok, this );
   if ( ok )
   {
      int mixerNum = lst.findIndex( res );
      Mixer *mixer = m_mixers.at( mixerNum );
      if (!mixer)
      {
         KMessageBox::sorry( this, i18n("Invalid mixer entered.") );
         return;
      }

      QString name = QInputDialog::getText( i18n("Description"), i18n( "Description" ),
                                            mixer->mixerName(), &ok, this );
      if ( ok )
      {
         KMixerWidget *mw = new KMixerWidget( m_maxId, mixer, mixer->mixerName(), mixerNum,
                                              false, true, this );
         m_maxId++;
         mw->setName( name );
         insertMixerWidget( mw );
      }
   }
}

void KMixApp::loadVolumes()
{
   KConfig *cfg = new KConfig( "kmixctrlrc", true );
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
      mixer->volumeLoad( cfg );
   delete cfg;
}

void KMixApp::saveVolumes()
{
   KConfig *cfg = new KConfig( "kmixctrlrc", false );
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
      mixer->volumeSave( cfg );
   delete cfg;
}

void KMixApp::applyPrefs( KMixPrefDlg *prefDlg )
{
   cerr << "KMixApp::applyPrefs( KMixPrefDlg *prefDlg )" << endl;

   m_showDockWidget = prefDlg->m_dockingChk->isChecked();
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
   if( !m_showDockWidget && !isVisible() )
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
