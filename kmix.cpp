/*
 * KMix -- KDE's full featured mini mixer
 *
 * $Id$
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2003 Sven Leiber <s.leiber@web.de>
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

// include files for QT
#include <qmap.h>
#include <qtimer.h>
#include <qcheckbox.h>
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
#include <kkeydialog.h>
#include <kpopupmenu.h>

	// application specific includes
#include "kmix.h"
#include "kmixerwidget.h"
#include "kmixprefdlg.h"
#include "kmixdockwidget.h"


/**
 * Constructs a mixer window (KMix main window)
 */
KMixWindow::KMixWindow()
	: KMainWindow(0, 0, 0 ), m_showTicks( false ), m_maxId( 0 ),
	m_dockWidget( 0L ) 
{
	m_visibilityUpdateAllowed	= true;
	m_multiDriverMode		= false; // -<- I never-ever want the multi-drivermode to be activated by accident
	// As long as we do not know better, we assume to start hidden. We need
	// to initialize this variable here, as we don't trigger a hideEvent().
	m_isVisible = false;
	m_mixerWidgets.setAutoDelete(true);
	loadConfig(); // Need to load config before initMixer(), due to "MultiDriver" keyword
	initMixer();
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
	connect( kapp, SIGNAL( aboutToQuit()), SLOT( saveConfig()));

	//setMaximumSize( QSize( 32767, size().height() ) );  // !!! What is this good for ?!?
}


KMixWindow::~KMixWindow()
{
}


void
KMixWindow::initActions()
{
	// file menu
	(void)new KAction( i18n("&Restore Default Volumes"), 0, this, SLOT(loadVolumes()),
							 actionCollection(), "file_load_volume" );
	(void)new KAction( i18n("&Save Current Volumes as Default"), 0, this, SLOT(saveVolumes()),
							 actionCollection(), "file_save_volume" );
	KStdAction::quit( this, SLOT(quit()), actionCollection());

	// settings menu
	KStdAction::showMenubar( this, SLOT(toggleMenuBar()), actionCollection());
	KStdAction::preferences( this, SLOT(showSettings()), actionCollection());

	KStdAction::keyBindings( this, SLOT( slotConfigureKeys() ), actionCollection() );

	(void)new KToggleAction( i18n( "M&ute" ), 0, this, SLOT( dockMute() ),
				 actionCollection(), "dock_mute" );

	(void) new KAction( i18n( "Hardware &Information" ), 0, this, SLOT( slotHWInfo() ), actionCollection(), "hwinfo" );
	createGUI( "kmixui.rc" );
}

void KMixWindow::slotConfigureKeys()
{
  KKeyDialog::configure( actionCollection(), this );
}

