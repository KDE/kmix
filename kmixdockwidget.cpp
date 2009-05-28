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

#include <kaction.h>
#include <klocale.h>
#include <kapplication.h>
#include <kmenu.h>
#include <kurl.h>
#include <kglobalsettings.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kwindowsystem.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <qapplication.h>
#include <qcursor.h>
#include <QDesktopWidget>
#include <QMouseEvent>

#ifdef Q_WS_X11
#include <fixx11h.h>
#endif

#include <Phonon/MediaObject>

#include "dialogselectmaster.h"
#include "kmix.h"
#include "kmixdockwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"
#include "mixertoolbox.h"
#include "viewdockareapopup.h"

KMixDockWidget::KMixDockWidget(KMixWindow* parent, QWidget *referenceWidget, bool volumePopup )
    : KNotificationItem( referenceWidget ),
      _audioPlayer(0L),
      _playBeepOnVolumeChange(false), // disabled due to triggering a "Bug"
      _oldToolTipValue(-1),
      _oldPixmapType('-'),
      _volumePopup(volumePopup)
      , _kmixMainWindow(parent)
{
   setToolTipIconByName("kmix");
   setCategory(Hardware);
   setStatus(Active);
   m_mixer = Mixer::getGlobalMasterMixer();  // ugly, but we'll live with that for now
   createMasterVolWidget();
   createActions();
   connect(this, SIGNAL(scrollRequested(int,Qt::Orientation)), this, SLOT(trayWheelEvent(int)));
   connect(this, SIGNAL(activateRequested(bool,QPoint)), this, SLOT(moveVolumePopup(bool,QPoint)));
   connect(this, SIGNAL(secondaryActivateRequested(QPoint)), this, SLOT(dockMute()));
   connect(contextMenu(), SIGNAL(aboutToShow()), this, SLOT(contextMenuAboutToShow()));

   ViewDockAreaPopup* dockAreaPopup = qobject_cast<ViewDockAreaPopup*>(referenceWidget);
   if ( dockAreaPopup ) {
       KWindowSystem::setState( dockAreaPopup->winId(), NET::StaysOnTop | NET::SkipTaskbar | NET::SkipPager );
   }
}

KMixDockWidget::~KMixDockWidget()
{
    delete _audioPlayer;
}

void KMixDockWidget::createActions()
{
   QMenu *menu = contextMenu();

   MixDevice* md = Mixer::getGlobalMasterMD();
  if ( md != 0 && md->playbackVolume().hasSwitch() ) {
    // Put "Mute" selector in context menu
    KToggleAction *action = actionCollection()->add<KToggleAction>( "dock_mute" );
    action->setText( i18n( "M&ute" ) );
    connect(action, SIGNAL(triggered(bool) ), SLOT( dockMute() ));
    QAction *a = actionCollection()->action( "dock_mute" );
    if ( a ) menu->addAction( a );
}

  // Put "Select Master Channel" dialog in context menu
  if ( m_mixer != 0 ) {
  QAction *action = actionCollection()->addAction( "select_master" );
  action->setText( i18n("Select Master Channel...") );
  connect(action, SIGNAL(triggered(bool) ), SLOT(selectMaster()));
  QAction *a2 = actionCollection()->action( "select_master" );
  if (a2) menu->addAction( a2 );
  }

   // Setup volume preview
  if ( _playBeepOnVolumeChange ) {
    _audioPlayer = Phonon::createPlayer(Phonon::MusicCategory);
    _audioPlayer->setParent(this);
  }
}


void
KMixDockWidget::createMasterVolWidget()
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

    m_mixer->readSetFromHWforceUpdate();  // after changing the master device, make sure to re-read (otherwise no "changed()" signals might get sent by the Mixer
    /* With the recently introduced QSocketNotifier stuff, we can't rely on regular timer updates
       any longer. Also the readSetFromHWforceUpdate() won't be enough. As a workaround, we trigger
       all "repaints" manually here.
       The call to m_mixer->readSetFromHWforceUpdate() is most likely superfluous, even if we don't use QSocketNotifier (e.g. in backends OSS, Solaris, ...)
     */
    setVolumeTip();
    updatePixmap();
    /* We are setting up 3 connections:
     * Refreshig the _dockAreaPopup (not anymore necessary, because ViewBase already does it)
     * Refreshing the Tooltip
     * Refreshing the Icon
     */
    //    connect( m_mixer, SIGNAL(controlChanged()), _dockAreaPopup, SLOT(refreshVolumeLevels()) );
    connect( m_mixer, SIGNAL(controlChanged()), this, SLOT(setVolumeTip() ) );
    connect( m_mixer, SIGNAL(controlChanged()), this, SLOT(updatePixmap() ) );
}

