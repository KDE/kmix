/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2003 Sven Leiber <s.leiber@web.de>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <kaction.h>
#include <klocale.h>
#include <kapplication.h>
#include <kpanelapplet.h>
#include <kpopupmenu.h>
#include <kglobalsettings.h>
#include <kdialog.h>
#include <kaudioplayer.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qapplication.h>
#include <qcursor.h>
#include <qtooltip.h>
#include <X11/Xlib.h>
#include <fixx11h.h>

#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixdockwidget.h"
#include "kwin.h"
#include "viewdockareapopup.h"

KMixDockWidget::KMixDockWidget( Mixer *mixer, QWidget *parent, const char *name, bool volumePopup )
    : KSystemTray( parent, name ),
      m_mixer(mixer),
      _dockAreaPopup(0L),
      _audioPlayer(0L),
      _playBeepOnVolumeChange(false), // disabled due to triggering a "Bug"
      _oldToolTipValue(-1),
      _oldPixmapType('-'),
      _volumePopup(volumePopup)
{
    createMasterVolWidget();
    connect(this, SIGNAL(quitSelected()), kapp, SLOT(quitExtended()));
}

KMixDockWidget::~KMixDockWidget()
{
    delete _audioPlayer;
    delete _dockAreaPopup;
}

void
KMixDockWidget::createMasterVolWidget()
{
    if (m_mixer == 0) {
        // In case that there is no mixer installed, there will be no newVolumeLevels() signal's
        // Thus we prepare the dock areas manually
        setVolumeTip();
        updatePixmap();
        return;
    }

    // create devices

    _dockAreaPopup = new ViewDockAreaPopup(0, "dockArea", m_mixer, 0, this);
    _dockAreaPopup->createDeviceWidgets();
    /* We are setting up 3 connections:
     * Refreshig the _dockAreaPopup (not anymore neccesary, because ViewBase already does it)
     * Refreshing the Tooltip
     * Refreshing the Icon
     *
     */
    //    connect( m_mixer, SIGNAL(newVolumeLevels()), _dockAreaPopup, SLOT(refreshVolumeLevels()) );
    connect( m_mixer, SIGNAL(newVolumeLevels()), this, SLOT(setVolumeTip() ) );
    connect( m_mixer, SIGNAL(newVolumeLevels()), this, SLOT(updatePixmap() ) );

   // Setup volume preview
   if ( _playBeepOnVolumeChange ) {
        _audioPlayer = new KAudioPlayer("KDE_Beep_Digital_1.ogg");
	// !! it would be better to connect the MixDevice, but it is not yet implemented
        connect(_dockAreaPopup->getMdwHACK(),
                SIGNAL(newVolume(int, Volume)),
                _audioPlayer,
                SLOT(play()));
   }
}



void
KMixDockWidget::setVolumeTip()
{
    MixDevice *md = 0;
    if ( _dockAreaPopup != 0 ) {
        md = _dockAreaPopup->dockDevice();
    }
    QString tip = "";

    int newToolTipValue = 0;
    if ( md == 0 )
    {
        tip = i18n("Mixer cannot be found"); // !! text could be reworked
	newToolTipValue = -2;
    }
    else
    {
        long val = -1;
        if ( md->maxVolume() != 0 ) {
	    val = (md->getAvgVolume()*100 )/( md->maxVolume() );
        }
	newToolTipValue = val + 10000*md->isMuted();
	if ( _oldToolTipValue != newToolTipValue ) {
	    tip = i18n( "Volume at %1%" ).arg( val );
	    if ( md->isMuted() ) {
		tip += i18n( " (Muted)" );
	    }
	}
	// create a new "virtual" value. With that we see "volume changes" as well as "muted changes"
	newToolTipValue = val + 10000*md->isMuted();
    }

    // The actual updating is only done when the "toolTipValue" was changed
    if ( newToolTipValue != _oldToolTipValue ) {
	// changed (or completely new tooltip)
	if ( _oldToolTipValue >= 0 ) {
	    // there was an old Tooltip: remove it
	    QToolTip::remove(this);
	}
	QToolTip::add(this, tip);
    }
    _oldToolTipValue = newToolTipValue;
}

