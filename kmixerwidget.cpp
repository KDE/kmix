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
 
KMixerWidget::KMixerWidget( Mixer *mixer, bool small, bool vert, QWidget * parent, const char * name )
   : QWidget( parent, name ), m_mixer(mixer), m_devLayout(0), m_name(mixer->mixerName())
{   
   kDebugInfo("-> KMixerWidget::KMixerWidget");
   m_actions = new KActionCollection( this );
   new KAction( i18n("Show &all"), 0, this, SLOT(showAll()), m_actions, "show_all" );
   m_channels.setAutoDelete( true );
   m_small = small;
   m_vertical = vert;

   // Create mixer device widgets  
   kDebugInfo("m_topLayout");
   m_topLayout = new QVBoxLayout( this, 0, 3 );     
   updateDevices( vert );

   // Create the left-right-slider   
   if ( !small )
   {      
      m_balanceSlider = new QSlider( -100, 100, 25, 0, QSlider::Horizontal,
				  this, "RightLeft" );
      m_topLayout->addWidget( m_balanceSlider );
      connect( m_balanceSlider, SIGNAL(valueChanged(int)), this, SLOT(setBalance(int)) );
      QToolTip::add( m_balanceSlider, i18n("Left/Right balancing") );
   } else
      m_balanceSlider = 0;

   updateSize();

   kDebugInfo("<- KMixerWidget::KMixerWidget");
}

KMixerWidget::~KMixerWidget()
{
}

void KMixerWidget::updateDevices( bool vert )
{   
   kDebugInfo("-> KMixerWidget::updateDevices");

   m_channels.clear();
   m_vertical = vert;

   kDebugInfo("m_devLayout");
   delete m_devLayout;
   m_devLayout = new QHBoxLayout( m_topLayout );
//   m_topLayout->insertLayout( 0, m_devLayout );
   
   kDebugInfo("mixSet");
   MixSet mixSet = m_mixer->getMixSet();
   MixDevice *mixDevice = mixSet.first();
   for ( ; mixDevice != 0; mixDevice = mixSet.next())
   {
      kDebugInfo("MixDeviceWidget");
      MixDeviceWidget *mdw;
      if ( m_small )
      {
	 mdw =  new SmallMixDeviceWidget( m_mixer, mixDevice, true, this, mixDevice->name() );
      } else
      {
	 mdw =  new BigMixDeviceWidget( m_mixer, mixDevice, true, true, true, this, 
					mixDevice->name() );
           
	 connect( this, SIGNAL(updateTicks(bool)), mdw, SLOT(setTicks(bool)) );
	 connect( this, SIGNAL(updateLabels(bool)), mdw, SLOT(setLabeled(bool)) );
      }

      connect( this, SIGNAL(updateIcons(bool)), mdw, SLOT(setIcons(bool)) );
      connect( mdw, SIGNAL(updateLayout()), this, SLOT(updateSize()));
      m_devLayout->addWidget( mdw, 0 );

      Channel *chn = new Channel;
      chn->dev = mdw;
      m_channels.append( chn );
   }

   kDebugInfo("m_devLayout stretch");
   m_devLayout->addStretch( 1 );

   updateSize();

   kDebugInfo("<- KMixerWidget::updateDevices");
}

void KMixerWidget::updateSize()
{   
   layout()->activate();
   kDebugInfo("KMixerWidget::updateSize minwidth=%d", m_topLayout->minimumSize().width() );
   setMinimumWidth( layout()->minimumSize().width() );
   setMinimumHeight( layout()->minimumSize().height() );
   emit updateLayout();
}

void KMixerWidget::setTicks( bool on )
{
   emit updateTicks( on );
}

void KMixerWidget::setLabels( bool on )
{
   emit updateLabels( on );
}

void KMixerWidget::setIcons( bool on )
{
   kDebugInfo("KMixerWidget::setIcons( %d )", on );
   emit updateIcons( on );
}

void KMixerWidget::setBalance( int value )
{
   m_mixer->setBalance( value );
   if ( m_balanceSlider )
      m_balanceSlider->setValue( value );
}

void KMixerWidget::setOrientation( int vert )
{
   updateDevices( vert ); 
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
   kDebugInfo("-> KMixerWidget::sessionLoad");

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

   kDebugInfo("<- KMixerWidget::sessionLoad");
}

void KMixerWidget::showAll()
{
   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
      chn->dev->setDisabled( false );
   }
   
   updateSize();
}