void KMixDockWidget::selectMaster()
{
   DialogSelectMaster* dsm = new DialogSelectMaster(m_mixer);
   connect ( dsm, SIGNAL(newMasterSelected(QString&, QString&)), SLOT( handleNewMaster(QString&, QString&)) );
   connect ( dsm, SIGNAL(newMasterSelected(QString&, QString&)), SIGNAL( newMasterSelected()) );
   dsm->show();
    // !! The dialog is modal. Does it delete itself?
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
/*
    Mixer *mixer = MixerToolBox::instance()->find(mixerID);
    if ( mixer == 0 ) {
        kError(67100) << "KMixDockWidget::createPage(): Invalid Mixer (mixerID=" << mixerID << ")" << endl;
        return; // can not happen
   }
   m_mixer = mixer;
   //Mixer::setGlobalMaster(mixer->id(), control_id); // We must save this information "somewhere".
   createMasterVolWidget();
*/
}


void
KMixDockWidget::setVolumeTip()
{
    MixDevice *md = Mixer::getGlobalMasterMD();
    QString tip = "";
    int newToolTipValue = 0;

    if ( md == 0 )
    {
        tip = i18n("Mixer cannot be found"); // !! text could be reworked
        newToolTipValue = -2;
    }
    else
    {
        // Playback volume will be used for the DockIcon if available.
        // This heuristic is "good enough" for the DockIcon for now.
        long val = 0;
        Volume& vol       = md->playbackVolume();
        if (! vol.hasVolume() ) {
           vol = md->captureVolume();
        }
        if ( vol.hasVolume() && vol.maxVolume() != 0 ) {
            val = (vol.getAvgVolume(Volume::MMAIN)*100 )/( vol.maxVolume() );
        }

        // create a new "virtual" value. With that we see "volume changes" as well as "muted changes"
        newToolTipValue = val;
        if ( md->isMuted() ) newToolTipValue += 10000;
        if ( _oldToolTipValue != newToolTipValue ) {
            tip = i18n( "Volume at %1%", val );
            if ( md->playbackVolume().hasSwitch() && md->isMuted() ) {
                tip += i18n( " (Muted)" );
            }
        }
    }

    // The actual updating is only done when the "toolTipValue" was changed
    if ( newToolTipValue != _oldToolTipValue ) {
        // changed (or completely new tooltip)
        setToolTipTitle(tip);
    }
    _oldToolTipValue = newToolTipValue;
}

void
KMixDockWidget::updatePixmap()
{
    MixDevice *md = Mixer::getGlobalMasterMD();

    char newPixmapType;
    if ( md == 0 )
    {
        newPixmapType = 'e';
    }
    else if ( md->playbackVolume().hasSwitch() && md->isMuted() )
    {
        newPixmapType = 'm';
    }
    else
    {
        Volume& vol = md->playbackVolume();
        if (! vol.hasVolume() ) {
            vol = md->captureVolume();
        }
        long absoluteVolume    = vol.getAvgVolume(Volume::MALL);
        int percentage         = vol.percentage(absoluteVolume);
        if      ( percentage < 25 ) newPixmapType = '1';  // Hint: also negative-values
        else if ( percentage < 75 ) newPixmapType = '2';
        else                        newPixmapType = '3';
   }


   if ( newPixmapType != _oldPixmapType ) {
      // Pixmap must be changed => do so
      switch ( newPixmapType ) {
         case 'e': setIconByName( "kmixdocked_error" ); break;
         case 'm': setIconByName( "audio-volume-muted"  ); break;
         case '1': setIconByName( "audio-volume-low"  ); break;
         case '2': setIconByName( "audio-volume-medium" ); break;
         case '3': setIconByName( "audio-volume-high" ); break;
      }
   }

   _oldPixmapType = newPixmapType;
}


