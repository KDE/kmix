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
#include <kaction.h>
#include <kpopupmenu.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"

struct Channel
{
      MixDeviceWidget *dev;
};

Profile::Profile( Mixer *mixer )
{
   m_mixer = mixer;
}

void Profile::write()
{
   
}

void Profile::read()
{
}

void Profile::loadConfig( const QString &/*grp*/ )
{
}

void Profile::saveConfig( const QString &/*grp*/ )
{
}

/********************** KMixerWidget *************************/
 
KMixerWidget::KMixerWidget( Mixer *mixer, QWidget * parent, const char * name )
   : QWidget( parent, name ), m_mixer(mixer), m_name(mixer->mixerName())
{
   cerr << "KMixerWidget::KMixerWidget" << endl;

   m_actions = new KActionCollection( this );
   new KAction( i18n("Show &all"), 0, this, SLOT(showAll()), m_actions, "show_all" );
   m_channels.setAutoDelete( true );
   kDebugInfo("mixer=%x", m_mixer);
   
   // Create update timer
   m_timer = new QTimer;
   m_timer->start( 500 );
  
   // Create mixer device widgets
   m_topLayout = new QVBoxLayout( this, 0, 3 );
   QBoxLayout* layout = new QHBoxLayout( m_topLayout );   

   MixSet mixSet = m_mixer->getMixSet();
   MixDevice *mixDevice = mixSet.first();
   for ( ; mixDevice != 0; mixDevice = mixSet.next())
   {
      MixDeviceWidget *mdw =  new MixDeviceWidget( m_mixer, mixDevice, true, true, this, mixDevice->name() );
      layout->addWidget( mdw, 1 );

      connect( mdw, SIGNAL( newVolume( int, Volume )), m_mixer, SLOT( writeVolumeToHW( int, Volume ) ));
      connect( mdw, SIGNAL(newRecsrc(int, bool)), m_mixer, SLOT(setRecsrc(int, bool)) );
      connect( m_mixer, SIGNAL(newRecsrc()), mdw, SLOT(updateRecsrc()) );
      
      connect( this, SIGNAL(updateTicks(bool)), mdw, SLOT(setTicks(bool)) );
      connect( this, SIGNAL(updateLabels(bool)), mdw, SLOT(setLabeled(bool)) );      

      if( mixDevice->num()==m_mixer->masterDevice() )
	 connect( m_mixer, SIGNAL(newBalance(Volume)), mdw, SLOT(setVolume(Volume)) );

      connect( m_timer, SIGNAL(timeout()), mdw, SLOT(updateSliders()) );
      connect( m_timer, SIGNAL(timeout()), mdw, SLOT(updateRecsrc()) );
      
      Channel *chn = new Channel;
      chn->dev = mdw;
      m_channels.append( chn );
   }

   //layout->addStretch( 1000 );

   // Create the left-right-slider
   m_balanceSlider = new QSlider( -100, 100, 25, 0, QSlider::Horizontal,
				  this, "RightLeft" );
   m_topLayout->addWidget( m_balanceSlider );
   connect( m_balanceSlider, SIGNAL(valueChanged(int)), this, SLOT(setBalance(int)) );
   QToolTip::add( m_balanceSlider, i18n("Left/Right balancing") );
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
   updateGeometry();
}

void KMixerWidget::setLabels( bool on )
{
   kDebugInfo("KMixerWidget::setLabels");
   emit updateLabels( on );
   updateGeometry();
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
   KPopupMenu *menu = new KPopupMenu( i18n("Device settings"), this );

   KAction *a = m_actions->action( "show_all" );
   if ( a )
   {
      a->plug( menu );
  
      if (menu)
      {
	 QPoint pos = QCursor::pos();
	 menu->popup( pos );
      }
   }
}

void KMixerWidget::sessionSave( QString grp, bool /*sessionConfig*/ )
{
   KConfig* config = KGlobal::config();
   config->setGroup( grp );

   config->writeEntry( "Devs", m_channels.count() );
   config->writeEntry( "Name", m_name );

   int n=0;
   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
      QString devgrp;
      devgrp.sprintf( "%s.Dev%i", grp.ascii(), n );   
      config->setGroup( devgrp );

      config->writeEntry( "Split", !chn->dev->isStereoLinked() );
      config->writeEntry( "Show", !chn->dev->isDisabled() );

      n++;
   }
}

void KMixerWidget::sessionLoad( QString grp, bool /*sessionConfig*/ )
{
   KConfig* config = KGlobal::config();
   config->setGroup( grp );
   
   int num = config->readNumEntry("Devs", 0);   
   m_name = config->readEntry("Name", m_name );
   
   int n=0;
   for (Channel *chn=m_channels.first(); chn!=0 && n<num; chn=m_channels.next())
   {
      QString devgrp;
      devgrp.sprintf( "%s.Dev%i", grp.ascii(), n );   
      config->setGroup( devgrp );
      
      chn->dev->setStereoLinked( !config->readBoolEntry("Split", false) );
      chn->dev->setDisabled( !config->readBoolEntry("Show", true) );

      n++;
   }
}

void KMixerWidget::showAll()
{
   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
      chn->dev->setDisabled( false );
   }
}
