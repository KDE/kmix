/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#include <qtooltip.h>
#include <X11/Xlib.h>
#include <fixx11h.h>

#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixdockwidget.h"
#include "kwin.h"

//-----------------------------------------------------------------------
// MasterVolume

KMixMasterVolume::KMixMasterVolume(QWidget* parent, const char* name, Mixer* mixer, KMixDockWidget *dockW)
    : QVBox(parent, name, WStyle_Customize | WType_Popup), dock(dockW)
{
    setFrameStyle(QFrame::PopupPanel);
    setMargin(KDialog::marginHint());

    MixDevice *masterDevice = (*mixer)[mixer->masterDevice()];
    mdw = new MixDeviceWidget( mixer, masterDevice, false, false,
                                false, KPanelApplet::Up, this, NULL,
                                masterDevice->name().latin1() );
    resize(sizeHint());
    installEventFilter( mdw );
}

void 
KMixMasterVolume::mousePressEvent(QMouseEvent *e)
{
  // this is very tricky:
  // if we hide this popup here, the dockWidget receives the event.
  // if we wouldn't hide this popup, the dockWidget doesn't receive this event
  // and could therefore not hide this popup
  // solution: we hide this popup here, but tell the dockWidget to ignore the
  // mouseRelease event - otherwise it would show the popup again.
  // We must to do this only for the "LeftButton", otherwise we would ignore events after
  // a right-click (which shows the context menu)
  if ( dock->hasMouse() ) 
  {
	  if ( e->button() == MidButton)  
		  dock->ignoreNextEvent();
	  hide();
	  return;
  }
  QVBox::mousePressEvent(e);  // will hide this popup
}

//-----------------------------------------------------------------------
// DockWidget

KMixDockWidget::KMixDockWidget( Mixer *mixer, QWidget *parent, const char *name )
    : KSystemTray( parent, name ),
      m_mixer(mixer),
      masterVol(0L),
      audioPlayer(0L),
      m_playBeepOnVolumeChange(false), // disabled due to triggering a "Bug"
      m_ignoreNextEvent(false)
{
    createMasterVolWidget();
    connect(this, SIGNAL(quitSelected()), kapp, SLOT(quitExtended()));
}

KMixDockWidget::~KMixDockWidget()
{
    delete audioPlayer;
    delete masterVol;
}

void 
KMixDockWidget::createMasterVolWidget()
{
    if (!m_mixer)
        return;

   // create devices
   MixDevice *masterDevice = (*m_mixer)[ m_mixer->masterDevice() ];

   masterVol = new KMixMasterVolume(0, "masterVol", m_mixer, this);
   connect(masterVol->mixerWidget(), SIGNAL(newVolume(int, Volume)),
           SLOT(setVolumeTip(int, Volume)));
   setVolumeTip(0, masterDevice->getVolume());

   // Setup volume preview
   if ( m_playBeepOnVolumeChange ) {
        audioPlayer = new KAudioPlayer("KDE_Beep_ShortBeep.wav");
        connect(masterVol->mixerWidget(), SIGNAL(newVolume(int, Volume)), audioPlayer, SLOT(play()));
   }
}

void 
KMixDockWidget::setVolumeTip(int, Volume vol)
{
    MixDevice *masterDevice = ( *m_mixer )[ m_mixer->masterDevice() ];
    QString tip = i18n( "Volume at %1%" ).arg( ( vol.getVolume( 0 )*100 )/( vol.maxVolume() ) );
    if ( masterDevice->isMuted() )
        tip += i18n( " (Muted)" );

    QToolTip::remove(this);
    QToolTip::add(this, tip);
}

void 
KMixDockWidget::updatePixmap()
{
    MixDevice *masterDevice = ( *m_mixer )[ m_mixer->masterDevice() ];

    if ( masterDevice->isMuted() )
        setPixmap( loadIcon( "kmixdocked_mute" ) );
    else
        setPixmap( loadIcon( "kmixdocked" ) );
}

void 
KMixDockWidget::setErrorPixmap()
{
    setPixmap( loadIcon( "kmixdocked_error" ) );
}

void 
KMixDockWidget::ignoreNextEvent()
{
  m_ignoreNextEvent = true;
}

void 
KMixDockWidget::mousePressEvent(QMouseEvent *me)
{
	// esken: exchanged LeftButton and MidButton, because helio changed mousePressEvent() to MidButton.
	//        And using MidButton for showing TrayVolumeControl is more logical (you use the MidButton wheel!)
	if ( me->button() == LeftButton )
	{
		return KSystemTray::mousePressEvent(me);
	}
	else if ( me->button() == MidButton )
	{
		if ( m_ignoreNextEvent )
		{
			m_ignoreNextEvent = false;
			return;
		}
		
		if ( masterVol->isVisible() )
		{
			masterVol->hide();
			return;
		}
		
		QRect desktop = KGlobalSettings::desktopGeometry(this);
		int sw = desktop.width();
		int sh = desktop.height();
		int sx = desktop.x();
		int sy = desktop.y();
		int x = me->globalPos().x();
		int y = me->globalPos().y();
		y -= masterVol->geometry().height();
		int w = masterVol->width();
		int h = masterVol->height();
		
		if (x+w > sw)
			x = me->globalPos().x() - w - 2;
		if (y+h > sh)
			y = me->globalPos().y() - h - 2;
		if (x < sx)
			x = me->globalPos().x() + 2;
		if (y < sy)
			y = me->globalPos().y() + 2;
		
		masterVol->move(x, y);  // so that the mouse is outside of the widget
		masterVol->show();
		XWarpPointer(masterVol->x11Display(), None, masterVol->handle(), 0,0,0,0, w/2, h/2);
		
		QWidget::mousePressEvent(me); // KSystemTray's shouldn't do the default action for this
		return;
	}
	else
		KSystemTray::mousePressEvent(me);
	
}

void 
KMixDockWidget::mouseReleaseEvent( QMouseEvent *me )
{
	
    KSystemTray::mouseReleaseEvent(me);
}

void 
KMixDockWidget::wheelEvent(QWheelEvent *e)
{
    MixDevice *masterDevice = (*m_mixer)[m_mixer->masterDevice()];
    Volume vol = masterDevice->getVolume();
    int inc = vol.maxVolume() / 20;

    if ( inc == 0 ) inc = 1;

    for ( int i = 0; i < vol.channels(); i++ ) {
        int newVal = vol[i] + (inc * (e->delta() / 120));
        if( newVal < 0 ) newVal = 0;
        vol.setVolume( i, newVal < vol.maxVolume() ? newVal : vol.maxVolume() );
    }

    if ( m_playBeepOnVolumeChange ) {
        audioPlayer->play();
    }
    masterDevice->setVolume(vol);
    m_mixer->writeVolumeToHW(masterDevice->num(), vol);
    setVolumeTip(masterDevice->num(), vol);
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

// vim: ts=3 sw=3
