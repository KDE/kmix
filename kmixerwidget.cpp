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

#include <iostream>
#include <stdlib.h>

#include <qcursor.h>
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
#include <kiconloader.h>
#include <kglobalaccel.h>

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
                            bool small, KPanelApplet::Direction dir,
                            QWidget * parent, const char * name )
   : QWidget( parent, name ), m_mixer(mixer), m_balanceSlider(0),
     m_topLayout(0), m_devLayout(0),
     m_name( mixerName ), m_mixerName( mixerName ), m_mixerNum( mixerNum ), m_id( _id ),
     m_direction(dir),
     m_iconsEnabled( true ), m_labelsEnabled( false ), m_ticksEnabled( false )

{
   m_actions = new KActionCollection( this );
   new KAction( i18n("Show &All"), 0, this, SLOT(showAll()), m_actions, "show_all" );

   m_channels.setAutoDelete( true );
   m_small = small;

   // Create mixer device widgets
   if ( mixer ) {
      createDeviceWidgets( m_direction );
   } else {
      QBoxLayout *layout = new QHBoxLayout( this );
      QString s = i18n("Invalid mixer");
      if ( !mixerName.isEmpty() ) s += " \"" + mixerName + "\"";
      QLabel *errorLabel = new QLabel( s, this );
      errorLabel->setAlignment( QLabel::AlignCenter | QLabel::WordBreak );
      layout->addWidget( errorLabel );
   }
}

KMixerWidget::~KMixerWidget()
{
}

void KMixerWidget::createDeviceWidgets( KPanelApplet::Direction dir )
{
   if ( !m_mixer ) return;

   // delete old objects
   m_channels.clear();
   delete m_balanceSlider;
   delete m_devLayout;
   delete m_topLayout;

   m_direction = dir;

   // create layouts
   m_topLayout = new QVBoxLayout( this, 0, 3 );
   if ((m_direction == KPanelApplet::Up) || (m_direction == KPanelApplet::Down))
     m_devLayout = new QHBoxLayout( m_topLayout );
   else
     m_devLayout = new QVBoxLayout( m_topLayout );

   // create devices
   MixSet mixSet = m_mixer->getMixSet();
   MixDevice *mixDevice = mixSet.first();
   for ( ; mixDevice != 0; mixDevice = mixSet.next())
   {
      MixDeviceWidget *mdw =
	new MixDeviceWidget( m_mixer, mixDevice, !m_small, !m_small, m_small,
			     m_direction, this, mixDevice->name().latin1() );

      connect( mdw, SIGNAL( masterMuted( bool ) ),
                  SIGNAL( masterMuted( bool ) ) );

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

      QTimer *updateTimer = new QTimer( this );
      connect( updateTimer, SIGNAL(timeout()), this, SLOT(updateBalance()) );
      updateTimer->start( 200, FALSE );
   } else
      m_balanceSlider = 0;

   updateSize();
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

void KMixerWidget::setColors( const Colors &color )
{
    for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() ) {
        chn->dev->setColors( color.high, color.low, color.back );
        chn->dev->setMutedColors( color.mutedHigh, color.mutedLow, color.mutedBack );
    }
}

void KMixerWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      rightMouseClicked();
}

void KMixerWidget::addActionToPopup( KAction *action ) {
  m_actions->insert( action );

  for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() ) {
    chn->dev->addActionToPopup( action );
  }
}

void KMixerWidget::rightMouseClicked()
{
   KPopupMenu *menu = new KPopupMenu( this );
   menu->insertTitle( SmallIcon( "kmix" ), i18n("Device settings") );

   KAction *a = m_actions->action( "show_all" );
   if ( a ) a->plug( menu );

   a = m_actions->action( "options_show_menubar" );
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

      KGlobalAccel *keys=chn->dev->keys();
      if (keys) {
	 QString devgrpkeys;
	 devgrpkeys.sprintf( "%s.Dev%i.keys", grp.ascii(), n );
	 keys->setConfigGroup(devgrpkeys);
	 keys->writeSettings(config);
      }
      n++;
   }
}

void KMixerWidget::loadConfig( KConfig *config, QString grp )
{
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

      KGlobalAccel *keys=chn->dev->keys();
      if ( keys ) {
	 QString devgrpkeys;
	 devgrpkeys.sprintf( "%s.Dev%i.keys", grp.ascii(), n );

	 keys->setConfigGroup(devgrpkeys);
	 keys->readSettings(config);
	 keys->updateConnections();
      }

      n++;
   }
}

void KMixerWidget::showAll()
{
   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
      chn->dev->setDisabled( false );
   }

   updateSize();
}

void KMixerWidget::updateBalance()
{
  MixDevice *md = m_mixer->mixDeviceByType( 0 );
  int right= md->rightVolume();
  int left = md->leftVolume();
  
  int value = 0;
  if ( left != right ) {
    int refvol = left > right ? left : right;
    if (left > right)
      value = (100*right) / refvol - 100;
    else
      value = -((100*left) / refvol - 100);
  } 
  m_balanceSlider->blockSignals( true );
  m_balanceSlider->setValue( value );
  m_balanceSlider->blockSignals( false );  
}


#include "kmixerwidget.moc"
