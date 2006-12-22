/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2003 Sven Leiber <s.leiber@web.de>
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

// include files for QT
#include <qmap.h>
#include <qhbox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qwidgetstack.h>
#include <qlayout.h>
#include <qtooltip.h>

// include files for KDE
#include <kcombobox.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kmenubar.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kapplication.h>
#include <kstdaction.h>
#include <kpanelapplet.h>
#include <kpopupmenu.h>
#include <khelpmenu.h>
#include <kdebug.h>
#include <kaccel.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>
#include <kpopupmenu.h>

// application specific includes
#include "mixertoolbox.h"
#include "kmix.h"
#include "kmixerwidget.h"
#include "kmixprefdlg.h"
#include "kmixdockwidget.h"
#include "kmixtoolbox.h"


/**
 * Constructs a mixer window (KMix main window)
 */
KMixWindow::KMixWindow()
	: KMainWindow(0, 0, 0, 0), m_showTicks( true ),
	m_dockWidget( 0L )
{
	m_visibilityUpdateAllowed	= true;
	m_multiDriverMode		= false; // -<- I never-ever want the multi-drivermode to be activated by accident
	m_surroundView		        = false; // -<- Also the experimental surround View (3D)
	m_gridView		        = false; // -<- Also the experimental Grid View
	// As long as we do not know better, we assume to start hidden. We need
	// to initialize this variable here, as we don't trigger a hideEvent().
	m_isVisible = false;
	m_mixerWidgets.setAutoDelete(true);
	loadConfig(); // Need to load config before initMixer(), due to "MultiDriver" keyword
	MixerToolBox::initMixer(Mixer::mixers(), m_multiDriverMode, m_hwInfoString);
	initActions();
	initWidgets();
	initMixerWidgets();

	initPrefDlg();
	updateDocking();

	if ( m_startVisible )
	{
		 /* Started visible: We should do probably do:
		  *   m_isVisible = true;
		  * But as a showEvent() is triggered by show() we don't need it.
		  */
		 show();
	}
	else
	{
		hide();
	}
	connect( kapp, SIGNAL( aboutToQuit()), SLOT( saveSettings()) );
}


KMixWindow::~KMixWindow()
{
   MixerToolBox::deinitMixer();
}


void
KMixWindow::initActions()
{
	// file menu
	KStdAction::quit( this, SLOT(quit()), actionCollection());

	// settings menu
	KStdAction::showMenubar( this, SLOT(toggleMenuBar()), actionCollection());
	KStdAction::preferences( this, SLOT(showSettings()), actionCollection());
	new KAction( i18n( "Configure &Global Shortcuts..." ), "configure_shortcuts", 0, this,
		SLOT( configureGlobalShortcuts() ), actionCollection(), "settings_global" );
	KStdAction::keyBindings( guiFactory(), SLOT(configureShortcuts()), actionCollection());

	(void) new KAction( i18n( "Hardware &Information" ), 0, this, SLOT( slotHWInfo() ), actionCollection(), "hwinfo" );
	(void) new KAction( i18n( "Hide Mixer Window" ), Key_Escape, this, SLOT(hide()), actionCollection(), "hide_kmixwindow" );

	m_globalAccel = new KGlobalAccel( this );
	m_globalAccel->insert( "Increase volume", i18n( "Increase Volume of Master Channel"), QString::null,
			KShortcut(), KShortcut(), this, SLOT( increaseVolume() ) );
	m_globalAccel->insert( "Decrease volume", i18n( "Decrease Volume of Master Channel"), QString::null,
			KShortcut(), KShortcut(), this, SLOT( decreaseVolume() ) );
	m_globalAccel->insert( "Toggle mute", i18n( "Toggle Mute of Master Channel"), QString::null,
			KShortcut(), KShortcut(), this, SLOT( toggleMuted() ) );
	m_globalAccel->readSettings();
	m_globalAccel->updateConnections();

	createGUI( "kmixui.rc" );
}


