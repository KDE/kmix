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

#define BUTTONSIZE 15

KMixApplet::KMixApplet( Mixer *mixer, QString id, 
			QWidget *parent, const char* name )
   : KPanelApplet( id, KPanelApplet::Normal, 0, parent, name ), m_dockId( id ), m_lockedLayout( 0 )
{
   kdDebug() << "dockId = " << endl;

   // scale icon
   QPixmap icon = BarIcon("kmixdocked");
   QWMatrix t;
   t = t.scale( 10.0/icon.width(), 10.0/icon.height() );

   // create show/hide button      
   m_button = new QPushButton( icon.xForm( t ), QString::null, this );
   connect( m_button, SIGNAL(clicked()), this, SLOT(showButton()) );

   // init mixer widget
   m_mixerWidget = new KMixerWidget( mixer, true, true, this );
   
   // ulgy hack to avoid sending to many updateSize requests to kicker that would freeze it
   m_layoutTimer = new QTimer( this );
   connect( m_layoutTimer, SIGNAL(timeout()), this, SLOT(updateSize()) );
   connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(updateLayout()));

   //FIXME activate menu items
   //setActions(About | Help | Preferences);
}

void KMixApplet::updateLayout()
{
   if ( m_lockedLayout ) return;
   if ( !m_layoutTimer->isActive() )
      m_layoutTimer->start( 100, TRUE );
}

int KMixApplet::widthForHeight(int )
{
  m_lockedLayout++;  
  m_mixerWidget->setIcons( height()>=32 );
  m_lockedLayout--;
  return m_mixerWidget->minimumWidth() + BUTTONSIZE + 1;
}
 
int KMixApplet::heightForWidth(int width)
{
  m_lockedLayout++;
  m_mixerWidget->setIcons( height()>=32 );
  m_lockedLayout--;
  return BUTTONSIZE + width + 1 ;
}    

void KMixApplet::removedFromPanel()
{
   emit closeApplet( this );
}

void KMixApplet::resizeEvent(QResizeEvent *e) 
{      
   KPanelApplet::resizeEvent( e );

   m_lockedLayout++;
   if ( orientation()==Vertical )
   {
      m_button->setGeometry( 0 ,0, width(), BUTTONSIZE );
      m_mixerWidget->setGeometry( 0, BUTTONSIZE + 1, 
				  width(), height()-BUTTONSIZE - 1 );
   } else
   {
      m_button->setGeometry( 0 ,0, BUTTONSIZE, height() );   
      m_mixerWidget->setGeometry( BUTTONSIZE + 1, 0, 
				  width()-BUTTONSIZE-1, height() );
   }
   m_lockedLayout--;
}
