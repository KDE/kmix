/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#include <stdlib.h>

#include <qlayout.h>
#include <qpixmap.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kaction.h>
#include <qpushbutton.h>
#include <kapp.h>
#include <qwmatrix.h>
#include <kiconloader.h>
#include <qtimer.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixapplet.h"

KMixApplet::KMixApplet( Mixer *mixer, QWidget *parent, const char* name )
   : KPanelApplet( parent, name )
{
   // scale icon
   QPixmap icon = BarIcon("kmixdocked");
   QWMatrix t;
   t = t.scale( 10.0/icon.width(), 10.0/icon.height() );

   // create show/hide button      
   m_button = new QPushButton( icon.xForm( t ), "*", this );
   m_button->setFixedWidth( 15 );   
   connect( m_button, SIGNAL(clicked()), this, SLOT(showButton()));

   // init mixer widget
   m_mixerWidget = new KMixerWidget( mixer, true, true, this );
   
   // ulgy hack to avoid sending to many updateSize requests to kicker that would freeze it
   m_timer = new QTimer( this );
   connect( m_timer, SIGNAL(timeout()), this, SLOT(updateSize()) );
   connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(updateLayout()));
}

void KMixApplet::updateLayout()
{
   if ( !m_timer->isActive() )
      m_timer->start( 100, TRUE );
}

int KMixApplet::widthForHeight(int )
{
  return m_mixerWidget->minimumWidth() + m_button->minimumWidth() + 1;
}
 
int KMixApplet::heightForWidth(int )
{
  return m_mixerWidget->minimumHeight();
}    

void KMixApplet::removedFromPanel()
{
   emit closeApplet( this );
}

void KMixApplet::resizeEvent(QResizeEvent *e) 
{      
   KPanelApplet::resizeEvent( e );

   m_button->setGeometry( 0 ,0, m_button->minimumWidth(), height() );
   //m_mixerWidget->setOrientation( orientation()==Left || orientation()==Right ); 
   m_mixerWidget->setIcons( height()>=32 );
   m_mixerWidget->setGeometry( m_button->minimumWidth() + 1, 0, 
			       width() - m_button->minimumWidth(), height() );
}
