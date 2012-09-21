/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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

#include "gui/kmixdockwidget.h"

#include <kaction.h>
#include <klocale.h>
#include <kmenu.h>
#include <kdebug.h>
#include <kwindowsystem.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <QDesktopWidget>
#include <QApplication>

#include "gui/dialogselectmaster.h"
#include "apps/kmix.h"
#include "core/mixer.h"
#include "gui/mixdevicewidget.h"
#include "core/mixertoolbox.h"
#include "gui/viewdockareapopup.h"

void MetaMixer::reset()
{
    disconnect(m_mixer, SIGNAL(controlChanged()), this, SIGNAL(controlChanged()));
    m_mixer = Mixer::getGlobalMasterMixer();
    // after changing the master device, make sure to re-read (otherwise no "changed()" signals might get sent by the Mixer
    m_mixer->readSetFromHWforceUpdate();
    connect(m_mixer, SIGNAL(controlChanged()), this, SIGNAL(controlChanged()));
    emit controlChanged(); // Triggers UI updates accordingly
}

KMixDockWidget::KMixDockWidget(KMixWindow* parent, bool volumePopup)
    : KStatusNotifierItem(parent)
    , _oldToolTipValue(-1)
    , _oldPixmapType('-')
    , _volumePopup(volumePopup)
    , _kmixMainWindow(parent)
    , _contextMenuWasOpen(false)
{
    setToolTipIconByName("kmix");
    setTitle(i18n( "Volume Control"));
    setCategory(Hardware);
    setStatus(Active);

    m_metaMixer.reset();
    createMasterVolWidget();
    createActions();

    connect(this, SIGNAL(scrollRequested(int,Qt::Orientation)), this, SLOT(trayWheelEvent(int,Qt::Orientation)));
    connect(this, SIGNAL(secondaryActivateRequested(QPoint)), this, SLOT(dockMute()));

    connect(contextMenu(), SIGNAL(aboutToShow()), this, SLOT(contextMenuAboutToShow()));

#ifdef __GNUC__
#warning minimizeRestore usage is currently slightly broken in KMIx. This should be fixed before doing a release.
#endif
    // TODO minimizeRestore usage is currently a bit broken. It only works by chance

    if (_volumePopup) {
        kDebug() << "Construct the ViewDockAreaPopup and actions";
        _referenceWidget = new KMenu(parent);
        ViewDockAreaPopup* _referenceWidget2 = new ViewDockAreaPopup(_referenceWidget, "dockArea", Mixer::getGlobalMasterMixer(), 0, (GUIProfile*)0, parent);
        _referenceWidget2->createDeviceWidgets();
        connect(_referenceWidget2, SIGNAL(recreateMe()), _kmixMainWindow, SLOT(recreateDockWidget()));

        _volWA = new QWidgetAction(_referenceWidget);
        _volWA->setDefaultWidget(_referenceWidget2);
        _referenceWidget->addAction(_volWA);

        connect( &m_metaMixer, SIGNAL(controlChanged()), _referenceWidget2, SLOT(refreshVolumeLevels()) );
        //setAssociatedWidget(_referenceWidget);
        //setAssociatedWidget(_referenceWidget);  // If you use the popup, associate that instead of the MainWindow

        //setContextMenu(_referenceWidget2);
    } else {
        _volWA = 0;
        _referenceWidget = 0;
    }
}

KMixDockWidget::~KMixDockWidget()
{
    // Note: deleting _volWA also deletes its associated ViewDockAreaPopup (_referenceWidget) and prevents the
    //       action to be left with a dangling pointer.
    //       cesken: I adapted the patch from https://bugs.kde.org/show_bug.cgi?id=220621#c27 to branch /branches/work/kmix 
    delete _volWA;
}

