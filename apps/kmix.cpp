/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 1996-2000 Christian Esken <esken@kde.org>
 * Copyright 2000-2003 Christian Esken <esken@kde.org>, Stefan Schimanski <1Stein@gmx.de>
 * Copyright 2002-2007 Christian Esken <esken@kde.org>, Helio Chissini de Castro <helio@conectiva.com.br>
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


// include files for QT
#include <QCheckBox>
#include <QLabel>
#include <QDesktopWidget>
#include <QPushButton>
#include <qradiobutton.h>
#include <QCursor>
#include <KTabWidget>

// include files for KDE
#include <kcombobox.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kapplication.h>
#include <kstandardaction.h>
#include <kmenu.h>
#include <khelpmenu.h>
#include <kdebug.h>
#include <kxmlguifactory.h>
#include <kglobal.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>

// KMix
#include "gui/guiprofile.h"
#include "core/mixertoolbox.h"
#include "apps/kmix.h"
#include "core/kmixdevicemanager.h"
#include "gui/kmixerwidget.h"
#include "gui/kmixprefdlg.h"
#include "gui/kmixdockwidget.h"
#include "gui/kmixtoolbox.h"
#include "core/version.h"
#include "gui/viewdockareapopup.h"
#include "gui/dialogselectmaster.h"

#include "gui/osdwidget.h"


/* KMixWindow
 * Constructs a mixer window (KMix main window)
 */
KMixWindow::KMixWindow(bool invisible)
: KXmlGuiWindow(0, Qt::WindowFlags( KDE_DEFAULT_WINDOWFLAGS | Qt::WindowContextHelpButtonHint) ),
  m_showTicks( true ),
  m_multiDriverMode (false), // -<- I never-ever want the multi-drivermode to be activated by accident
  m_dockWidget(),
  m_dontSetDefaultCardOnStart (false)
{
    _cornerLabelNew = 0;

    setObjectName("KMixWindow");
    // disable delete-on-close because KMix might just sit in the background waiting for cards to be plugged in
    setAttribute(Qt::WA_DeleteOnClose, false);

    initActions(); // init actions first, so we can use them in the loadConfig() already
    loadConfig(); // Load config before initMixer(), e.g. due to "MultiDriver" keyword
    initActionsLate(); // init actions that require a loaded config
    KGlobal::locale()->insertCatalog("kmix-controls");
    initWidgets();
    initPrefDlg();
    MixerToolBox::instance()->initMixer(m_multiDriverMode, m_hwInfoString);
    KMixDeviceManager *theKMixDeviceManager = KMixDeviceManager::instance();
    recreateGUI(false);
    fixConfigAfterRead();
    theKMixDeviceManager->initHotplug();
    connect(theKMixDeviceManager, SIGNAL( plugged( const char*, const QString&, QString&)), SLOT (plugged( const char*, const QString&, QString&) ) );
    connect(theKMixDeviceManager, SIGNAL( unplugged( const QString&)), SLOT (unplugged( const QString&) ) );
    if ( m_startVisible && ! invisible)
        show(); // Started visible

    connect( kapp, SIGNAL( aboutToQuit()), SLOT( saveConfig()) );
}


KMixWindow::~KMixWindow()
{
    // -1- Cleanup Memory: clearMixerWidgets
    while ( m_wsMixers->count() != 0 )
    {
        QWidget *mw = m_wsMixers->widget(0);
        m_wsMixers->removeTab(0);
        delete mw;
    }
    // -2- Mixer HW
    MixerToolBox::instance()->deinitMixer();
}


void KMixWindow::initActions()
{
    // file menu
    KStandardAction::quit( this, SLOT(quit()), actionCollection());

    // settings menu
    _actionShowMenubar = KStandardAction::showMenubar( this, SLOT(toggleMenuBar()), actionCollection());
    //actionCollection()->addAction( a->objectName(), a );
    KStandardAction::preferences( this, SLOT(showSettings()), actionCollection());
    KStandardAction::keyBindings( guiFactory(), SLOT(configureShortcuts()), actionCollection());

    KAction *action = actionCollection()->addAction( "hwinfo" );
    action->setText( i18n( "Hardware &Information" ) );
    connect(action, SIGNAL(triggered(bool) ), SLOT( slotHWInfo() ));
    action = actionCollection()->addAction( "hide_kmixwindow" );
    action->setText( i18n( "Hide Mixer Window" ) );
    connect(action, SIGNAL(triggered(bool) ), SLOT(hideOrClose()));
    action->setShortcut(QKeySequence(Qt::Key_Escape));
    action = actionCollection()->addAction("toggle_channels_currentview");
    action->setText(i18n("Configure &Channels..."));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotConfigureCurrentView()));
    action = actionCollection()->addAction( "select_master" );
    action->setText( i18n("Select Master Channel...") );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotSelectMaster()));

    osdWidget = new OSDWidget();

    createGUI( "kmixui.rc" );
}