void
KMixWindow::initMixer()
{
	QString tmpstr;
	timer = new QTimer( this );  // timer will be started on show()

	// get maximum values
	KConfig *config= new KConfig("kcmkmixrc", false);
	config->setGroup("Misc");
	int maxDevices = config->readNumEntry( "maxDevices", 2 );
	delete config;

	// poll for mixers
	QMap<QString,int> mixerNums;
	int drvNum = Mixer::getDriverNum();

	int driverWithMixer = -1;
	bool multipleDriversActive = false;

	QString driverInfo = "";
	QString driverInfoUsed = "";
	for( int drv=0; drv<drvNum ; drv++ ) 
	{
		QString driverName = Mixer::driverName(drv);
		if ( drv!= 0 ) 
		{
			driverInfo += " + ";
		}
		driverInfo += driverName;
	}

	/* Run a loop over all drivers. The loop will terminate after the first driver which
	   has mixers. And here is the reason:
	   - If you run ALSA with ALSA-OSS-Emulation enabled, mixers will show up twice: once
	     as native ALSA mixer, once as OSS mixer (emulated by ALSA). This is bad and WILL
	     confuse users. So it is a design decision that we can compile in multiple drivers
	     but we can run only one driver.
	   - For special usage scenarios, people will still want to run both drivers at the
	     same time. We allow them to hack their Config-File, where they can enable a
	     multi-driver mode.
	   - Another remark: For KMix3.0 or so, we should allow multiple-driver, for allowing
	     addition of special-use drivers, e.g. an ARTS-mixer-driver, or a CD-Rom volume driver.
	 */
	for( int drv=0; drv<drvNum && ( m_multiDriverMode || m_mixers.count()==0) ; drv++ )
	{
		bool drvInfoAppended = false;
		{
			for( int dev=0; dev<maxDevices; dev++ )
			{
				// hint: no driver is using cardnum, except the unsupported alsa0.5 - esken
				for( int card=0; card<1 /*maxCards*/; card++ )
				{
					Mixer *mixer = Mixer::getMixer( drv, dev, card );
					int mixerError = mixer->grab();
					if ( mixerError!=0 )
					{
						delete mixer;
						continue;
					}
					
					connect( timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
					m_mixers.append( mixer );
					
					// append driverName (used drivers)
					if ( !drvInfoAppended )
					{
						drvInfoAppended = true;
						QString driverName = Mixer::driverName(drv);
						if ( drv!= 0 ) 
						{
							driverInfoUsed += " + ";
						}
						driverInfoUsed += driverName;
					}
					
					// Check whether there are mixers in different drivers, so that the usr can be warned
					if (!multipleDriversActive) 
					{
						if ( driverWithMixer == -1 ) 
						{
							// Aha, this is the very first detected device
							driverWithMixer = drv;
						}
						else 
						{
							if ( driverWithMixer != drv ) 
							{
								// Got him: There are mixers in different drivers
								multipleDriversActive = true;
							}
						}
					}
					
					// count mixer nums for every mixer name to identify mixers with equal names
					mixerNums[mixer->mixerName()]++;
					mixer->setMixerNum( mixerNums[mixer->mixerName()] );
				}
			}
		}
	}

	m_hwInfoString = i18n("Sound drivers supported");
	m_hwInfoString += ": " + driverInfo + 
		"\n" + i18n("Sound drivers used") + ": " + driverInfoUsed;
	if ( multipleDriversActive ) 
	{
		// this will only be possible by hacking the config-file, as it will not be officially supported
		m_hwInfoString += "\nExperimental multiple-Driver mode activated";
	}
	
	kdDebug() << m_hwInfoString << endl;
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
	//QGridLayout *mainLayout = new QGridLayout( centralWidget(), 1, 1, 3, 1, "MainLayout" );

	// Widgets layout
	QVBoxLayout *widgetsLayout = new QVBoxLayout(   centralWidget(), 0, 0, "widgetsLayout" );
	widgetsLayout->setResizeMode(QLayout::Minimum); // basically good, but needs more work
	
	// Mixer widget line
	QHBoxLayout *mixerNameLayout = new QHBoxLayout( 0, 0, 6, "mixerNameLayout" );
	mixerNameLayout->addWidget( new QLabel( i18n(" Current Mixer: "), centralWidget() ) );
	m_cMixer = new KComboBox( FALSE, centralWidget(), "mixerCombo" );
   connect( m_cMixer, SIGNAL( activated( int ) ), this, SLOT( showSelectedMixer( int ) ) );
	QToolTip::add( m_cMixer, i18n("Current Mixer" ) );
	mixerNameLayout->addWidget( m_cMixer );

	// Add first layout to widgets
	widgetsLayout->addLayout( mixerNameLayout );
	
	m_wsMixers = new QWidgetStack( centralWidget(), "MixerWidgetStack" );
	widgetsLayout->addWidget( m_wsMixers );

	// Add to main layout
	//mainLayout->addLayout( widgetsLayout, 0, 0 );
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
		m_dockWidget = new KMixDockWidget( m_mixers.first(), this, "mainDockWidget" );
		updateDockIcon();

		// create RMB menu
		KPopupMenu *menu = m_dockWidget->contextMenu();

		KAction *a = actionCollection()->action("options_configure");
		if (a) a->plug( menu );

		a = actionCollection()->action("help_about_app");
		if (a) a->plug( menu );

		a = actionCollection()->action("help");
		if (a) a->plug( menu );

		menu->insertSeparator();

		a = actionCollection()->action( "dock_mute" );
		if ( a ) a->plug( menu );

		m_dockWidget->show();
	}
}

void
KMixWindow::dockMute()
{
	Mixer *mixer = m_mixers.first();
        if ( !mixer )
            return;
	MixDevice *masterDevice = ( *mixer )[ mixer->masterDevice() ];

	masterDevice->setMuted( !masterDevice->isMuted() );
	mixer->writeVolumeToHW( masterDevice->num(), masterDevice->getVolume() );

	updateDockIcon();
}

void
KMixWindow::updateDockIcon()
{
	Mixer *mixer = m_mixers.first();
        if ( !mixer )
	{
	    m_dockWidget->setErrorPixmap();
            return;
	}
	MixDevice *masterDevice = ( *mixer )[ mixer->masterDevice() ];

	// Required if muted from mixer widget
	KToggleAction *a = static_cast<KToggleAction *>( actionCollection()->action(  "dock_mute" ) );
	if ( a ) a->setChecked( masterDevice->isMuted() );

	m_dockWidget->updatePixmap();
	m_dockWidget->setVolumeTip( 0, masterDevice->getVolume() );
}

void
KMixWindow::updateDockTip(Volume vol)
{
	m_dockWidget->setVolumeTip( 0, vol );
}

