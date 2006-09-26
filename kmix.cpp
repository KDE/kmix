/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
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


// include files for QT
#include <QMap>
//#include <qhbox.h>
#include <QCheckBox>
#include <qradiobutton.h>
#include <qstackedwidget.h>
#include <QLayout>
#include <QToolTip>
#include <qlabel.h>

// include files for KDE
#include <kcombobox.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kmenubar.h>
//#include <klineeditdlg.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kapplication.h>
#include <kstdaction.h>
#include <kmenu.h>
#include <khelpmenu.h>
#include <kdebug.h>

#include <kxmlguifactory.h>
#include <kglobal.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
// application specific includes
#include "mixertoolbox.h"
#include "kmix.h"
#include "kmixerwidget.h"
#include "kmixprefdlg.h"
#include "kmixdockwidget.h"
#include "kmixtoolbox.h"


/*KMixWindowu
 * Constructs a mixer window (KMix main window)
 */
KMixWindow::KMixWindow()
	: KMainWindow(0), m_showTicks( true ),
	m_dockWidget()
{
   setObjectName("KMixWindow");
   m_visibilityUpdateAllowed	= true;
   m_multiDriverMode		= false; // -<- I never-ever want the multi-drivermode to be activated by accident
   m_surroundView		        = false; // -<- Also the experimental surround View (3D)
   m_gridView		        = false; // -<- Also the experimental Grid View
   // As long as we do not know better, we assume to start hidden. We need
   // to initialize this variable here, as we don't trigger a hideEvent().
   m_isVisible = false;
   loadConfig(); // Need to load config before initMixer(), due to "MultiDriver" keyword
kDebug(67100) << "MultiDriver c = " << m_multiDriverMode << endl;

   initActions();
   initWidgets();
   initPrefDlg();
   MixerToolBox::instance()->initMixer(m_multiDriverMode, m_hwInfoString);
   recreateGUI();


   if ( m_startVisible )
   {
      /* Started visible: We should do probably do:
         *   m_isVisible = true;
         * But as a showEvent() is triggered by show() we don't need it.
         */
            show();
   }
   else {
      hide();
   }
   connect( kapp, SIGNAL( aboutToQuit()), SLOT( saveSettings()) );
}


KMixWindow::~KMixWindow()
{
   clearMixerWidgets();
   MixerToolBox::instance()->deinitMixer();
}


void KMixWindow::initActions()
{
	// file menu
	KStdAction::quit( this, SLOT(quit()), actionCollection());

	// settings menu
	KStdAction::showMenubar( this, SLOT(toggleMenuBar()), actionCollection());
	KStdAction::preferences( this, SLOT(showSettings()), actionCollection());
	KStdAction::keyBindings( guiFactory(), SLOT(configureShortcuts()), actionCollection());

	KAction *action = new KAction( i18n( "Hardware &Information" ), actionCollection(), "hwinfo" );
	connect(action, SIGNAL(triggered(bool) ), SLOT( slotHWInfo() ));
	action = new KAction( i18n( "Hide Mixer Window" ), actionCollection(), "hide_kmixwindow" );
	connect(action, SIGNAL(triggered(bool) ), SLOT(hide()));
	action->setShortcut(Qt::Key_Escape);
	createGUI( "kmixui.rc" );
}


void KMixWindow::initPrefDlg()
{
	m_prefDlg = new KMixPrefDlg( this );
	connect( m_prefDlg, SIGNAL(signalApplied(KMixPrefDlg *)),
			this, SLOT(applyPrefs(KMixPrefDlg *)) );
}




void KMixWindow::initWidgets()
{
	// Main widget and layout
	setCentralWidget( new QWidget( this ) );

	// Widgets layout
	m_widgetsLayout = new QVBoxLayout(   centralWidget()   );
	m_widgetsLayout->setObjectName(   "m_widgetsLayout"   );
	m_widgetsLayout->setSpacing(   0   );
	m_widgetsLayout->setMargin (   0   );
	//m_widgetsLayout->setResizeMode(QLayout::Minimum); // works fine

	m_wsMixers = new QStackedWidget( centralWidget() );
        m_widgetsLayout->addWidget(m_wsMixers);

	if ( m_showMenubar )
		menuBar()->show();
	else
		menuBar()->hide();

	m_widgetsLayout->activate();
}


