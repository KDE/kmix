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
#define MAXDEVICES 2
#define MAXCARDS 4


extern "C"
{
  KPanelApplet* init(QWidget *parent, const QString& configFile)
  {
     kdDebug() << "kmixapplet init" << endl;
     KGlobal::locale()->insertCatalogue("kmixapplet");
     return new KMixApplet(configFile, KPanelApplet::Normal,
			   KPanelApplet::About | KPanelApplet::Help | KPanelApplet::Preferences,
			   parent, "kmixapplet");
  }
}


int KMixApplet::s_instCount = 0;

KMixApplet::KMixApplet( const QString& configFile, Type t, int actions,
			QWidget *parent, const char *name )

   : KPanelApplet( configFile, t, actions, parent, name ), m_lockedLayout( 0 )
{
   // init static vars
   if ( !s_instCount )
   {
      // create update timer
      QTimer *s_timer = new QTimer;
      s_timer->start( 500 );

      // get mixer devices
      s_mixers.setAutoDelete( TRUE );
      for ( int dev=0; dev<MAXDEVICES; dev++ )
	 for ( int card=0; card<MAXCARDS; card++ )
	 {
	    Mixer *mixer = Mixer::getMixer( dev, card );
	    int mixerError = mixer->grab();
	    if ( mixerError!=0 )
	    {
	       delete mixer;	
	    } else
	    {
	       connect( s_timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
	       s_mixers.append( mixer );
	    }
	 }
   }

   s_instCount++;

   // ulgy hack to avoid sending to many updateSize requests to kicker that would freeze it
   m_layoutTimer = new QTimer( this );
   connect( m_layoutTimer, SIGNAL(timeout()), this, SLOT(updateSize()) );

   // init mixer widget
   initMixer( s_mixers.first() );
     
   // FIXME activate menu items
   //setActions(About | Help | Preferences);
}

KMixApplet::~KMixApplet()
{
   // destroy static vars
   s_instCount--;
   if ( !s_instCount )
   {
      s_mixers.clear();
      delete s_timer;
   }
}

void KMixApplet::initMixer( Mixer *mixer )
{
   delete m_mixerWidget;   
   if ( mixer )
   {
      m_mixerWidget = new KMixerWidget( mixer, true, true, this );
      connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(updateLayout()));
   }
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

void KMixApplet::resizeEvent(QResizeEvent *e) 
{      
   KPanelApplet::resizeEvent( e );

   m_lockedLayout++;
   m_mixerWidget->setGeometry( 0, 0, width(), height() );
   m_lockedLayout--;
}

#include "kmixapplet.moc"