void
KMixWindow::initPrefDlg()
{
	m_prefDlg = new KMixPrefDlg( this );
	connect( m_prefDlg, SIGNAL(signalApplied(KMixPrefDlg *)),
			this, SLOT(applyPrefs(KMixPrefDlg *)) );
}


void
KMixWindow::initWidgets()
{
	// Main widget and layout
	setCentralWidget( new QWidget(  this, "qt_central_widget" ) );

	// Widgets layout
	widgetsLayout = new QVBoxLayout(   centralWidget(), 0, 0, "widgetsLayout" );
	widgetsLayout->setResizeMode(QLayout::Minimum); // works fine


	// Mixer widget line
	mixerNameLayout = new QHBox( centralWidget(), "mixerNameLayout" );
        widgetsLayout->setStretchFactor( mixerNameLayout, 0 );
	QSizePolicy qsp( QSizePolicy::Ignored, QSizePolicy::Maximum);
	mixerNameLayout->setSizePolicy(qsp);
	mixerNameLayout->setSpacing(KDialog::spacingHint());
	QLabel *qlbl = new QLabel( i18n("Current mixer:"), mixerNameLayout );
	qlbl->setFixedHeight(qlbl->sizeHint().height());
	m_cMixer = new KComboBox( FALSE, mixerNameLayout, "mixerCombo" );
	m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
	connect( m_cMixer, SIGNAL( activated( int ) ), this, SLOT( showSelectedMixer( int ) ) );
	QToolTip::add( m_cMixer, i18n("Current mixer" ) );

	// Add first layout to widgets
	widgetsLayout->addWidget( mixerNameLayout );

	m_wsMixers = new QWidgetStack( centralWidget(), "MixerWidgetStack" );
        widgetsLayout->setStretchFactor( m_wsMixers, 10 );
	widgetsLayout->addWidget( m_wsMixers );

	if ( m_showMenubar )
		menuBar()->show();
	else
		menuBar()->hide();

	widgetsLayout->activate();
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

	if (m_showDockWidget)
	{

		// create dock widget
                // !! This should be a View in the future
		m_dockWidget = new KMixDockWidget( Mixer::mixers().first(), this, "mainDockWidget", m_volumeWidget );

/* Belongs in KMixDockWidget
		// create RMB menu
		KPopupMenu *menu = m_dockWidget->contextMenu();

		// !! check this
		KAction *a = actionCollection()->action( "dock_mute" );
		if ( a ) a->plug( menu );
*/

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
    KConfig *config = kapp->config();
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
      config->writeEntry( "MasterMixerDevice", mdMaster->getPK() );
   }

   if ( m_valueStyle == MixDeviceWidget::NABSOLUTE )
      config->writeEntry( "ValueStyle", "Absolute");
   else if ( m_valueStyle == MixDeviceWidget::NRELATIVE )
      config->writeEntry( "ValueStyle", "Relative");
   else 
     config->writeEntry( "ValueStyle", "None" );

   if ( m_toplevelOrientation  == Qt::Vertical )
      config->writeEntry( "Orientation","Vertical" );
   else
      config->writeEntry( "Orientation","Horizontal" );

   // save mixer widgets
   for ( KMixerWidget *mw = m_mixerWidgets.first(); mw != 0; mw = m_mixerWidgets.next() )
   {
	if ( mw->mixer()->isOpen() )
	{ // protect from unplugged devices (better do *not* save them)
		QString grp;
		grp.sprintf( "%i", mw->id() );
		mw->saveConfig( config, grp );
	}
   }

   config->setGroup(0);
}