void
KMixWindow::saveConfig()
{
	KConfig *config = kapp->config();
	config->setGroup(0);

	config->writeEntry( "Size", size() );
   config->writeEntry( "Position", pos() );
   // Cannot use isVisible() here, as in the "aboutToQuit()" case this widget is already hidden.
   // (Please note that the problem was only there when quitting via Systray - esken).
   config->writeEntry( "Visible", m_isVisible );
   config->writeEntry( "Menubar", m_showMenubar );
   config->writeEntry( "AllowDocking", m_showDockWidget );
   config->writeEntry( "TrayVolumeControl", m_volumeWidget );
   config->writeEntry( "Tickmarks", m_showTicks );
   config->writeEntry( "Labels", m_showLabels );

   // save mixer widgets
   QStringList devices;
   for ( KMixerWidget *mw = m_mixerWidgets.first(); mw != 0; mw = m_mixerWidgets.next() )
   {
		QString grp;
		grp.sprintf( "%i", mw->id() );
		devices << grp;
		
		config->setGroup( grp );
		config->writeEntry( "Mixer", mw->mixerNum() );
		config->writeEntry( "MixerName", mw->mixerName() );
		config->writeEntry( "Name", mw->name() );
		
		mw->saveConfig( config, grp );
	}

   config->setGroup(0);
   config->writeEntry( "Devices", devices );
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
   m_showTicks = config->readBoolEntry("Tickmarks", false);
   m_showLabels = config->readBoolEntry("Labels", false);
   m_startVisible = config->readBoolEntry("Visible", true);
   m_multiDriverMode = config->readBoolEntry("MultiDriver", false);

   // show/hide menu bar
   m_showMenubar = config->readBoolEntry("Menubar", true);
   if ( m_showMenubar )
      menuBar()->show();
   else
      menuBar()->hide();

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
	
	for ( mixer=m_mixers.first(),id=0; mixer!=0; mixer=m_mixers.next(),id++ )
	{
		kdDebug() << "Mixer number: " << id << " Name: " << mixer->mixerName() << endl ;

		
		KMixerWidget *mw = new KMixerWidget( id, mixer, mixer->mixerName(), mixer->mixerNum(),
				false, KPanelApplet::Up, MixDevice::ALL, this, "KMixerWidget" );

		mw->setName( mixer->mixerName() );
		
		m_mixerWidgets.append( mw );
		
		// Add to Combo
		m_cMixer->insertItem( mw->name() );
		
		// Add to Stack
		kdDebug() << "Inserted mixer " << id << ":" << mw->name() << endl;
		m_wsMixers->addWidget( mw, id );
		
		mw->setTicks( m_showTicks );
		mw->setLabels( m_showLabels );
		mw->addActionToPopup( actionCollection()->action("options_show_menubar") );
		mw->show();
		
		connect( mw, SIGNAL( masterMuted( bool ) ), SLOT( updateDockIcon() ) );
		connect( mw, SIGNAL( newMasterVolume( Volume ) ), SLOT( updateDockTip(Volume) ) );
	}
}

bool 
KMixWindow::queryClose ( )
{
    if ( m_showDockWidget )
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
 * Loads the volumes of all mixers from kmixctrl. In other words:
 * Restores the default voumes as stored via saveVolumes() or the
 * Mixer control center module..
 */
void
KMixWindow::loadVolumes()
{
	KConfig *cfg = new KConfig( "kmixctrlrc", true );
	for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
	{
		mixer->volumeLoad( cfg );
	}
	delete cfg;
}


/**
 * Stores the volumes of all mixers  Can be restored via loadVolumes() or
 * the kmixctrl application.
 */
void
KMixWindow::saveVolumes()
{
	KConfig *cfg = new KConfig( "kmixctrlrc", false );
   for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next())
	{
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

   this->setUpdatesEnabled(false);
   for (KMixerWidget *mw=m_mixerWidgets.first(); mw!=0; mw=m_mixerWidgets.next())
   {
      mw->setTicks( m_showTicks );
      mw->setLabels( m_showLabels );
   }

   updateDocking();
   this->setUpdatesEnabled(false);

   // avoid invisible and unaccessible main window
   if( !m_showDockWidget && !isVisible() )
	{
      timer->start(500);  // !!! Lets see how context menu, tooltips work
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
		 m_isVisible = true;
	 timer->start(500);
}

void 
KMixWindow::hideEvent( QHideEvent * ) 
{
	if ( m_visibilityUpdateAllowed ) 
	{
		m_isVisible = false;
	}
	timer->stop();
}


void 
KMixWindow::stopVisibilityUpdates() {
    m_visibilityUpdateAllowed = false;
}

void 
KMixWindow::slotHWInfo() {
	KMessageBox::information( 0, m_hwInfoString, i18n("Mixer hardware information") );
}

void
KMixWindow::showSelectedMixer( int mixer )
{
		kdDebug() << "showSelectedMixer(" << mixer << ")\n";
	m_wsMixers->raiseWidget( mixer );
}

#include "kmix.moc"

