/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include <klocale.h>
#include <kled.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>
#include <kdebug.h>

#include <qobject.h>
#include <qcursor.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qwmatrix.h>

#include "mixer.h"
#include "mixdevicewidget.h"
#include "viewbase.h"
#include "kledbutton.h"
#include "ksmallslider.h"
#include "verticaltext.h"

/**
 * Class that represents a single mix device, inlcuding PopUp, muteLED, ...
 * Used in KMix main window and DockWidget and PanelApplet.
 * It can be configured to include or exclude the recordLED and the muteLED.
 * The direction (horizontal, vertical) can be configured and whether it should
 * be "small"  (uses KSmallSlider instead of QSlider then).
 */
MixDeviceWidget::MixDeviceWidget(Mixer *mixer, MixDevice* md,
                                 bool small, Qt::Orientation orientation,
                                 QWidget* parent, ViewBase* mw, const char* name) :
   QWidget( parent, name ), m_mixer(mixer), m_mixdevice( md ), m_mixerwidget( mw ),
   m_disabled( false ), _orientation( orientation ), m_small( small )
{
   _mdwActions = new KActionCollection( this );
   m_keys = new KGlobalAccel( this, "Keys" );
}

MixDeviceWidget::~MixDeviceWidget()
{
}


void MixDeviceWidget::addActionToPopup( KAction *action )
{
	_mdwActions->insert( action );
}


bool MixDeviceWidget::isDisabled() const
{
   return m_disabled;
}


KGlobalAccel *MixDeviceWidget::keys( void )
{
    return m_keys;
}

void MixDeviceWidget::defineKeys()
{
   if (m_keys) {
      KKeyDialog::configure(m_keys, 0, false);
      // The keys are saved in KMixerWidget::saveConfig, see kmixerwidget.cpp
      m_keys->updateConnections();
   }
}

void MixDeviceWidget::volumeChange( int ) { /* is virtual */ }
void MixDeviceWidget::setDisabled( bool ) { /* is virtual */ }
void MixDeviceWidget::setVolume( int /*channel*/, int /*vol*/ ) { /* is virtual */ }
void MixDeviceWidget::setVolume( Volume /*vol*/ ) { /* is virtual */ }
void MixDeviceWidget::update() { /* is virtual */ }
void MixDeviceWidget::showContextMenu() { /* is virtual */ }
void MixDeviceWidget::setColors( QColor , QColor , QColor ) { /* is virtual */ }
void MixDeviceWidget::setIcons( bool ) { /* is virtual */ }
void MixDeviceWidget::setLabeled( bool ) { /* is virtual */ }
void MixDeviceWidget::setMutedColors( QColor , QColor , QColor ) { /* is virtual */ }




void MixDeviceWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      showContextMenu();
   else {
       QWidget::mousePressEvent(e);
   }
}


#include "mixdevicewidget.moc"