void KMixDockWidget::createActions()
{
    QMenu *menu = contextMenu();

    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if ( md.get() != 0 && md->playbackVolume().hasSwitch() ) {
        // Put "Mute" selector in context menu
        KToggleAction *action = actionCollection()->add<KToggleAction>( "dock_mute" );
        action->setText( i18n( "M&ute" ) );
        connect(action, SIGNAL(triggered(bool)), SLOT(dockMute()));
        menu->addAction( action );
    }

    // Put "Select Master Channel" dialog in context menu
    QAction *action = actionCollection()->addAction( "select_master" );
    action->setText( i18n("Select Master Channel...") );
    action->setEnabled(m_metaMixer.hasMixer());
    connect(action, SIGNAL(triggered(bool)), SLOT(selectMaster()));
    menu->addAction( action );

    //Context menu entry to access phonon settings
    menu->addAction(_kmixMainWindow->actionCollection()->action("launch_kdesoundsetup"));
}

void KMixDockWidget::createMasterVolWidget()
{
     // Reset flags, so that the dock icon will be reconstructed
     _oldToolTipValue = -1;
     _oldPixmapType   = '-';

    if (Mixer::getGlobalMasterMD() == 0) {
        // In case that there is no mixer installed, there will be no controlChanged() signal's
        // Thus we prepare the dock areas manually
        setVolumeTip();
        updatePixmap();
        return;
    }
    // create devices

    setVolumeTip();
    updatePixmap();

    /* With the recently introduced QSocketNotifier stuff, we can't rely on regular timer updates
       any longer. Also the readSetFromHWforceUpdate() won't be enough. As a workaround, we trigger
       all "repaints" manually here.
       The call to m_mixer->readSetFromHWforceUpdate() is most likely superfluous, even if we don't use QSocketNotifier (e.g. in backends OSS, Solaris, ...)
     */
    connect( &m_metaMixer, SIGNAL(controlChanged()), this, SLOT(setVolumeTip()) );
    connect( &m_metaMixer, SIGNAL(controlChanged()), this, SLOT(updatePixmap()) );
}

void KMixDockWidget::selectMaster()
{
   DialogSelectMaster* dsm = new DialogSelectMaster(m_metaMixer.mixer());
   dsm->setAttribute(Qt::WA_DeleteOnClose, true);
   connect ( dsm, SIGNAL(newMasterSelected(QString&,QString&)), SLOT(handleNewMaster(QString&,QString&)) );
   connect ( dsm, SIGNAL(newMasterSelected(QString&,QString&)), SIGNAL(newMasterSelected()) );
   dsm->show();
}

void KMixDockWidget::handleNewMaster(QString& /*mixerID*/, QString& /*control_id*/)
{
   /* When a new master is selected, we will need to destroy *this instance of KMixDockWidget.
      Reason: This widget is a KSystemTrayIcon, and needs an associated QWidget. This is in
      our case the ViewDockAreaPopup instance. As that is recreated, we need to recrate ourself as well.
      The only chance to do so is outside, in fact it is done by KMixWindow::updateDocking(). This needs to
      use deleteLater(), as we are executing in a SLOT currently.
    */
   _kmixMainWindow->updateDocking();
}

void KMixDockWidget::update()
{
    m_metaMixer.reset();
    actionCollection()->action(QLatin1String("select_master"))->setEnabled(m_metaMixer.hasMixer());
}

int KMixDockWidget::getUserfriendlyVolumeLevel(const shared_ptr<MixDevice>& md)
{
	bool usePlayback = md->playbackVolume().hasVolume();
	Volume& vol = usePlayback ? md->playbackVolume() : md->captureVolume();
	bool isActive = usePlayback ? !md->isMuted() : md->isRecSource();
	int val = isActive ? vol.getAvgVolumePercent(Volume::MALL) : 0;
	return val;
}