void KMixWindow::initActionsLate()
{
    if ( m_autouseMultimediaKeys ) {
        KAction* globalAction = actionCollection()->addAction("increase_volume");
        globalAction->setText(i18n("Increase Volume"));
        globalAction->setGlobalShortcut(KShortcut(Qt::Key_VolumeUp), ( KAction::ShortcutTypes)( KAction::ActiveShortcut |  KAction::DefaultShortcut),  KAction::NoAutoloading);
        connect(globalAction, SIGNAL(triggered(bool) ), SLOT(slotIncreaseVolume()));

        globalAction = actionCollection()->addAction("decrease_volume");
        globalAction->setText(i18n("Decrease Volume"));
        globalAction->setGlobalShortcut(KShortcut(Qt::Key_VolumeDown));
        connect(globalAction, SIGNAL(triggered(bool) ), SLOT(slotDecreaseVolume()));

        globalAction = actionCollection()->addAction("mute");
        globalAction->setText(i18n("Mute"));
        globalAction->setGlobalShortcut(KShortcut(Qt::Key_VolumeMute));
        connect(globalAction, SIGNAL(triggered(bool) ), SLOT(slotMute()));
    }
}


void KMixWindow::initPrefDlg()
{
    m_prefDlg = new KMixPrefDlg( this );
    connect( m_prefDlg, SIGNAL(signalApplied(KMixPrefDlg *)), SLOT(applyPrefs(KMixPrefDlg *)) );
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

    m_wsMixers = new KTabWidget( centralWidget() );
    m_wsMixers->setTabsClosable(true);
    connect (m_wsMixers, SIGNAL(tabCloseRequested(int)), SLOT(closeView(int)) );

    QPixmap cornerNewPM = KIconLoader::global()->loadIcon( "tab-new", KIconLoader::Toolbar, KIconLoader::SizeSmall );
    _cornerLabelNew = new QPushButton();
    _cornerLabelNew->setIcon(cornerNewPM);
    //cornerLabelNew->setSizePolicy(QSizePolicy());
    m_wsMixers->setCornerWidget(_cornerLabelNew, Qt::TopLeftCorner);
    connect ( _cornerLabelNew, SIGNAL( clicked() ), SLOT (newView() ) );

    connect( m_wsMixers, SIGNAL( currentChanged ( int ) ), SLOT( newMixerShown(int)) );

    m_widgetsLayout->addWidget(m_wsMixers);

    // show menubar if the actions says so (or if the action does not exist)
    menuBar()->setVisible( (_actionShowMenubar==0) || _actionShowMenubar->isChecked());

    m_widgetsLayout->activate();
}


/**
 * Updates the docking icon by recreating it.
 * @returns Whether the docking succeeded. Failure usually means that there
 *    was no suitable mixer control selected.
 */
bool KMixWindow::updateDocking()
{
    // delete old dock widget
    if (m_dockWidget)
    {
        // If this is called during a master control change, the dock widget is currently active, so we use deleteLater().
        m_dockWidget->deleteLater();
        m_dockWidget = 0L;
    }

    if ( m_showDockWidget == false || Mixer::mixers().count() == 0 ) {
        return false;
    }

    m_dockWidget = new KMixDockWidget( this, m_volumeWidget );  // Could be optimized, by refreshing instead of recreating.
    connect(m_dockWidget, SIGNAL(newMasterSelected()), SLOT(saveConfig()) );
    return true;
}

void KMixWindow::saveConfig()
{
    kDebug() << "About to save config";
    saveBaseConfig();
    saveViewConfig();
    saveVolumes();
#ifdef __GNUC_
#warn We must Sync here, or we will lose configuration data. The reson for that is unknown.
#endif

    kDebug() << "Saved config ... now syncing explicitely";
    KGlobal::config()->sync();
    kDebug() << "Saved config ... sync finished";
}

