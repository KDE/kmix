/*
 * KMix -- KDE's full featured mini mixer
 *
 * $Id$
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
	: KMainWindow(0, 0, 0 ), m_showTicks( true ), m_maxId( 0 ),
	m_lockedLayout(0),
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
	connect( kapp, SIGNAL( aboutToQuit()), SLOT( saveSettings()) );
}


KMixWindow::~KMixWindow()
{
}


void
KMixWindow::initActions()
{
	// file menu
	KStdAction::quit( this, SLOT(quit()), actionCollection());

	// settings menu
	KStdAction::showMenubar( this, SLOT(toggleMenuBar()), actionCollection());
	KStdAction::preferences( this, SLOT(showSettings()), actionCollection());

	KStdAction::keyBindings( guiFactory(), SLOT(configureShortcuts()), actionCollection());

	(void)new KToggleAction( i18n( "M&ute" ), 0, this, SLOT( dockMute() ),
				 actionCollection(), "dock_mute" );

	(void) new KAction( i18n( "Hardware &Information" ), 0, this, SLOT( slotHWInfo() ), actionCollection(), "hwinfo" );
	createGUI( "kmixui.rc" );
}

void
KMixWindow::initMixer()
{
	QString tmpstr;

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

	bool autodetectionFinished = false;
	for( int drv=0; drv<drvNum; drv++ )
	{
	    if ( autodetectionFinished ) {
		// sane exit from loop
		break;
	    }
	    bool drvInfoAppended = false;
	    // The "64" below is just a "silly" number:
	    // The loop will break as soon as an error is detected (e.g. on 3rd loop when 2 soundcards are installed)
	    for( int dev=0; dev<64; dev++ )
	    {
		Mixer *mixer = Mixer::getMixer( drv, dev, 0 );
		int mixerError = mixer->grab();
		if ( mixerError!=0 )
		{
		    if ( m_mixers.count() > 0 ) {
			// why not always ?!? !!
			delete mixer;
			mixer = 0;
		    }

		    /* If we get here, we *assume* that we probed the last dev of the current soundcard driver.
		     * We cannot be sure 100%, probably it would help to check the "mixerError" variable. But I
		     * currently don't see an error code that needs to be handled explicitely.
		     *
		     * Lets decide if we the autoprobing shall continue:
		     */
		    if ( m_mixers.count() == 0 ) {
			// Simple case: We have no mixers. Lets go on with next driver
			break;
		    }
		    else if ( m_multiDriverMode ) {
			// Special case: Multi-driver mode will probe more soundcards
			break;
		    }
		    else {
			// We have mixers, but no Multi-driver mode: Fine, we're done
			autodetectionFinished = true;
			break;
		    }
		}

		if ( mixer != 0 ) {
		    m_mixers.append( mixer );
		}

		// append driverName (used drivers)
		if ( !drvInfoAppended ) {
		    drvInfoAppended = true;
		    QString driverName = Mixer::driverName(drv);
		    if ( drv!= 0 && m_mixers.count() > 0) {
			driverInfoUsed += " + ";
		    }
		    driverInfoUsed += driverName;
		}

		// Check whether there are mixers in different drivers, so that the user can be warned
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
	    } // loop over sound card devices of current driver
	} // loop over soundcard drivers

	m_hwInfoString = i18n("Sound drivers supported");
	m_hwInfoString += ": " + driverInfo +
		"\n" + i18n("Sound drivers used") + ": " + driverInfoUsed;
	if ( multipleDriversActive )
	{
		// this will only be possible by hacking the config-file, as it will not be officially supported
		m_hwInfoString += "\nExperimental multiple-Driver mode activated";
	}

	kdDebug(67100) << m_hwInfoString << endl;
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
	QSizePolicy qsp( QSizePolicy::Ignored, QSizePolicy::Maximum);
	mixerNameLayout->setSizePolicy(qsp);
	mixerNameLayout->setSpacing(KDialog::spacingHint());
	QLabel *qlbl = new QLabel( i18n(" Current mixer:"), mixerNameLayout );
	qlbl->setFixedHeight(qlbl->sizeHint().height());
	m_cMixer = new KComboBox( FALSE, mixerNameLayout, "mixerCombo" );
	m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
	connect( m_cMixer, SIGNAL( activated( int ) ), this, SLOT( showSelectedMixer( int ) ) );
	QToolTip::add( m_cMixer, i18n("Current mixer" ) );

	// Add first layout to widgets
	widgetsLayout->addWidget( mixerNameLayout );

	m_wsMixers = new QWidgetStack( centralWidget(), "MixerWidgetStack" );
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
		m_dockWidget = new KMixDockWidget( m_mixers.first(), this, "mainDockWidget", m_volumeWidget );

		// create RMB menu
		KPopupMenu *menu = m_dockWidget->contextMenu();

		// !! check this
		KAction *a = actionCollection()->action( "dock_mute" );
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

   // save mixer widgets
   QStringList devices;
   for ( KMixerWidget *mw = m_mixerWidgets.first(); mw != 0; mw = m_mixerWidgets.next() )
   {
		QString grp;
		grp.sprintf( "%i", mw->id() );
		devices << grp;
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
   m_showTicks = config->readBoolEntry("Tickmarks", true);
   m_showLabels = config->readBoolEntry("Labels", true);
   m_onLogin = config->readBoolEntry("startkdeRestore", true );
   m_startVisible = config->readBoolEntry("Visible", true);
   m_multiDriverMode = config->readBoolEntry("MultiDriver", false);

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

	for ( mixer=m_mixers.first(),id=0; mixer!=0; mixer=m_mixers.next(),id++ )
	{
	    //kdDebug(67100) << "Mixer number: " << id << " Name: " << mixer->mixerName() << endl ;


                ViewBase::ViewFlags vflags = ViewBase::HasMenuBar;
                if ( m_showMenubar ) {
                    vflags |= ViewBase::MenuBarVisible;
	        }

		KMixerWidget *mw = new KMixerWidget( id, mixer, mixer->mixerName(), mixer->mixerNum(),
						     MixDevice::ALL, this, "KMixerWidget", vflags );

		//mw->setName( mixer->mixerName() );

		m_mixerWidgets.append( mw );

		// Add to Combo
		m_cMixer->insertItem( mixer->mixerName() );

		// Add to Stack
		//kdDebug(67100) << "Inserted mixer " << id << ":" << mw->name() << endl;
		m_wsMixers->addWidget( mw, id );

		QString grp;
		grp.sprintf( "%i", mw->id() );
		mw->loadConfig( kapp->config(), grp );

		mw->setTicks( m_showTicks );
		mw->setLabels( m_showLabels );
		// !!!
		//mw->addActionToPopup( actionCollection()->action("options_show_menubar") );
		//toggleMenuBar
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
    for (Mixer *mixer=m_mixers.first(); mixer!=0; mixer=m_mixers.next()) {
	//kdDebug(67100) << "KMixWindow::saveConfig()" << endl;
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
    // !! could possibly start polling now (idea: use someting like ref() and unref() on Mixer instance
}

void
KMixWindow::hideEvent( QHideEvent * )
{
    if ( m_visibilityUpdateAllowed )
    {
	m_isVisible = false;
    }
    // !! could possibly stop polling now (idea: use someting like ref() and unref() on Mixer instance
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

#include "kmix.moc"