void KMixDockWidget::moveVolumePopup(bool activated, const QPoint &pos)
{
   ViewDockAreaPopup* dockAreaPopup = qobject_cast<ViewDockAreaPopup*>(parent());
   if ( !dockAreaPopup || !activated ) {
      // If the associated parent os the MainWindow (and not the ViewDockAreaPopup), we return.
      return;
   }

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

   // Now handle Multihead displays. And also make sure that the dialog is not
   // moved out-of-the screen on the right (see Bug 101742).
   const QDesktopWidget* vdesktop = QApplication::desktop();
   const QRect& vScreenSize = vdesktop->screenGeometry(dockAreaPopup);
   //const QRect screenGeometry(const QWidget *widget) const
   if ( (x+dockAreaPopup->width()) > (vScreenSize.width() + vScreenSize.x()) ) {
       // move horizontally, so that it is completely visible
       dockAreaPopup->move(vScreenSize.width() + vScreenSize.x() - dockAreaPopup->width() -1 , y);
   }
   else if ( x < vScreenSize.x() ) {
       // horizontally out-of bound
       dockAreaPopup->move(vScreenSize.x(), y);
   }
   // the above stuff could also be implemented vertically

   KWindowSystem::setState( dockAreaPopup->winId(), NET::StaysOnTop | NET::SkipTaskbar | NET::SkipPager );
}

// void
// KMixDockWidget::trayToolTipEvent(QHelpEvent *e ) {
//    kDebug(67100) << "trayToolTipEvent" ;
//    setVolumeTip();
// }

void
KMixDockWidget::trayWheelEvent(int delta)
{
  MixDevice *md = Mixer::getGlobalMasterMD();
  if ( md != 0 )
  {
      Volume vol = md->playbackVolume();
      if ( md->playbackVolume().hasVolume() )
         vol = md->playbackVolume();
      else
         vol = md->captureVolume();

      int inc = vol.maxVolume() / 20;

    if ( inc < 1 ) inc = 1;

    for ( int i = 0; i < vol.count(); i++ ) {
        int newVal = vol[i] + (inc * (delta / 120));
        if( newVal < 0 ) newVal = 0;
        vol.setVolume( (Volume::ChannelID)i, newVal < vol.maxVolume() ? newVal : vol.maxVolume() );
    }

    if ( _playBeepOnVolumeChange ) {
        QString fileName("KDE_Beep_Digital_1.ogg");
        _audioPlayer->setCurrentSource(fileName);
        _audioPlayer->play();
    }
      if ( md->playbackVolume().hasVolume() )
         md->playbackVolume().setVolume(vol);
      else
         md->captureVolume().setVolume(vol);
      m_mixer->commitVolumeChange(md);
    setVolumeTip();
  }
}

void
KMixDockWidget::dockMute()
{
   MixDevice *md = Mixer::getGlobalMasterMD();
   if ( md != 0 ) {
      md->setMuted( !md->isMuted() );
      md->mixer()->commitVolumeChange( md );
   }
}

void
KMixDockWidget::contextMenuAboutToShow()
{
    QAction* showAction = actionCollection()->action("minimizeRestore");
    if ( associatedWidget() && showAction )
    {
        if ( associatedWidget()->isVisible() )
        {
            showAction->setText( i18n("Hide Mixer Window") );
        }
        else
        {
            showAction->setText( i18n("Show Mixer Window") );
        }
    }

    // Enable/Disable "Muted" menu item
    MixDevice* md = Mixer::getGlobalMasterMD();
    KToggleAction *dockMuteAction = static_cast<KToggleAction*>(actionCollection()->action("dock_mute"));
    //kDebug(67100) << "---> md=" << md << "dockMuteAction=" << dockMuteAction << "isMuted=" << md->isMuted();
    if ( md != 0 && dockMuteAction != 0 ) {
        bool hasSwitch = md->playbackVolume().hasSwitch();
        dockMuteAction->setEnabled( hasSwitch );
        dockMuteAction->setChecked( hasSwitch && md->isMuted() );
    }
}

#include "kmixdockwidget.moc"

