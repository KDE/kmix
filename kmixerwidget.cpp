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

#include <iostream.h>
#include <stdlib.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qslider.h>
#include <qtooltip.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <kcombobox.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"

struct Channel
{
      bool show;
      bool split;
};

/********************** KMixerWidget *************************/
 
KMixerWidget::KMixerWidget( Mixer *mixer, QWidget * parent, const char * name )
   : QWidget( parent, name ), m_mixer(mixer), m_name(mixer->mixerName())
{
   cerr << "KMixerWidget::KMixerWidget" << endl;

   m_channels.setAutoDelete( true );
   kDebugInfo("mixer=%x", m_mixer);
   
   // Create update timer
   m_timer = new QTimer;
   m_timer->start( 1000 );
   connect( m_timer, SIGNAL(timeout()), m_mixer, SLOT(readSetFromHW()));

   // Create mixer device widgets
   m_topLayout = new QVBoxLayout( this, 0, 3 );
   QBoxLayout* layout = new QHBoxLayout( m_topLayout );   
   MixSet mixSet = m_mixer->getMixSet();
   MixDevice *mixDevice = mixSet.first();
   for ( ; mixDevice != 0; mixDevice = mixSet.next())
   {
      Channel *chn = new Channel;
      chn->split = false;
      chn->show = true;
      m_channels.append( chn );

      cerr << "mixDevice = " << mixDevice << endl;
      MixDeviceWidget *mdw =  new MixDeviceWidget( mixDevice, true, true,
						   this, mixDevice->name() );
      layout->addWidget( mdw );
      connect( mdw, SIGNAL( newVolume( int, Volume )),
               m_mixer, SLOT( writeVolumeToHW( int, Volume ) ));
      connect( mdw, SIGNAL( newRecsrc(int, bool)),
               m_mixer, SLOT( setRecsrc(int, bool ) ));
      connect( m_mixer, SIGNAL( newRecsrc()),
               mdw, SLOT( updateRecsrc() ));
      connect( m_timer, SIGNAL(timeout()), mdw, SLOT(updateSliders()) );
      connect( this, SIGNAL(updateTicks(bool)), mdw, SLOT(updateTicks(bool)) );
      if( mixDevice->num()==m_mixer->masterDevice() )
	 connect( m_mixer, SIGNAL(newBalance(Volume)), mdw, SLOT(setVolume(Volume)));
      connect( mdw, SIGNAL(rightMouseClick()), this, SLOT(rightMouseClicked()));
   }

   // Create the left-right-slider
   m_balanceSlider = new QSlider( -100, 100, 25, 0, QSlider::Horizontal,
				  this, "RightLeft" );
   m_topLayout->addWidget( m_balanceSlider );
   connect( m_balanceSlider, SIGNAL(valueChanged(int)), this, SLOT(setBalance(int)) );
   QToolTip::add( m_balanceSlider, i18n("Left/Right balancing") );

   updateSize();
}

KMixerWidget::~KMixerWidget()
{
   if (m_timer) delete m_timer;
}

void KMixerWidget::updateSize()
{
   setFixedWidth( m_topLayout->minimumSize().width() );
   setMinimumHeight( m_topLayout->minimumSize().height() );
}

void KMixerWidget::setTicks( bool on )
{
   emit updateTicks( on );
   updateSize();
}

void KMixerWidget::setBalance( int value )
{
   m_mixer->setBalance( value );
   m_balanceSlider->setValue( value );
}

void KMixerWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
   {
      rightMouseClicked();
   }
}

void KMixerWidget::rightMouseClicked()
{
}

void KMixerWidget::sessionSave( QString grp, bool /*sessionConfig*/ )
{
   KConfig* config = KGlobal::config();
   config->setGroup( grp );

   config->writeEntry( "devs", m_channels.count() );
   config->writeEntry( "name", m_name );

   int n=0;
   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
      QString devgrp;
      devgrp.sprintf( "%s.Dev%i", grp.ascii(), n );   
      config->setGroup( devgrp );

      config->writeEntry( "split", chn->split );
      config->writeEntry( "show", chn->show );

      n++;
   }
}

void KMixerWidget::sessionLoad( QString grp, bool /*sessionConfig*/ )
{
   KConfig* config = KGlobal::config();
   config->setGroup( grp );
   
   int num = config->readNumEntry("devs", 0);   

   QString name = config->readEntry("name", QString::null );
   if ( !name.isEmpty() ) m_name = name;

   int n=0;
   for (Channel *chn=m_channels.first(); chn!=0 && n<num; chn=m_channels.next())
   {
      QString devgrp;
      devgrp.sprintf( "%s.Dev%i", grp.ascii(), n );   
      config->setGroup( devgrp );
      
      chn->split = config->readBoolEntry("split", false);
      chn->show = config->readBoolEntry("show", true);

      n++;
   }
}