void KMixWindow::saveBaseConfig()
{
    kDebug() << "About to save config (Base)";
    KConfigGroup config(KGlobal::config(), "Global");

    config.writeEntry( "Size", size() );
    config.writeEntry( "Position", pos() );
    // Cannot use isVisible() here, as in the "aboutToQuit()" case this widget is already hidden.
    // (Please note that the problem was only there when quitting via Systray - esken).
    // Using it again, as internal behaviour has changed with KDE4
    config.writeEntry( "Visible", isVisible() );
    config.writeEntry( "Menubar", _actionShowMenubar->isChecked() );
    config.writeEntry( "AllowDocking", m_showDockWidget );
    config.writeEntry( "TrayVolumeControl", m_volumeWidget );
    config.writeEntry( "Tickmarks", m_showTicks );
    config.writeEntry( "Labels", m_showLabels );
    config.writeEntry( "startkdeRestore", m_onLogin );
    config.writeEntry( "DefaultCardOnStart", m_defaultCardOnStart );
    config.writeEntry( "ConfigVersion", KMIX_CONFIG_VERSION );
    config.writeEntry( "AutoUseMultimediaKeys", m_autouseMultimediaKeys );
    Mixer* mixerMasterCard = Mixer::getGlobalMasterMixer();
    if ( mixerMasterCard != 0 ) {
        config.writeEntry( "MasterMixer", mixerMasterCard->id() );
    }
    MixDevice* mdMaster = Mixer::getGlobalMasterMD();
    if ( mdMaster != 0 ) {
        config.writeEntry( "MasterMixerDevice", mdMaster->id() );
    }
    QString mixerIgnoreExpression = MixerToolBox::instance()->mixerIgnoreExpression();
    config.writeEntry( "MixerIgnoreExpression", mixerIgnoreExpression );

    // @todo basically this should be moved in the views later (e.g. KDE4.2 ?)
    if ( m_toplevelOrientation  == Qt::Horizontal )
        config.writeEntry( "Orientation","Horizontal" );
    else
        config.writeEntry( "Orientation","Vertical" );

    kDebug() << "Config (Base) saving done";
}

void KMixWindow::saveViewConfig()
{
    kDebug() << "About to save config (View)";
    // Save Views

    QMap<QString, QStringList> mixerViews;
    
    // The following loop is necessary for the case that the user has hidden all views for a Mixer instance.
    // Otherwise we would not save the Meta information (step -2- below for that mixer.
    QListIterator<Mixer*> it(Mixer::mixers());
    while ( it.hasNext()) {
        Mixer* mixer = it.next();
        mixerViews[mixer->id()]; // just insert a map entry
	}

    // -1- Save the views themselves
    for ( int i=0; i<m_wsMixers->count() ; ++i )
    {
        QWidget *w = m_wsMixers->widget(i);
        if ( w->inherits("KMixerWidget") ) {
            KMixerWidget* mw = (KMixerWidget*)w;
            // Here also Views are saved. even for Mixers that are closed. This is necessary when unplugging cards.
            // Otherwise the user will be confused afer re-plugging the card (as the config was not saved).
            mw->saveConfig( KGlobal::config().data() );
            // add the view to the corresponding mixer list, so we can save a views-per-mixer list below
            QStringList& qsl = mixerViews[mw->mixer()->id()];
            qsl.append(mw->getGuiprof()->getId());
        }
    }

    // -2- Save Meta-Information (which views, and in which order). views-per-mixer list
    KConfigGroup pconfig(KGlobal::config(), "Profiles");
    QMap<QString, QStringList>::const_iterator itEnd = mixerViews.end();
    for ( QMap<QString, QStringList>::const_iterator it=mixerViews.begin(); it != itEnd; ++it )
    {
        const QString& mixerProfileKey = it.key();   // this is actually some mixer->id()
        const QStringList& qslProfiles = it.value();
        pconfig.writeEntry( mixerProfileKey, qslProfiles );
        kDebug() << "Save Profile List for " << mixerProfileKey << ", number of views is " << qslProfiles.count();
    }

    kDebug() << "Config (View) saving done";
}


/**
 * Stores the volumes of all mixers  Can be restored via loadVolumes() or
 * the kmixctrl application.
 */
void KMixWindow::saveVolumes()
{
    kDebug() << "About to save config (Volume)";
    KConfig *cfg = new KConfig( "kmixctrlrc" );
    for ( int i=0; i<Mixer::mixers().count(); ++i)
    {
        Mixer *mixer = (Mixer::mixers())[i];
        if ( mixer->isOpen() ) { // protect from unplugged devices (better do *not* save them)
            mixer->volumeSave( cfg );
        }
    }
    delete cfg;
    kDebug() << "Config (Volume) saving done";
}



void KMixWindow::loadConfig()
{
    loadBaseConfig();
    //loadViewConfig(); // mw->loadConfig() explicitly called always after creating mw.
    //loadVolumes(); // not in use
}