void
KMixWindow::updateDocking()
{
	// delete old dock widget
	if (m_dockWidget)
	{
		delete m_dockWidget;
		m_dockWidget = 0L;
	}

	if ( Mixer::mixers().count() == 0 ) {
		return;
	}
	if (m_showDockWidget)
	{

		// create dock widget
                // !! This should be a View in the future
		m_dockWidget = new KMixDockWidget( Mixer::mixers().first(), this, "mainDockWidget", m_volumeWidget );

		/*
		 * Mail from 31.1.2005: "make sure your features are at least string complete"
		 * Preparation for fixing Bug #55078 - scheduled for KDE3.4.1 .
		 * This text will be plugged into the dock-icon popup menu.
		 */
		QString selectChannel = i18n("Select Channel"); // This text will be used in KDE3.4.1 !!!

		m_dockWidget->show();
	}
}

void
KMixWindow::saveSettings()
{
    saveConfig();
    saveVolumes();
}

void
KMixWindow::saveConfig()
{
    KConfig *config = KGlobal::config();
    config->setGroup(0);

   // make sure we don't start without any UI
   // can happen e.g. when not docked and kmix closed via 'X' button
   bool startVisible = m_isVisible;
   if ( !m_showDockWidget )
       startVisible = true;

   config->writeEntry( "Size", size() );
   config->writeEntry( "Position", pos() );
   // Cannot use isVisible() here, as in the "aboutToQuit()" case this widget is already hidden.
   // (Please note that the problem was only there when quitting via Systray - esken).
   config->writeEntry( "Visible", startVisible );
   config->writeEntry( "Menubar", m_showMenubar );
   config->writeEntry( "AllowDocking", m_showDockWidget );
   config->writeEntry( "TrayVolumeControl", m_volumeWidget );
   config->writeEntry( "Tickmarks", m_showTicks );
   config->writeEntry( "Labels", m_showLabels );
   config->writeEntry( "startkdeRestore", m_onLogin );
   Mixer* mixerMasterCard = Mixer::masterCard();
   if ( mixerMasterCard != 0 ) {
      config->writeEntry( "MasterMixer", mixerMasterCard->id() );
   }
   MixDevice* mdMaster = Mixer::masterCardDevice();
   if ( mdMaster != 0 ) {
      config->writeEntry( "MasterMixerDevice", mdMaster->id() );
   }

   if ( m_toplevelOrientation  == Qt::Vertical )
      config->writeEntry( "Orientation","Vertical" );
   else
      config->writeEntry( "Orientation","Horizontal" );

   // save mixer widgets
   for ( int i=0; i<m_wsMixers->count() ; ++i )
   {
      QWidget *w = m_wsMixers->widget(i);
      if ( w->inherits("KMixerWidget") ) {
         KMixerWidget* mw = (KMixerWidget*)w;
         QString grp (mw->id());
         mw->saveConfig( config, grp );
      }
   }

   config->setGroup(0);
}

void
KMixWindow::loadConfig()
{
    KConfig *config = KGlobal::config();
    config->setGroup(0);

   m_showDockWidget = config->readEntry("AllowDocking", true);
   m_volumeWidget = config->readEntry("TrayVolumeControl", true);
	//hide on close has to stay true for usability. KMixPrefDlg option commented out. nolden
   m_hideOnClose = config->readEntry("HideOnClose", true);
   m_showTicks = config->readEntry("Tickmarks", true);
   m_showLabels = config->readEntry("Labels", true);
   m_onLogin = config->readEntry("startkdeRestore", true );
   m_startVisible = config->readEntry("Visible", true);
   kDebug(67100) << "MultiDriver a = " << m_multiDriverMode << endl;
   m_multiDriverMode = config->readEntry("MultiDriver", false);
   kDebug(67100) << "MultiDriver b = " << m_multiDriverMode << endl;
   m_surroundView    = config->readEntry("Experimental-ViewSurround", false );
   m_gridView    = config->readEntry("Experimental-ViewGrid", false );
   const QString& orientationString = config->readEntry("Orientation", "Horizontal");
   QString mixerMasterCard = config->readEntry( "MasterMixer", "" );
   Mixer::setMasterCard(mixerMasterCard);
   QString masterDev = config->readEntry( "MasterMixerDevice", "" );
   Mixer::setMasterCardDevice(masterDev);


   if ( orientationString == "Vertical" )
       m_toplevelOrientation  = Qt::Vertical;
   else
       m_toplevelOrientation = Qt::Horizontal;

   // show/hide menu bar
   m_showMenubar = config->readEntry("Menubar", true);

   KToggleAction *a = static_cast<KToggleAction*>(actionCollection()->action("options_show_menubar"));
   if (a) a->setChecked( m_showMenubar );

   // restore window size and position
   if ( !kapp->isSessionRestored() ) // done by the session manager otherwise
   {
      QSize defSize( minimumWidth(), height() );
      QSize size = config->readEntry("Size", defSize );
      if(!size.isEmpty()) resize(size);
      
      QPoint defPos = pos();
      QPoint pos = config->readEntry("Position", defPos);
      move(pos);
   }
}