void
KMixWindow::loadConfig()
{
    KConfig *config = kapp->config();
    config->setGroup(0);

   m_showDockWidget = config->readBoolEntry("AllowDocking", true);
   m_volumeWidget = config->readBoolEntry("TrayVolumeControl", true);
	//hide on close has to stay true for usability. KMixPrefDlg option commented out. nolden
   m_hideOnClose = config->readBoolEntry("HideOnClose", true);
   m_showTicks = config->readBoolEntry("Tickmarks", true);
   m_showLabels = config->readBoolEntry("Labels", true);
   const QString& valueStyleString = config->readEntry("ValueStyle", "None");
   m_onLogin = config->readBoolEntry("startkdeRestore", true );
   m_startVisible = config->readBoolEntry("Visible", true);
   m_multiDriverMode = config->readBoolEntry("MultiDriver", false);
   m_surroundView    = config->readBoolEntry("Experimental-ViewSurround", false );
   m_gridView    = config->readBoolEntry("Experimental-ViewGrid", false );
   const QString& orientationString = config->readEntry("Orientation", "Horizontal");
   QString mixerMasterCard = config->readEntry( "MasterMixer", "" );
   Mixer::setMasterCard(mixerMasterCard);
   QString masterDev = config->readEntry( "MasterMixerDevice", "" );
   Mixer::setMasterCardDevice(masterDev);

   if ( valueStyleString == "Absolute" )
       m_valueStyle = MixDeviceWidget::NABSOLUTE;
   else if ( valueStyleString == "Relative" )
       m_valueStyle = MixDeviceWidget::NRELATIVE;
   else 
       m_valueStyle = MixDeviceWidget::NNONE;

   if ( orientationString == "Vertical" )
       m_toplevelOrientation  = Qt::Vertical;
   else
       m_toplevelOrientation = Qt::Horizontal;

   // show/hide menu bar
   m_showMenubar = config->readBoolEntry("Menubar", true);

   KToggleAction *a = static_cast<KToggleAction*>(actionCollection()->action("options_show_menubar"));
   if (a) a->setChecked( m_showMenubar );

   // restore window size and position
   if ( !kapp->isRestored() ) // done by the session manager otherwise
   {
		QSize defSize( minimumWidth(), height() );
		QSize size = config->readSizeEntry("Size", &defSize );
		if(!size.isEmpty()) resize(size);

		QPoint defPos = pos();
		QPoint pos = config->readPointEntry("Position", &defPos);
		move(pos);
	}
}


void
KMixWindow::initMixerWidgets()
{
   m_mixerWidgets.clear();

	int id=0;
	Mixer *mixer;

	// Attention!! If Mixer::mixers() is empty, we behave stupid. We don't show nothing and there
        //             is not even a context menu.
	for ( mixer=Mixer::mixers().first(),id=0; mixer!=0; mixer=Mixer::mixers().next(),id++ )
	{
		//kdDebug(67100) << "Mixer number: " << id << " Name: " << mixer->mixerName() << endl ;
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

	
		KMixerWidget *mw = new KMixerWidget( id, mixer, mixer->mixerName(),
						     MixDevice::ALL, this, "KMixerWidget", vflags );
		m_mixerWidgets.append( mw );

		// Add to Combo and Stack
		m_cMixer->insertItem( mixer->mixerName() );
		m_wsMixers->addWidget( mw, id );

		QString grp;
		grp.sprintf( "%i", mw->id() );
		mw->loadConfig( kapp->config(), grp );

		mw->setTicks( m_showTicks );
		mw->setLabels( m_showLabels );
		mw->setValueStyle ( m_valueStyle );
		// !! I am still not sure whether this works 100% reliably - chris
		mw->show();
	}

	if (id == 1)
	{
		// don't show the Current Mixer bit unless we have multiple mixers
		mixerNameLayout->hide();
	}
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
      m_prefDlg->_rbNone->setChecked( m_valueStyle == MixDeviceWidget::NNONE );
      m_prefDlg->_rbAbsolute->setChecked( m_valueStyle == MixDeviceWidget::NABSOLUTE );
      m_prefDlg->_rbRelative->setChecked( m_valueStyle == MixDeviceWidget::NRELATIVE );

      m_prefDlg->show();
   }
}