void KMixWindow::loadBaseConfig()
{
    KConfigGroup config(KGlobal::config(), "Global");

    m_showDockWidget = config.readEntry("AllowDocking", true);
    m_volumeWidget = config.readEntry("TrayVolumeControl", true);
    m_showTicks = config.readEntry("Tickmarks", true);
    m_showLabels = config.readEntry("Labels", true);
    m_onLogin = config.readEntry("startkdeRestore", true );
    m_startVisible = config.readEntry("Visible", false);
    m_multiDriverMode = config.readEntry("MultiDriver", false);
    const QString& orientationString = config.readEntry("Orientation", "Vertical");
    m_defaultCardOnStart = config.readEntry( "DefaultCardOnStart", "" );
    m_configVersion = config.readEntry( "ConfigVersion", 0 );
    // WARNING Don't overwrite m_configVersion with the "correct" value, before having it
    // evaluated. Better only write that in saveBaseConfig()
    m_autouseMultimediaKeys = config.readEntry( "AutoUseMultimediaKeys", true );
    QString mixerMasterCard = config.readEntry( "MasterMixer", "" );
    QString masterDev = config.readEntry( "MasterMixerDevice", "" );
    //if ( ! mixerMasterCard.isEmpty() && ! masterDev.isEmpty() ) {
    Mixer::setGlobalMaster(mixerMasterCard, masterDev);
    //}
    QString mixerIgnoreExpression = config.readEntry( "MixerIgnoreExpression", "Modem" );
    MixerToolBox::instance()->setMixerIgnoreExpression(mixerIgnoreExpression);

    if ( orientationString == "Horizontal" )
        m_toplevelOrientation  = Qt::Horizontal;
    else
        m_toplevelOrientation = Qt::Vertical;

    // show/hide menu bar
    bool showMenubar = config.readEntry("Menubar", true);

    if (_actionShowMenubar) _actionShowMenubar->setChecked( showMenubar );

    // restore window size and position
    if ( !kapp->isSessionRestored() ) // done by the session manager otherwise
    {
        QSize defSize( minimumWidth(), height() );
        QSize size = config.readEntry("Size", defSize );
        if(!size.isEmpty()) resize(size);

        QPoint defPos = pos();
        QPoint pos = config.readEntry("Position", defPos);
        move(pos);
    }
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




void KMixWindow::recreateGUIwithoutSavingView()
{
    recreateGUI(false);
}


void KMixWindow::recreateGUIwithSavingView()
{
    recreateGUI(true);
}

/**
 * Create or recreate the Mixer GUI elements
 */
void KMixWindow::recreateGUI(bool saveConfig)
{
    // Find out which of the tabs is currently selected for restoration
    int current_tab = -1;
    if (m_wsMixers)
        current_tab = m_wsMixers->currentIndex();

    KConfigGroup config(KGlobal::config(), "Global");

    if (saveConfig)
        saveViewConfig();  // save the state before recreating

    // Start a new generation now. This shows whether we already have recreated the
    // KMixerWidget/Tab for the CURRENTLY STARTED recreation phase.
    KMixerWidget::increaseGeneration(); // @todo:Generation stuff should be obsolete now => Remove it

    // *** RECREATE THE ALREADY EXISTING TABS **********************************
    QMap<Mixer*, bool> mixerHasProfile; //

    QMap<QString, GUIProfile*>::const_iterator itEnd = GUIProfile::getProfiles().end();
    for ( QMap<QString, GUIProfile*>::const_iterator it    = GUIProfile::getProfiles().begin(); it != itEnd; ++it)
    {
        GUIProfile* guiprof = it.value();
        KMixerWidget* kmw = findKMWforTab(guiprof->getId());
        Mixer *mixer =  Mixer::findMixer( guiprof->getMixerId() );
        if ( mixer == 0 ) {
            kError() << "MixerToolBox::find() hasn't found the Mixer for the profile " << guiprof->getId();
            continue;
        }
        mixerHasProfile[mixer] = true;
        if ( kmw == 0 ) {
            // does not yet exist => create
            addMixerWidget(mixer->id(), guiprof, -1);
        }
        else {
            if ( kmw->generationIsOutdated() ) {
                // not yet regenerated => regenerate
                int indexOfTab =  m_wsMixers->indexOf(kmw);
                if ( indexOfTab != -1 ) m_wsMixers->removeTab(indexOfTab);
                delete kmw;
                addMixerWidget(mixer->id(), guiprof, indexOfTab);
            }
        }
    } // Loop over all GUIProfile's


    // *** ADD TABS FOR Mixer instances that have no tab yet **********************************
    KConfigGroup pconfig(KGlobal::config(), "Profiles");
    for (int i=0; i<Mixer::mixers().count(); ++i) {
        Mixer *mixer = (Mixer::mixers())[i];
        if ( mixerHasProfile.contains(mixer)) {
            continue;
        }
        // No TAB YET => This should mean KMix is just started, or the user has added another GUIProfile
        QString mixerProfileKey(mixer->id());
        QStringList profileList = pconfig.readEntry( mixerProfileKey, QStringList("") ); // Hint: Default is a list with ONE entry (an empty string). Important for GUIProfile::find()
        for ( int i=0; i<profileList.count(); i++ ) {
            kDebug() << "Now searching for profile: " << profileList.at(i);
            bool allowDefault = (i == (profileList.count() - 1)); // In the last loop iteration, allow the fallback as last resort.
            GUIProfile* guiprof = GUIProfile::find(mixer, profileList.at(i), allowDefault);
            if ( guiprof == 0 ) {
                QString fullQualifiedProfileNameWithoutCardname = GUIProfile::buildProfileName(mixer, profileList.at(i), true);
                guiprof = GUIProfile::find(mixer, fullQualifiedProfileNameWithoutCardname, allowDefault);  // Load from XML ### Card unspecific profile ###
                if ( guiprof != 0 ) {
                    guiprof->setDirty();  // loaded from unspecific file => dirty
                }
            }
            if ( guiprof == 0 ) {
                kError() << "Cannot load profile " << profileList.at(i) << " . It was removed by the user, or the KMix config file is defective.";
                continue;
            }
            else

            addMixerWidget(mixer->id(), guiprof, -1);
        }
    }
    mixerHasProfile.clear();

    if (m_wsMixers->count() > 0) {
        if (current_tab >= 0) {
            m_wsMixers->setCurrentIndex(current_tab);
        }
        bool dockingSucceded = updateDocking();
        if ( !dockingSucceded && Mixer::mixers().count() > 0 )
        {
            show(); // avoid invisible and unaccessible main window
        }
    }
    else {
        // No soundcard found. Do not complain, but sit in the background, and wait for newly plugged soundcards.   
        updateDocking();  // -<- removes the DockIcon
        hide();
    }

}

KMixerWidget* KMixWindow::findKMWforTab( QString kmwId )
{
    KMixerWidget* kmw = 0;
    for (int i=0; i< m_wsMixers->count(); ++i)
    {
        KMixerWidget* kmwTmp = (KMixerWidget*)m_wsMixers->widget(i);
        if ( kmwTmp->getGuiprof()->getId() == kmwId ) {
            kmw = kmwTmp;
            break;
        }
    }
    return kmw;
}


void KMixWindow::newView()
{
    kDebug() << "Enter";

    if ( Mixer::mixers().count()  < 1 ) {
        kError() << "Trying to create a View, but no Mixer exists";
        return; // should never happen
    }

    Mixer* mixer = Mixer::mixers()[0];

    bool allowDefault = false;

    static int dummyToggler = false;
    dummyToggler = !dummyToggler;
    GUIProfile* guiprof;
    QString profileName;

    if ( dummyToggler ) {
        profileName = GUIProfile::buildProfileName(mixer, QString("playback"), false);
        guiprof = GUIProfile::find(mixer, profileName, allowDefault);
        if ( guiprof == 0 ) {
            profileName = GUIProfile::buildProfileName(mixer, QString("playback"), true);
            guiprof = GUIProfile::find(mixer, profileName, allowDefault);
        }
    }
    else {
        profileName = GUIProfile::buildProfileName(mixer, QString("capture"), false);
        guiprof = GUIProfile::find(mixer, profileName, allowDefault);
        if ( guiprof == 0 ) {
            profileName = GUIProfile::buildProfileName(mixer, QString("capture"), true);
            guiprof = GUIProfile::find(mixer, profileName, allowDefault);
        }
    }

    if ( guiprof == 0 ) {
        kError() << "Cannot add view - GUIProfile is invalid";
    }
    else  {
        addMixerWidget(mixer->id(), guiprof, -1);
    }

    kDebug() << "Exit";
}

void KMixWindow::closeView(int idx)
{
    kDebug() << "Enter";
    QWidget *w = m_wsMixers->widget(idx);
    KMixerWidget* kmw = ::qobject_cast<KMixerWidget*>(w);
    if ( kmw ) {
        saveAndCloseView(kmw, idx);
    }
    kDebug() << "Exit";
}

/**
 * Save the view and close it
 * @arg kmw The KMixerWidget to close
 * @arg idx The index in the TabWidget
 */
void KMixWindow::saveAndCloseView(KMixerWidget *kmw, int idx)
{
    kmw->saveConfig( KGlobal::config().data() );
    m_wsMixers->removeTab(idx);
    delete kmw;

    if ( m_wsMixers->count() < 2 ) {
        m_wsMixers->setTabsClosable(false);
    }
}

/**
 * Create or recreate the Mixer GUI elements
 */
void KMixWindow::redrawMixer( const QString& mixer_ID )
{
    for ( int i=0; i<m_wsMixers->count() ; ++i )
    {
        QWidget *w = m_wsMixers->widget(i);
        if ( w->inherits("KMixerWidget") )
        {
            KMixerWidget* kmw = (KMixerWidget*)w;
            if ( kmw->mixer()->id() == mixer_ID )
            {
                kDebug(67100) << "KMixWindow::redrawMixer() " << mixer_ID << " is being redrawn";
                kmw->loadConfig( KGlobal::config().data() );

                // Is the below needed? It is done on startup so copied it here...
                kmw->setTicks( m_showTicks );
                kmw->setLabels( m_showLabels );

                return;
            }
        }
    }

    kWarning(67100) << "KMixWindow::redrawMixer() Requested to redraw " << mixer_ID << " but I cannot find it :s";
}


void KMixWindow::fixConfigAfterRead()
{
    KConfigGroup grp(KGlobal::config(), "Global");
    unsigned int configVersion = grp.readEntry( "ConfigVersion", 0 );
    if ( configVersion < 3 ) {
        // Fix the "double Base" bug, by deleting all groups starting with "View.Base.Base.".
        // The group has been copied over by KMixToolBox::loadView() for all soundcards, so
        // we should be fine now
        QStringList cfgGroups = KGlobal::config()->groupList();
        QStringListIterator it(cfgGroups);
        while ( it.hasNext() ) {
            QString groupName = it.next();
            if ( groupName.indexOf("View.Base.Base" ) == 0 ) {
                kDebug(67100) << "Fixing group " << groupName;
                KConfigGroup buggyDevgrpCG = KGlobal::config()->group( groupName );
                buggyDevgrpCG.deleteGroup();
            } // remove buggy group
        } // for all groups
    } // if config version < 3
}

void KMixWindow::plugged( const char* driverName, const QString& /*udi*/, QString& dev)
{
    //     kDebug(67100) << "Plugged: dev=" << dev << "(" << driverName << ") udi=" << udi << "\n";
    QString driverNameString;
    driverNameString = driverName;
    int devNum = dev.toInt();
    Mixer *mixer = new Mixer( driverNameString, devNum );
    if ( mixer != 0 ) {
        kDebug(67100) << "Plugged: dev=" << dev << "\n";
        MixerToolBox::instance()->possiblyAddMixer(mixer);
        recreateGUI(true);
    }

    // Test code for OSD. But OSD is postponed to KDE4.1
    //    OSDWidget* osd = new OSDWidget(0);
    //    osd->volChanged(70, true);

}

void KMixWindow::unplugged( const QString& udi)
{
    //     kDebug(67100) << "Unplugged: udi=" <<udi << "\n";
    for (int i=0; i<Mixer::mixers().count(); ++i) {
        Mixer *mixer = (Mixer::mixers())[i];
        //         kDebug(67100) << "Try Match with:" << mixer->udi() << "\n";
        if (mixer->udi() == udi ) {
            kDebug(67100) << "Unplugged Match: Removing udi=" <<udi << "\n";
            //KMixToolBox::notification("MasterFallback", "aaa");
            bool globalMasterMixerDestroyed = ( mixer == Mixer::getGlobalMasterMixer() );
            // Part 1) Remove Tab
            for ( int i=0; i<m_wsMixers->count() ; ++i )
            {
                QWidget *w = m_wsMixers->widget(i);
                KMixerWidget* kmw = ::qobject_cast<KMixerWidget*>(w);
                if ( kmw && kmw->mixer() ==  mixer ) {
                    saveAndCloseView(kmw,i);
                    i= -1; // Restart loop from scratch (indices are most likely invalidated at removeTab() )
                }
            }
            MixerToolBox::instance()->removeMixer(mixer);
            // Check whether the Global Master disappeared, and select a new one if necessary
            MixDevice* md = Mixer::getGlobalMasterMD();
            if ( globalMasterMixerDestroyed || md == 0 ) {
                // We don't know what the global master should be now.
                // So lets play stupid, and just select the recommendended master of the first device
                if ( Mixer::mixers().count() > 0 ) {
                    QString localMaster = ((Mixer::mixers())[0])->getLocalMasterMD()->id();
                    Mixer::setGlobalMaster( ((Mixer::mixers())[0])->id(), localMaster);

                    QString text;
                    text = i18n("The soundcard containing the master device was unplugged. Changing to control %1 on card %2.", 
                            ((Mixer::mixers())[0])->getLocalMasterMD()->readableName(),
                            ((Mixer::mixers())[0])->readableName()
                    );
                    KMixToolBox::notification("MasterFallback", text);
                }
            }
            if ( Mixer::mixers().count() == 0 ) {
                QString text;
                text = i18n("The last soundcard was unplugged.");
                KMixToolBox::notification("MasterFallback", text);
            }
            recreateGUI(true);
            break;
        }
    }

}


/**
 * Create a widget with an error message
 * This widget shows an error message like "no mixers detected.
void KMixWindow::setErrorMixerWidget()
{
   QString s = i18n("Please plug in your soundcard.No soundcard found. Probably you have not set it up or are missing soundcard drivers. Please check your operating system manual for installing your soundcard."); // !! better text
   m_errorLabel = new QLabel( s,this  );
   m_errorLabel->setAlignment( Qt::AlignCenter );
   m_errorLabel->setWordWrap(true);
   m_errorLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
   m_wsMixers->addTab( m_errorLabel, i18n("No soundcard found") );
}
 */

void KMixWindow::addMixerWidget(const QString& mixer_ID, GUIProfile *guiprof, int insertPosition)
{
    //    kDebug(67100) << "KMixWindow::addMixerWidget() " << mixer_ID;
    Mixer *mixer = Mixer::findMixer(mixer_ID);
    if ( mixer != 0 )
    {
        //       kDebug(67100) << "KMixWindow::addMixerWidget() " << mixer_ID << " is being added";
        ViewBase::ViewFlags vflags = ViewBase::HasMenuBar;
        if ( (_actionShowMenubar==0) || _actionShowMenubar->isChecked() ) {
            vflags |= ViewBase::MenuBarVisible;
        }
        if ( m_toplevelOrientation == Qt::Vertical ) {
            vflags |= ViewBase::Horizontal;
        }
        else {
            vflags |= ViewBase::Vertical;
        }


        KMixerWidget *kmw = new KMixerWidget( mixer, this, vflags, guiprof, actionCollection() );
        /* A newly added mixer will automatically added at the top
         * and thus the window title is also set appropriately */

        QString tabLabel(kmw->mixer()->readableName());
        if ( ! guiprof->getName().isEmpty() ) {
            tabLabel += ": ";
            tabLabel += guiprof->getName(); // @todo This name is possibly bad when using Pulesaudio => Check with Colin
        }

        m_dontSetDefaultCardOnStart = true; // inhibit implicit setting of m_defaultCardOnStart

        if ( insertPosition == -1 )
            m_wsMixers->addTab( kmw, tabLabel );
        else
            m_wsMixers->insertTab( insertPosition, kmw, tabLabel );

        if ( kmw->getGuiprof()->getId() == m_defaultCardOnStart ) {
            m_wsMixers->setCurrentWidget(kmw);
        }
        if ( m_wsMixers->count() > 1 ) {
            m_wsMixers->setTabsClosable(true);
        }
        m_dontSetDefaultCardOnStart = false;


        kmw->loadConfig( KGlobal::config().data() );

        kmw->setTicks( m_showTicks );
        kmw->setLabels( m_showLabels );
        kmw->mixer()->readSetFromHWforceUpdate();
    } // given mixer exist really
}



bool KMixWindow::queryClose ( )
{
    //     kDebug(67100) << "queryClose ";
    if ( m_showDockWidget && !kapp->sessionSaving() )
    {
        //         kDebug(67100) << "don't close";
        // Hide (don't close and destroy), if docking is enabled. Except when session saving (shutdown) is in process.
        hide();
        return false;
    }
    else {
        // Accept the close, if:
        //     The user has disabled docking
        // or  SessionSaving() is running
        //         kDebug(67100) << "close";
        return true;
    }
}

void KMixWindow::hideOrClose ( )
{
    if ( m_showDockWidget  && m_dockWidget != 0) {
        // we can hide if there is a dock widget
        hide();
    }
    else {
        //  if there is no dock widget, we will quit
        quit();
    }
}

// internal helper to prevent code duplication in slotIncreaseVolume and slotDecreaseVolume
void KMixWindow::increaseOrDecreaseVolume(bool increase)
{
    Mixer* mixer = Mixer::getGlobalMasterMixer(); // only needed for the awkward construct below
    if ( mixer == 0 ) return; // e.g. when no soundcard is available
    MixDevice *md = Mixer::getGlobalMasterMD();
    if ( md == 0 ) return; // shouldn't happen, but lets play safe
    md->setMuted(false);
    if (increase)
        mixer->increaseVolume(md->id());    // this is awkward. Better move the increaseVolume impl to the Volume class.
    else
        mixer->decreaseVolume(md->id());
    // md->playbackVolume().increase(); // not yet implemented
    showVolumeDisplay();
}

void KMixWindow::slotIncreaseVolume()
{
    increaseOrDecreaseVolume(true);
}

void KMixWindow::slotDecreaseVolume()
{
    increaseOrDecreaseVolume(false);
}

void KMixWindow::showVolumeDisplay()
{
    Mixer* mixer = Mixer::getGlobalMasterMixer();
    if ( mixer == 0 ) return; // e.g. when no soundcard is available
    MixDevice *md = Mixer::getGlobalMasterMD();
    if ( md == 0 ) return; // shouldn't happen, but lets play safe
    int currentVolume = mixer->volume(md->id());

    osdWidget->setCurrentVolume(currentVolume, md->isMuted());
    osdWidget->show();
    osdWidget->activateOSD(); //Enable the hide timer

    //Center the OSD
    QRect rect = KApplication::kApplication()->desktop()->screenGeometry(QCursor::pos());
    QSize size = osdWidget->sizeHint();
    int posX = rect.x() + (rect.width() - size.width()) / 2;
    int posY = rect.y() + 4 * rect.height() / 5;
    osdWidget->setGeometry(posX, posY, size.width(), size.height());
}

void KMixWindow::slotMute()
{
    Mixer* mixer = Mixer::getGlobalMasterMixer();
    if ( mixer == 0 ) return; // e.g. when no soundcard is available
    MixDevice *md = Mixer::getGlobalMasterMD();
    if ( md == 0 ) return; // shouldn't happen, but lets play safe
    mixer->toggleMute(md->id());
    showVolumeDisplay();
}

void KMixWindow::quit()
{
    //     kDebug(67100) << "quit";
    kapp->quit();
}


void KMixWindow::showSettings()
{
    if (!m_prefDlg->isVisible())
    {
        // copy actual values to dialog
        m_prefDlg->m_dockingChk->setChecked( m_showDockWidget );
        m_prefDlg->m_volumeChk->setChecked(m_volumeWidget);
        m_prefDlg->m_volumeChk->setEnabled( m_showDockWidget );
        m_prefDlg->m_onLogin->setChecked( m_onLogin );

        m_prefDlg->m_showTicks->setChecked( m_showTicks );
        m_prefDlg->m_showLabels->setChecked( m_showLabels );
        m_prefDlg->_rbVertical  ->setChecked( m_toplevelOrientation == Qt::Vertical );
        m_prefDlg->_rbHorizontal->setChecked( m_toplevelOrientation == Qt::Horizontal );

        // show dialog
        m_prefDlg->show();
    }
}


void KMixWindow::showHelp()
{
    actionCollection()->action( "help_contents" )->trigger();
}


void
KMixWindow::showAbout()
{
    actionCollection()->action( "help_about_app" )->trigger();
}



void KMixWindow::applyPrefs( KMixPrefDlg *prefDlg )
{
    bool labelsHasChanged = m_showLabels ^ prefDlg->m_showLabels->isChecked();
    bool ticksHasChanged = m_showTicks ^ prefDlg->m_showTicks->isChecked();
    bool dockwidgetHasChanged = m_showDockWidget ^ prefDlg->m_dockingChk->isChecked();
    bool systrayPopupHasChanged = m_volumeWidget ^ prefDlg->m_volumeChk->isChecked();
    bool toplevelOrientationHasChanged =
            ( prefDlg->_rbVertical->isChecked()   && m_toplevelOrientation == Qt::Horizontal )
            || ( prefDlg->_rbHorizontal->isChecked() && m_toplevelOrientation == Qt::Vertical   );

    m_showLabels = prefDlg->m_showLabels->isChecked();
    m_showTicks = prefDlg->m_showTicks->isChecked();
    m_showDockWidget = prefDlg->m_dockingChk->isChecked();
    m_volumeWidget = prefDlg->m_volumeChk->isChecked();
    m_onLogin = prefDlg->m_onLogin->isChecked();
    if ( prefDlg->_rbVertical->isChecked() ) {
        m_toplevelOrientation = Qt::Vertical;
    }
    else if ( prefDlg->_rbHorizontal->isChecked() ) {
        m_toplevelOrientation = Qt::Horizontal;
    }

    if ( labelsHasChanged || ticksHasChanged || dockwidgetHasChanged || toplevelOrientationHasChanged || systrayPopupHasChanged) {
        recreateGUI(true);
    }

    this->repaint(); // make KMix look fast (saveConfig() often uses several seconds)
    kapp->processEvents();
    saveConfig();
}


void KMixWindow::toggleMenuBar()
{
    menuBar()->setVisible(_actionShowMenubar->isChecked());
}


void KMixWindow::slotHWInfo()
{
    KMessageBox::information( 0, m_hwInfoString, i18n("Mixer Hardware Information") );
}

void KMixWindow::slotConfigureCurrentView()
{
    KMixerWidget* mw = (KMixerWidget*)m_wsMixers->currentWidget();
    ViewBase* view = 0;
    if (mw) view = mw->currentView();
    if (view) view->configureView();
}

void KMixWindow::slotSelectMaster()
{
    DialogSelectMaster* dsm = new DialogSelectMaster(Mixer::getGlobalMasterMixer());
    if (dsm) dsm->show();
}

void KMixWindow::newMixerShown(int /*tabIndex*/ ) {
    KMixerWidget* kmw = (KMixerWidget*)m_wsMixers->currentWidget();
    if (kmw) {
        setWindowTitle( kmw->mixer()->readableName() );
        if ( ! m_dontSetDefaultCardOnStart )
            m_defaultCardOnStart = kmw->getGuiprof()->getId();
        // As switching the tab does NOT mean switching the master card, we do not need to update dock icon here.
        // It would lead to unnecesary flickering of the (complete) dock area.
    }
}



#include "kmix.moc"