/**
 * Create or recreate the Mixer GUI elements
 */
void KMixWindow::recreateGUI()
{
   initMixerWidgets();
   for (int i=0; i<Mixer::mixers().count(); ++i) {
      Mixer *mixer = (Mixer::mixers())[i];
      addMixerWidget(mixer->id());
   }
   updateDocking();
}


/**
 * Put a "dummy" widget at the bottom of the GUI stack.
 * This widget shows an error message like "no mixers detected.
 */
void KMixWindow::initMixerWidgets()
{
   clearMixerWidgets();
   QString s = i18n("No soundcard found. Probably you have not set it up or are missing soundcard drivers. Please check your operating system manual for installing your soundcard."); // !! better text
   m_errorLabel = new QLabel( s,this  );
   m_errorLabel->setAlignment( Qt::AlignCenter );
   m_errorLabel->setWordWrap(true);
   m_errorLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
   m_wsMixers->addWidget( m_errorLabel );
}

void KMixWindow::clearMixerWidgets()
{
   while ( m_wsMixers->count() != 0 )
   {
      QWidget *mw = m_wsMixers->widget(0);
      m_wsMixers->removeWidget(mw);
      delete mw;
   }
}



void KMixWindow::addMixerWidget(QString mixer_ID)
{
   kDebug(67100) << "KMixWindow::addMixerWidget() " << mixer_ID << endl;

   //m_widgetsLayout->setStretchFactor( m_wsMixers, 10 );

   Mixer *mixer = MixerToolBox::instance()->find(mixer_ID);
   if ( mixer != 0 )
   {
      kDebug(67100) << "KMixWindow::addMixerWidget() " << mixer_ID << " is being added" << endl;
      ViewBase::ViewFlags vflags = ViewBase::HasMenuBar;
      if ( m_showMenubar ) {
            vflags |= ViewBase::MenuBarVisible;
      }
      if (  m_surroundView ) {
            vflags |= ViewBase::Experimental_SurroundView;
      }
      if (  m_gridView ) {
            vflags |= ViewBase::Experimental_GridView;
      }
      if ( m_toplevelOrientation == Qt::Vertical ) {
            vflags |= ViewBase::Vertical;
      }
      else {
            vflags |= ViewBase::Horizontal;
      }
      
      
      KMixerWidget *mw = new KMixerWidget( mixer,
                                          MixDevice::ALL, this, "KMixerWidget", vflags );
      
      // Add to WidgetStack
      /* A newly added mixer will automatically added at the top
      * and thus the window title is also set appropriately */
      m_wsMixers->addWidget( mw );
      m_wsMixers->setCurrentWidget(mw);
      setWindowTitle( mw->mixer()->mixerName() );
      connect(mw, SIGNAL(activateNextlayout()), SLOT(showNextMixer()) );
      
      QString grp(mw->id());
      mw->loadConfig( KGlobal::config(), grp );
      
      mw->setTicks( m_showTicks );
      mw->setLabels( m_showLabels );
   } // given mixer exist really
}



bool
KMixWindow::queryClose ( )
{
    if ( m_showDockWidget && !kapp->sessionSaving() )
    {
        hide();
        return false;
    }
    return true;
}


void
KMixWindow::quit()
{
	kapp->quit();
}


void
KMixWindow::showSettings()
{
   if (!m_prefDlg->isVisible())
   {
      m_prefDlg->m_dockingChk->setChecked( m_showDockWidget );
      m_prefDlg->m_volumeChk->setChecked(m_volumeWidget);
      m_prefDlg->m_showTicks->setChecked( m_showTicks );
      m_prefDlg->m_showLabels->setChecked( m_showLabels );
      m_prefDlg->m_onLogin->setChecked( m_onLogin );
      m_prefDlg->_rbVertical  ->setChecked( m_toplevelOrientation == Qt::Vertical );
      m_prefDlg->_rbHorizontal->setChecked( m_toplevelOrientation == Qt::Horizontal );

      m_prefDlg->show();
   }
}


void
KMixWindow::showHelp()
{
	actionCollection()->action( "help_contents" )->trigger();
}


void
KMixWindow::showAbout()
{
   actionCollection()->action( "help_about_app" )->trigger();
}

/**
 * Loads the volumes of all mixers from kmixctrlrc.
 * In other words:
 * Restores the default voumes as stored via saveVolumes() or the
 * execution of "kmixctrl --save"
 */
/* Currently this is not in use
void
KMixWindow::loadVolumes()
{
    KConfig *cfg = new KConfig( "kmixctrlrc", true );
    for ( int i=0; i<Mixer::mixers().count(); ++i)
    {
        Mixer *mixer = (Mixer::mixers())[i];
        mixer->volumeLoad( cfg );
    }
    delete cfg;
}
*/