void
KMixDockWidget::updatePixmap()
{
    MixDevice *md = 0;
    if ( _dockAreaPopup != 0 ) {
        md = _dockAreaPopup->dockDevice();
    }
    char newPixmapType;
    if ( md == 0 )
    {
	newPixmapType = 'e';
    }
    else if ( md->isMuted() )
    {
	newPixmapType = 'm';
    }
    else
    {
	newPixmapType = 'd';
    }


    if ( newPixmapType != _oldPixmapType ) {
	// Pixmap must be changed => do so
	switch ( newPixmapType ) {
	case 'e': setPixmap( loadIcon( "kmixdocked_error" ) ); break;
	case 'm': setPixmap( loadIcon( "kmixdocked_mute"  ) ); break;
	case 'd': setPixmap( loadIcon( "kmixdocked"       ) ); break;
	}
    }

    _oldPixmapType = newPixmapType;
}

void
KMixDockWidget::mousePressEvent(QMouseEvent *me)
{
	if ( _dockAreaPopup == 0 ) {
		return KSystemTray::mousePressEvent(me);
	}

        // esken: Due to overwhelming request, LeftButton shows the ViewDockAreaPopup, if configured
        //        to do so. Otherwise the main window will be shown.
	if ( me->button() == LeftButton )
	{
		if ( ! _volumePopup ) {
                    // Case 1: User wants to show main window => This is the KSystemTray default action
		    return KSystemTray::mousePressEvent(me);
		}

                // Case 2: User wants to show volume popup
		if ( _dockAreaPopup->isVisible() )
		{
			_dockAreaPopup->hide();
			return;
		}
		
		QRect desktop = KGlobalSettings::desktopGeometry(this);

		int w = _dockAreaPopup->width();
		int h = _dockAreaPopup->height();
		int x = this->mapToGlobal( QPoint( 0, 0 ) ).x() - this->width()/2;
		int y = this->mapToGlobal( QPoint( 0, 0 ) ).y() - h + this->height();
		if ( y - h < 0 )
			y = y + h - this->height();
	
		_dockAreaPopup->move(x, y);  // so that the mouse is outside of the widget
		_dockAreaPopup->show();
                // !! change back the next line after detemining how to do it properly
		XWarpPointer( _dockAreaPopup->x11Display(), None, _dockAreaPopup->handle(), 0,0,0,0, w/2, h/2 - 16 );
		
		QWidget::mousePressEvent(me); // KSystemTray's shouldn't do the default action for this
		return;
	} // LeftMouseButton pressed
	else {
		KSystemTray::mousePressEvent(me);
	} // Other MouseButton pressed
	
}

void
KMixDockWidget::mouseReleaseEvent( QMouseEvent *me )
{
	
    KSystemTray::mouseReleaseEvent(me);
}

void
KMixDockWidget::wheelEvent(QWheelEvent *e)
{
  MixDevice *md = 0;
  if ( _dockAreaPopup != 0 ) {
      md = _dockAreaPopup->dockDevice();
  }
  if ( md != 0 )
  {
    Volume vol = md->getVolume();
    int inc = vol.maxVolume() / 20;

    if ( inc == 0 ) inc = 1;

    for ( int i = 0; i < vol.channels(); i++ ) {
        int newVal = vol[i] + (inc * (e->delta() / 120));
        if( newVal < 0 ) newVal = 0;
        vol.setVolume( (Volume::ChannelID)i, newVal < vol.maxVolume() ? newVal : vol.maxVolume() );
    }

    if ( _playBeepOnVolumeChange ) {
        _audioPlayer->play();
    }
    md->getVolume().setVolume(vol);
    m_mixer->commitVolumeChange(md);
    // refresh the toolTip (Qt removes it on a MouseWheel event)
    // Mhhh, it doesn't work. Qt does not show it again.
    //setVolumeTip();
    setVolumeTip();
    // Simulate a mouse move to make Qt show the tooltip again
    QApplication::postEvent( this, new QMouseEvent( QEvent::MouseMove, QCursor::pos(), Qt::NoButton, Qt::NoButton ) );

  }
}

void
KMixDockWidget::contextMenuAboutToShow( KPopupMenu* /* menu */ )
{
    KAction* showAction = actionCollection()->action("minimizeRestore");
    if ( parentWidget() && showAction )
    {
        if ( parentWidget()->isVisible() )
        {
            showAction->setText( i18n("Hide Mixer Window") );
        }
        else
        {
            showAction->setText( i18n("Show Mixer Window") );
        }
    }
}

#include "kmixdockwidget.moc"

