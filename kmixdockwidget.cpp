/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
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

#include <klocale.h>
#include <kapp.h>
#include <kpanelapplet.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kdialog.h>
#include <kconfig.h>

#include <qvbox.h>
#include <qtooltip.h>

#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixdockwidget.h"


KMixDockWidget::KMixDockWidget( Mixer *mixer,
				QWidget *parent, const char *name )
    : KSystemTray( parent, name ), m_mixer(mixer), masterVol(0L)
{
    createMasterVolWidget();
}

KMixDockWidget::~KMixDockWidget()
{
    delete masterVol;
}

void KMixDockWidget::createMasterVolWidget()
{
    if (!m_mixer)
	return;

   // create devices
   MixDevice *masterDevice = (*m_mixer)[m_mixer->masterDevice()];
//   MixDevice *masterDevice = m_mixer->getMixer(m_mixer->masterDevice();

   masterVol = new QVBox(0L, "masterVol", WStyle_Customize |
			 WType_Popup);
   masterVol->setFrameStyle(QFrame::PopupPanel);
   masterVol->setMargin(KDialog::marginHint());

   MixDeviceWidget *mdw =
       new MixDeviceWidget( m_mixer, masterDevice, false, false,
			    false, KPanelApplet::Up, masterVol,
			    masterDevice->name().latin1() );
   connect(mdw, SIGNAL(newVolume(int, Volume)),
	   this, SLOT(setVolumeTip(int, Volume)));
   setVolumeTip(0, masterDevice->getVolume());
   masterVol->resize(masterVol->sizeHint());
}

void KMixDockWidget::setVolumeTip(int, Volume vol)
{
    QToolTip::remove(this);
    QToolTip::add(this, i18n("Volume at %1%").arg(vol.getVolume(0)));
}

void KMixDockWidget::mousePressEvent(QMouseEvent *me)
{
    KConfig *config = kapp->config();
    config->setGroup(0);
    if( config->readBoolEntry("TrayVolumeControl", true ) )
        QWidget::mousePressEvent(me);
    else
        KSystemTray::mousePressEvent(me);
}

void KMixDockWidget::mouseReleaseEvent(QMouseEvent *me)
{
    if(!masterVol) {
        KSystemTray::mouseReleaseEvent(me);
        return;
    }
    KConfig *config = kapp->config();
    config->setGroup(0);
    if( config->readBoolEntry("TrayVolumeControl", true ) ) {
        if (me->button() == QMouseEvent::LeftButton &&
            !masterVol->isVisible()) {
            QWidget *desktop = QApplication::desktop();
            int sw = desktop->width();
            int sh = desktop->height();
            int sx = desktop->x();
            int sy = desktop->y();
            int x = me->globalPos().x();
            int y = me->globalPos().y();
            y -= masterVol->geometry().height();
            int w = masterVol->width();
            int h = masterVol->height();

            if (x+w > sw)
                x = me->globalPos().x()-w;
            if (y+h > sh)
                y = me->globalPos().y()-h;
            if (x < sx)
                x = me->globalPos().x();
            if (y < sy)
                y = me->globalPos().y();

	masterVol->move(x, y);
	masterVol->show();
        } else if (me->button() == QMouseEvent::LeftButton &&
                   masterVol->isVisible()) {
            masterVol->hide();
        } else {
            masterVol->hide();
            KSystemTray::mousePressEvent(me);
        }
    } else {
        KSystemTray::mouseReleaseEvent(me);
    }
}

void KMixDockWidget::mouseDoubleClickEvent(QMouseEvent *me)
{
    KSystemTray::mousePressEvent(me);
}

void KMixDockWidget::contextMenuAboutToShow( KPopupMenu* menu )
{
    for ( unsigned n=0; n<menu->count(); n++ )
    {
        if ( QString( menu->text( menu->idAt(n) ) )==i18n("&Quit") )
            menu->removeItemAt( n );
    }

    menu->insertItem( SmallIcon("exit"), i18n("&Quit" ), kapp, SLOT(quit()) );
}

#include "kmixdockwidget.moc"