/**
 * Stores the volumes of all mixers  Can be restored via loadVolumes() or
 * the kmixctrl application.
 */
void
KMixWindow::saveVolumes()
{
    KConfig *cfg = new KConfig( "kmixctrlrc", false );
    for ( int i=0; i<Mixer::mixers().count(); ++i)
    {
	Mixer *mixer = (Mixer::mixers())[i];
	//kDebug(67100) << "KMixWindow::saveConfig()" << endl;
	mixer->volumeSave( cfg );
    }
    delete cfg;
}


void
KMixWindow::applyPrefs( KMixPrefDlg *prefDlg )
{
   m_showDockWidget = prefDlg->m_dockingChk->isChecked();
   m_volumeWidget = prefDlg->m_volumeChk->isChecked();
   m_showTicks = prefDlg->m_showTicks->isChecked();
   m_showLabels = prefDlg->m_showLabels->isChecked();
   m_onLogin = prefDlg->m_onLogin->isChecked();

   bool toplevelOrientationHasChanged =
        ( prefDlg->_rbVertical->isChecked()   && m_toplevelOrientation == Qt::Horizontal )
     || ( prefDlg->_rbHorizontal->isChecked() && m_toplevelOrientation == Qt::Vertical   );
   if ( toplevelOrientationHasChanged ) {
//      QString msg = i18n("The change of orientation will be adopted on the next start of KMix.");
//      KMessageBox::information(0,msg);
      recreateGUI();
   }
   if ( prefDlg->_rbVertical->isChecked() ) {
      //QString "For a change of language to take place, quit and restart KDiff3.";
      //kDebug(67100) << "KMix should change to Vertical layout\n";
      m_toplevelOrientation = Qt::Vertical;
   }
   else if ( prefDlg->_rbHorizontal->isChecked() ) {
     //kDebug(67100) << "KMix should change to Horizontal layout\n";
     m_toplevelOrientation = Qt::Horizontal;
   }


/********* THE FOLLOWING STUFF IS nOT STRICTLY NECCESARY IF
 toplevelOrientationHasChanged==true . TGhe GUI wa recreated anyhow in that case
 *********************************************************************************/
   //this->setUpdatesEnabled(false);
   updateDocking();

   for ( int i=0; i<m_wsMixers->count() ; ++i )
   {
      QWidget *w = m_wsMixers->widget(i);
      if ( w->inherits("KMixerWidget") ) {
         KMixerWidget* mw = (KMixerWidget*)w;
         mw->setTicks( m_showTicks );
         mw->setLabels( m_showLabels );
         mw->mixer()->readSetFromHWforceUpdate(); // needed, as updateDocking() has reconstructed the DockWidget
      }
   }


   //this->setUpdatesEnabled(false);

   // avoid invisible and unaccessible main window
   if( !m_showDockWidget && !isVisible() )
   {
       show();
   }

   this->repaint(); // make KMix look fast (saveConfig() often uses several seconds)
   kapp->processEvents();
   saveConfig();
}


void
KMixWindow::toggleMenuBar()
{
	m_showMenubar = !m_showMenubar;
	if( m_showMenubar )
	{
		menuBar()->show();
	}
   else
	{
		menuBar()->hide();
	}
}

void
KMixWindow::showEvent( QShowEvent * )
{
    if ( m_visibilityUpdateAllowed )
	m_isVisible = isVisible();
    // !! could possibly start polling now (idea: use someting like ref() and unref() on Mixer instance
}

void
KMixWindow::hideEvent( QHideEvent * )
{
    if ( m_visibilityUpdateAllowed )
    {
	m_isVisible = isVisible();
    }
    // !! could possibly stop polling now (idea: use someting like ref() and unref() on Mixer instance
    //    Update: This is a stupid idea, because now the views are responsible for updating. So it will be done in the Views.
    //    But the dock icon is currently no View, so that must be done first.
}


void
KMixWindow::stopVisibilityUpdates() {
    m_visibilityUpdateAllowed = false;
}

void
KMixWindow::slotHWInfo() {
	KMessageBox::information( 0, m_hwInfoString, i18n("Mixer Hardware Information") );
}



void KMixWindow::showNextMixer() {
   int nextIndex = m_wsMixers->currentIndex() + 1;
   if ( nextIndex >= m_wsMixers->count() ) {
      nextIndex = 0;
   }
   m_wsMixers->setCurrentIndex(nextIndex);
   KMixerWidget* mw = (KMixerWidget*)m_wsMixers->currentWidget();
   Mixer* mixer = mw->mixer();
   setWindowTitle( mixer->mixerName() );
   updateDocking();
}

#include "kmix.moc"
