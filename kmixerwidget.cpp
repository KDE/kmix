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

class Channel
{
   public:
      Channel() : dev( 0 ) {};
      ~Channel() { delete dev; };

      MixDeviceWidget *dev;
};


/********************** KMixerWidget *************************/
 
KMixerWidget::KMixerWidget( int _id, Mixer *mixer, QString mixerName, int mixerNum, 
			    bool small, bool vert, QWidget * parent, const char * name )
   : QWidget( parent, name ), m_mixer(mixer), m_balanceSlider(0),
     m_topLayout(0), m_devLayout(0), 
     m_name( mixerName ), m_mixerName( mixerName ), m_mixerNum( mixerNum ), m_id( _id ),
     m_iconsEnabled( true ), m_labelsEnabled( false ), m_ticksEnabled( false )
     
{   
   kdDebug() << "-> KMixerWidget::KMixerWidget" << endl;
   m_actions = new KActionCollection( this );
   new KAction( i18n("Show &all"), 0, this, SLOT(showAll()), m_actions, "show_all" );
   m_channels.setAutoDelete( true );
   m_small = small;
   m_vertical = vert;

   // Create mixer device widgets        
   if ( mixer )
      createDeviceWidgets( vert ); 
   else
   {
      QBoxLayout *layout = new QHBoxLayout( this );
      QString s = i18n("Invalid mixer");
      if ( mixerName ) s += " \"" + mixerName + "\"";
      QLabel *errorLabel = new QLabel( s, this );
      errorLabel->setAlignment( QLabel::AlignCenter );
      layout->addWidget( errorLabel );
   }

   kdDebug() << "<- KMixerWidget::KMixerWidget" << endl;
}

KMixerWidget::~KMixerWidget()
{
}

void KMixerWidget::createDeviceWidgets( bool vert )
{   
   kdDebug() << "-> KMixerWidget::createDeviceWidgets" << endl;

   if ( !m_mixer ) return;

   // delete old objects
   m_channels.clear(); 
   delete m_balanceSlider;
   delete m_devLayout;
   delete m_topLayout;

   m_vertical = vert;

   // create layouts   
   m_topLayout = new QVBoxLayout( this, 0, 3 );
   m_devLayout = new QHBoxLayout( m_topLayout );

   // create devices
   kdDebug() << "mixSet" << endl;
   MixSet mixSet = m_mixer->getMixSet();
   MixDevice *mixDevice = mixSet.first();
   for ( ; mixDevice != 0; mixDevice = mixSet.next())
   {
      kdDebug() << "MixDeviceWidget" << endl;
      MixDeviceWidget *mdw = 
	 new MixDeviceWidget( m_mixer, mixDevice, !m_small, !m_small, m_small, vert,
				 this, mixDevice->name().latin1() );
          
      connect( mdw, SIGNAL(updateLayout()), this, SLOT(updateSize()));
      m_devLayout->addWidget( mdw, 0 );            

      Channel *chn = new Channel;
      chn->dev = mdw;
      m_channels.append( chn );
   }

   m_devLayout->addStretch( 1 );

   // Create the left-right-slider   
   if ( !m_small )
   {      
      m_balanceSlider = new QSlider( -100, 100, 25, 0, QSlider::Horizontal,
				  this, "RightLeft" );
      m_balanceSlider->setTickmarks( QSlider::Below );
      m_balanceSlider->setTickInterval( 25 );
      m_topLayout->addWidget( m_balanceSlider );
      connect( m_balanceSlider, SIGNAL(valueChanged(int)), m_mixer, SLOT(setBalance(int)) );
      QToolTip::add( m_balanceSlider, i18n("Left/Right balancing") );
   } else
      m_balanceSlider = 0;

   updateSize();

   kdDebug() << "<- KMixerWidget::createDeviceWidgets" << endl;
}

void KMixerWidget::updateSize()
{   
   layout()->activate();
   setMinimumWidth( layout()->minimumSize().width() );
   setMinimumHeight( layout()->minimumSize().height() );
   emit updateLayout();
}

void KMixerWidget::setTicks( bool on )
{  
   if ( m_ticksEnabled!=on )
   {
      m_ticksEnabled = on;
      for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() )
	 chn->dev->setTicks( on );        
   }
}

void KMixerWidget::setLabels( bool on )
{
   if ( m_labelsEnabled!=on )
   {
      m_labelsEnabled = on;
      for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() )
	 chn->dev->setLabeled( on );            
   }
}

void KMixerWidget::setIcons( bool on )
{
   if ( m_iconsEnabled!=on )
   {
      m_iconsEnabled = on;
      for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() )
	 chn->dev->setIcons( on );      
   }
}

void KMixerWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      rightMouseClicked();   
}

void KMixerWidget::rightMouseClicked()
{
   KPopupMenu *menu = new KPopupMenu( i18n("Device settings"), this );

   KAction *a = m_actions->action( "show_all" );
   if ( a ) a->plug( menu );
   
   QPoint pos = QCursor::pos();
   menu->popup( pos );   
}

void KMixerWidget::saveConfig( KConfig *config, QString grp )
{
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

void KMixerWidget::loadConfig( KConfig *config, QString grp )
{
   kdDebug() << "-> KMixerWidget::sessionLoad" << endl;
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

   kdDebug() << "<- KMixerWidget::sessionLoad" << endl;
}

void KMixerWidget::showAll()
{
   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
      chn->dev->setDisabled( false );
   }
   
   updateSize();
}

#include "kmixerwidget.moc"