void
KMixDockWidget::setVolumeTip()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    QString tip = "";
    int virtualToolTipValue = 0;

    if ( md.get() == 0 )
    {
        tip = i18n("Mixer cannot be found"); // !! text could be reworked
        virtualToolTipValue = -2;
    }
    else
    {
        // Playback volume will be used for the DockIcon if available.
        // This heuristic is "good enough" for the DockIcon for now.
		int val = getUserfriendlyVolumeLevel(md);
       	tip = i18n( "Volume at %1%", val );
        if ( md->isMuted() )
        	tip += i18n( " (Muted)" );

        // create a new "virtual" value. With that we see "volume changes" as well as "muted changes"
        virtualToolTipValue = val;
        if ( md->isMuted() )
        	virtualToolTipValue += 10000;
    }

    // The actual updating is only done when the "toolTipValue" was changed (to avoid flicker)
    if ( virtualToolTipValue != _oldToolTipValue )
    {
        // changed (or completely new tooltip)
        setToolTipTitle(tip);
    }
    _oldToolTipValue = virtualToolTipValue;
}

void
KMixDockWidget::updatePixmap()
{
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();

    char newPixmapType;
    if ( md == 0 )
    {
        newPixmapType = 'e';
    }
    else
    {
    	int percentage = getUserfriendlyVolumeLevel(md);
		if      ( percentage <= 0 ) newPixmapType = '0';  // Hint: also negative-values
		else if ( percentage < 25 ) newPixmapType = '1';
		else if ( percentage < 75 ) newPixmapType = '2';
		else                        newPixmapType = '3';
    }

   if ( newPixmapType != _oldPixmapType ) {
      // Pixmap must be changed => do so
      switch ( newPixmapType ) {
         case 'e': setIconByName( "kmixdocked_error" ); break;
         case 'm': 
         case '0': setIconByName( "audio-volume-muted"  ); break;
         case '1': setIconByName( "audio-volume-low"  ); break;
         case '2': setIconByName( "audio-volume-medium" ); break;
         case '3': setIconByName( "audio-volume-high" ); break;
      }
   }

   _oldPixmapType = newPixmapType;
}

void KMixDockWidget::activate(const QPoint &pos)
{
    kDebug() << "Activate at " << pos;

    bool showHideMainWindow = false;
    showHideMainWindow |= (_referenceWidget == 0);
    showHideMainWindow |= (pos.x() == 0  && pos.y() == 0);  // HACK. When the action comes from the context menu, the pos is (0,0)

    if ( showHideMainWindow ) {
        // Use default KStatusNotifierItem behavior if we are not using the dockAreaPopup
        kDebug() << "Use default KStatusNotifierItem behavior";
        setAssociatedWidget(_kmixMainWindow);
//        KStatusNotifierItem::activate(pos);
        KStatusNotifierItem::activate();
        return;
    }

    KMenu* dockAreaPopup =_referenceWidget; // TODO Refactor to use _referenceWidget directly
    kDebug() << "Skip default KStatusNotifierItem behavior";
    if ( dockAreaPopup->isVisible() ) {
        dockAreaPopup->hide();
        kDebug() << "dap is visible => hide and return";
        return;
    }

//    if (dockAreaPopup->isVisible()) {
//        contextMenu()->hide();
//        setAssociatedWidget(_kmixMainWindow);
//        KStatusNotifierItem::activate(pos);
//        kDebug() << "cm is visible => setAssociatedWidget(_kmixMainWindow)";
//        return;
//    }
    if ( false ) {}
    else {
        setAssociatedWidget(_referenceWidget);
        kDebug() << "cm is NOT visible => setAssociatedWidget(_referenceWidget)";

        dockAreaPopup->adjustSize();
        int h = dockAreaPopup->height();
        int x = pos.x() - dockAreaPopup->width()/2;
        int y = pos.y() - h;

        // kDebug() << "h="<<h<< " x="<<x << " y="<<y<< "gx="<< geometry().x() << "gy="<< geometry().y();

        if ( y < 0 ) {
            y = pos.y();
        }

        dockAreaPopup->move(x, y);  // so that the mouse is outside of the widget
        kDebug() << "moving to" << dockAreaPopup->size() << x << y;

        dockAreaPopup->show();

        // Now handle Multihead displays. And also make sure that the dialog is not
        // moved out-of-the screen on the right (see Bug 101742).
        const QDesktopWidget* vdesktop = QApplication::desktop();
        const QRect& vScreenSize = vdesktop->screenGeometry(dockAreaPopup);
        //const QRect screenGeometry(const QWidget *widget) const
        if ( (x+dockAreaPopup->width()) > (vScreenSize.width() + vScreenSize.x()) ) {
            // move horizontally, so that it is completely visible
            dockAreaPopup->move(vScreenSize.width() + vScreenSize.x() - dockAreaPopup->width() -1 , y);
            kDebug() << "Multihead: (case 1) moving to" << vScreenSize.x() << "," << vScreenSize.y();
        }
        else if ( x < vScreenSize.x() ) {
            // horizontally out-of bound
            dockAreaPopup->move(vScreenSize.x(), y);
            kDebug() << "Multihead: (case 2) moving to" << vScreenSize.x() << "," << vScreenSize.y();
        }
        // the above stuff could also be implemented vertically

        KWindowSystem::setState( dockAreaPopup->winId(), NET::StaysOnTop | NET::SkipTaskbar | NET::SkipPager );
    }
}