void
KMixWindow::showHelp()
{
	actionCollection()->action( "help_contents" )->activate();
}


void
KMixWindow::showAbout()
{
   actionCollection()->action( "help_about_app" )->activate();
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
	for (Mixer *mixer=Mixer::mixers().first(); mixer!=0; mixer=Mixer::mixers().next())
	{
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
    for (Mixer *mixer=Mixer::mixers().first(); mixer!=0; mixer=Mixer::mixers().next()) {
	//kdDebug(67100) << "KMixWindow::saveConfig()" << endl;
	if ( mixer->isOpen() ) { // protect from unplugged devices (better do *not* save them)
		mixer->volumeSave( cfg );
	}
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

   if ( prefDlg->_rbNone->isChecked() ) {
     m_valueStyle = MixDeviceWidget::NNONE;
   } else if ( prefDlg->_rbAbsolute->isChecked() ) {
     m_valueStyle = MixDeviceWidget::NABSOLUTE;
   } else if ( prefDlg->_rbRelative->isChecked() ) {
     m_valueStyle = MixDeviceWidget::NRELATIVE;
   }

   bool toplevelOrientationHasChanged =
        ( prefDlg->_rbVertical->isChecked()   && m_toplevelOrientation == Qt::Horizontal )
     || ( prefDlg->_rbHorizontal->isChecked() && m_toplevelOrientation == Qt::Vertical   );
   if ( toplevelOrientationHasChanged ) {
      QString msg = i18n("The change of orientation will be adopted on the next start of KMix.");
      KMessageBox::information(0,msg);
   }
   if ( prefDlg->_rbVertical->isChecked() ) {
      //QString "For a change of language to take place, quit and restart KDiff3.";
      //kdDebug(67100) << "KMix should change to Vertical layout\n";
      m_toplevelOrientation = Qt::Vertical;
   }
   else if ( prefDlg->_rbHorizontal->isChecked() ) {
     //kdDebug(67100) << "KMix should change to Horizontal layout\n";
     m_toplevelOrientation = Qt::Horizontal;
   }


   this->setUpdatesEnabled(false);
   updateDocking();

   for (KMixerWidget *mw=m_mixerWidgets.first(); mw!=0; mw=m_mixerWidgets.next())
   {
      mw->setTicks( m_showTicks );
      mw->setLabels( m_showLabels );
      mw->setValueStyle ( m_valueStyle );
      mw->mixer()->readSetFromHWforceUpdate(); // needed, as updateDocking() has reconstructed the DockWidget
   }

   this->setUpdatesEnabled(true);

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

void
KMixWindow::showSelectedMixer( int mixer )
{
	m_wsMixers->raiseWidget( mixer );
}

void
KMixWindow::configureGlobalShortcuts()
{
	KKeyDialog::configure( m_globalAccel, 0, false ) ;
        m_globalAccel->writeSettings();
        m_globalAccel->updateConnections();
}

void
KMixWindow::toggleMuted()
{
   Mixer* mixerMaster = Mixer::masterCard();
   if ( mixerMaster != 0 ) {
      MixDevice* md = mixerMaster->masterDevice();
      if ( md != 0 && md->hasMute() ) {
         mixerMaster->toggleMute(md->num());
      }
   }
}

void
KMixWindow::increaseVolume()
{
   Mixer* mixerMaster = Mixer::masterCard();
   if ( mixerMaster != 0 ) {
      MixDevice* md = mixerMaster->masterDevice();
      if ( md != 0 ) {
         mixerMaster->increaseVolume(md->num());
      }
   }
}

void
KMixWindow::decreaseVolume()
{
   Mixer* mixerMaster = Mixer::masterCard();
   if ( mixerMaster != 0 ) {
      MixDevice* md = mixerMaster->masterDevice();
      if ( md != 0 ) {
         mixerMaster->decreaseVolume(md->num());
      }
   }
}

#include "kmix.moc"