void
KMixDockWidget::trayWheelEvent(int delta,Qt::Orientation wheelOrientation)
{
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
	if ( md.get() == 0 )
		return;


      Volume &vol = ( md->playbackVolume().hasVolume() ) ?  md->playbackVolume() : md->captureVolume();
      int inc = vol.volumeSpan() / Mixer::VOLUME_STEP_DIVISOR;

    if ( inc < 1 ) inc = 1;

    if (wheelOrientation == Qt::Horizontal) // Reverse horizontal scroll: bko228780 
    	delta = -delta;

    long int cv = inc * (delta / 120 );
//    kDebug() << "twe: " << cv << " : " << vol;
    bool isInactive =  vol.isCapture() ? !md->isRecSource() : md->isMuted();
    kDebug() << "Operating on capture=" << vol.isCapture() << ", isInactive=" << isInactive;
	if ( cv > 0 && isInactive)
	{   // increasing from muted state: unmute and start with a low volume level
		if ( vol.isCapture())
			md->setRecSource(true);
		else
			md->setMuted(false);
		vol.setAllVolumes(cv);
	}
	else
	    vol.changeAllVolumes(cv);

//	kDebug() << "twe: " << cv << " : " << vol;

    md->mixer()->commitVolumeChange(md);
    setVolumeTip();
}


void
KMixDockWidget::dockMute()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if ( md )
    {
        md->toggleMute();
        md->mixer()->commitVolumeChange( md );
    }
}

void
KMixDockWidget::contextMenuAboutToShow()
{
   // KStatusNotifierItem::contextMenuAboutToShow();
    /*
    kDebug() << "<<< mm 1";
    QAction* showAction = actionCollection()->action("minimizeRestore");
    kDebug() << "<<< mm 2";
    if ( parent() && showAction )
    {
        if ( ((QWidget*)parent())->isVisible() ) // TODO isVisible() is not good enough here
        {
            showAction->setText( i18n("Hide Mixer Window") );
        }
        else
        {
            showAction->setText( i18n("Show Mixer Window") );
        }
        kDebug() << "<<< mm 3";
        //disconnect(showAction, 0, 0, 0);
        //connect(showAction, SIGNAL(triggered(bool),this,hideOrShowMainWindow());
    }
    */

    // Enable/Disable "Muted" menu item
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    KToggleAction *dockMuteAction = static_cast<KToggleAction*>(actionCollection()->action("dock_mute"));
    //kDebug(67100) << "---> md=" << md << "dockMuteAction=" << dockMuteAction << "isMuted=" << md->isMuted();
    if ( md != 0 && dockMuteAction != 0 ) {
    	Volume& vol = md->playbackVolume().hasVolume() ? md->playbackVolume() : md->captureVolume();
    	bool isInactive =  vol.isCapture() ? md->isMuted() : !md->isRecSource();
        bool hasSwitch = vol.hasSwitch();
        dockMuteAction->setEnabled( hasSwitch );
        dockMuteAction->setChecked( hasSwitch && !isInactive );
    }
    _contextMenuWasOpen = true;
}

#include "kmixdockwidget.moc"
