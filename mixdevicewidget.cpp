/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *		 1996-2000 Christian Esken <esken@kde.org>
 *        		   Sven Fischer <herpes@kawo2.rwth-aachen.de>
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
#include <kled.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kaction.h>
#include <kpopmenu.h>

#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qpopupmenu.h>

#include <iostream.h>

#include "mixer.h"
#include "mixdevicewidget.h"
#include "kledbutton.h"

MixDeviceWidget::MixDeviceWidget(MixDevice* md, bool showMuteLED, bool showRecordLED,
                                 QWidget* parent, const char* name) :
   QWidget( parent, name ), m_mixdevice( md )
{
   // global stuff
   m_popupMenu = 0L;
   m_show = true;
   m_split = false;
   m_actions = new KActionCollection( this );

   new KToggleAction( i18n("&Split channels"), 0, this, SLOT(toggleStereoLinked()), 
		      m_actions, "stereo" );   
   new KAction( i18n("&Hide"), 0, this, SLOT(setDisabled()), m_actions, "hide" );
   new KAction( i18n("&Show all"), 0, parent, SLOT(showAll()), m_actions, "show_all" );

   connect( this, SIGNAL(rightMouseClick()), SLOT(contextMenu()) );   

   // create channel icon
   QBoxLayout *layout = new QVBoxLayout( this );
   m_iconLabel = 0L;
   setIcon( md->type() );
   layout->addWidget( m_iconLabel );
   m_iconLabel->installEventFilter( this );
   QToolTip::add( m_iconLabel, md->name() );

   // create label
   m_label = new QLabel( md->name(), this );
   m_label->setAlignment( AlignCenter | AlignVCenter );
   layout->addWidget( m_label );
   m_label->installEventFilter( this );
   QToolTip::add( m_label, md->name() );

   // create mute LED
   m_muteLED = new KLedButton( Qt::green, KLed::On, KLed::Flat,
			       KLed::Circular, this, "MuteLED" );
   if (!showMuteLED) m_muteLED->hide();
   m_muteLED->setFixedSize( QSize(16, 16) );
   QToolTip::add( m_muteLED, i18n("Muting") );
   layout->addWidget( m_muteLED );
   m_muteLED->installEventFilter( this );
   connect(m_muteLED, SIGNAL(stateChanged(bool)), this, SLOT(setUnmuted(bool)));

   // create sliders
   QBoxLayout *sliders = new QHBoxLayout( layout );
   for( int i = 0; i < md->getVolume().channels(); i++ )
   {
      int maxvol = md->getVolume().maxVolume();
      QSlider* slider = new QSlider( 0, maxvol, maxvol/10, maxvol - md->getVolume( i ),
				     QSlider::Vertical, this, md->name() );
      QToolTip::add( slider, md->name() );
      slider->installEventFilter( this );
      if( i>0 ) slider->hide();
      sliders->addWidget( slider );
      m_sliders.append ( slider );
      connect( slider, SIGNAL( valueChanged(int) ), this, SLOT( volumeChange ( int ) ));
   }

   // create record source LED
   if( md->isRecordable() )
   {
      m_recordLED = new KLedButton( Qt::red, md->isRecsrc() ? KLed::On : KLed::Off,
				    KLed::Flat, KLed::Circular, this,
				    "RecordLED" );
      if (!showRecordLED) m_recordLED->hide();
      QToolTip::add( m_recordLED, i18n("Recording") );
      m_recordLED->setFixedSize( QSize(16, 16) );

      layout->addWidget( m_recordLED );
      connect(m_recordLED, SIGNAL(stateChanged(bool)), this, SLOT(setRecsrc(bool)));
      m_recordLED->installEventFilter( this );
   }
   else
   {
      m_recordLED = 0L;
      layout->addSpacing( 16 );
   }
};

MixDeviceWidget::~MixDeviceWidget()
{
   //TODO: delete Sliders
   delete m_muteLED;
   if( m_recordLED ) delete m_recordLED;
}

void MixDeviceWidget::contextMenu()
{
   kDebugInfo("MixDeviceWidget::contextMenu");

   KPopupMenu *menu = new KPopupMenu( i18n("Device settings"), this );

   KToggleAction *stereo = (KToggleAction *)m_actions->action( "stereo" );
   if ( stereo )
   {
      stereo->setChecked( !isStereoLinked() );
      stereo->plug( menu );
   }
   
   KAction *a = m_actions->action( "hide" );
   if ( a ) a->plug( menu );

   KActionSeparator sep( this );
   sep.plug( menu );

   a = m_actions->action( "show_all" );
   if ( a ) a->plug( menu );
  
   if (menu)
   {
      QPoint pos = QCursor::pos();
      menu->popup( pos );
   }
}

bool MixDeviceWidget::eventFilter( QObject* , QEvent* e)
{
   // Lets see, if we have a "Right mouse button press"
   if (e->type() == QEvent::MouseButtonPress)
   {
      QMouseEvent *qme = (QMouseEvent*)e;
      if (qme->button() == RightButton) {
	 emit rightMouseClick();
      }
   }
   return false;
}

void MixDeviceWidget::setIcon( int icon )
{
   if( !m_iconLabel )
   {
      m_iconLabel = new QLabel(this);
      m_iconLabel->installEventFilter( parent() );
   }

   QPixmap miniDevPM;
   switch (icon) {
      case MixDevice::AUDIO:
	 miniDevPM = BarIcon("mix_audio");	break;
      case MixDevice::BASS:
	 miniDevPM = BarIcon("mix_bass");	break;
      case MixDevice::CD:
	 miniDevPM = BarIcon("mix_cd");	break;
      case MixDevice::EXTERNAL:
	 miniDevPM = BarIcon("mix_ext");	break;
      case MixDevice::MICROPHONE:
	 miniDevPM = BarIcon("mix_microphone");break;
      case MixDevice::MIDI:
	 miniDevPM = BarIcon("mix_midi");	break;
      case MixDevice::RECMONITOR:
	 miniDevPM = BarIcon("mix_recmon");	break;
      case MixDevice::TREBLE:
	 miniDevPM = BarIcon("mix_treble");	break;
      case MixDevice::UNKNOWN:
	 miniDevPM = BarIcon("mix_unknown");	break;
      case MixDevice::VOLUME:
	 miniDevPM = BarIcon("mix_volume");	break;
      default:
	 miniDevPM = BarIcon("mix_unknown");	break;
   }

   if (! miniDevPM.isNull()) {
      m_iconLabel->setPixmap(miniDevPM);
      //m_iconLabel->resize(miniDevPM.width(),miniDevPM.height());
      m_iconLabel->resize( 22, 22 );
      m_iconLabel->setAlignment( Qt::AlignCenter );
   }
   else {
      cerr << "Pixmap missing.\n";
   }

   updateGeometry();
}

bool MixDeviceWidget::isDisabled()
{
   return !m_show;
}

bool MixDeviceWidget::isMuted()
{
   return m_mixdevice->isMuted();
}

bool MixDeviceWidget::isRecsrc()
{
   return m_mixdevice->isRecsrc();
}

bool MixDeviceWidget::isStereoLinked()
{
   return !m_split;
}

bool MixDeviceWidget::isLabeled()
{
   return m_label->isVisible();
}

void MixDeviceWidget::setStereoLinked(bool value)
{
   m_split = !value;
   QSlider* slider;
   for( slider = m_sliders.at( 1 ); slider != 0 ; slider = m_sliders.next() )
      value ? slider->hide() : slider->show();
}

void MixDeviceWidget::toggleStereoLinked()
{
   setStereoLinked( !m_split );
}

void MixDeviceWidget::setMuted(bool value)
{
   m_muteLED->setState( value ? KLed::Off : KLed::On );
   m_mixdevice->setMuted( value );
   emit newVolume( m_mixdevice->num(), m_mixdevice->getVolume() );
}

void MixDeviceWidget::toggleMuted()
{
   setMuted( !m_mixdevice->isMuted() );
}

void MixDeviceWidget::setRecsrc(bool value)
{
   if( m_recordLED )
   {
      if( m_mixdevice->isRecsrc() != value )
      {
	 m_mixdevice->setRecsrc( value );
	 m_recordLED->setState( value ? KLed::On : KLed::Off );
	 emit newRecsrc( m_mixdevice->num(), value );
      }
   }
}

void MixDeviceWidget::setLabeled(bool value)
{
   if (value)
      m_label->show();
   else
      m_label->hide();
  
   updateGeometry(); 
  
   layout()->activate();
}

void MixDeviceWidget::updateRecsrc()
{
   if( m_recordLED ) m_recordLED->setState( m_mixdevice->isRecsrc() ? KLed::On : KLed::Off );
}

void MixDeviceWidget::toggleRecsrc()
{
   setRecsrc( !m_mixdevice->isRecsrc() );
}

void MixDeviceWidget::setDisabled(bool value)
{
   value ? hide() : show();
   m_show = !value;
}

void MixDeviceWidget::setTicks( bool ticks )
{
   for( QSlider* slider = m_sliders.first(); slider != 0; slider = m_sliders.next() )
   {
      if( ticks )
	 if( m_sliders.at() == 0 )
	    slider->setTickmarks( QSlider::Right );
	 else
	    slider->setTickmarks( QSlider::Left );
      else
	 slider->setTickmarks( QSlider::NoMarks );
   }
 
   layout()->activate();
}

void MixDeviceWidget::updateSliders()
{
   Volume vol = m_mixdevice->getVolume();
   setVolume( vol );
}

void MixDeviceWidget::volumeChange( int )
{
   QSlider* slider;
   Volume vol = m_mixdevice->getVolume();
   int index = 0;
   for( slider = m_sliders.first(); slider != 0; slider = m_sliders.next() )
   {
      int svol = slider->maxValue() - slider->value();
      index = m_sliders.at();
      if( index == 0 && !m_split )
      {
	 vol.setAllVolumes( svol );
	 break;
      }
      else
	 vol.setVolume( index, slider->maxValue() - slider->value());
      index++;
   }
   setVolume( vol );
   emit newVolume( m_mixdevice->num(), vol );
}

void MixDeviceWidget::setVolume( int channel, int vol )
{
   m_sliders.at( channel )->setValue( m_sliders.at( channel )->maxValue() - vol );
   m_mixdevice->setVolume( channel, vol );
}

void MixDeviceWidget::setVolume( Volume vol )
{
   if( !m_split )
   {
      int maxvol = 0;
      for( int i = 0; i < vol.channels(); i++ )
	 maxvol = vol[i] > maxvol ? vol[i] : maxvol;
      m_sliders.first()->setValue( vol.maxVolume() - maxvol );
   }
   else
      for( int i = 0; i < vol.channels(); i++ )
      {
	 m_sliders.at( i )->blockSignals( true );
	 m_sliders.at( i )->setValue( vol.maxVolume() - vol[i] );
	 m_sliders.at( i )->blockSignals( false );
      }
   m_mixdevice->setVolume( vol );
}

void MixDeviceWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      emit rightMouseClick();   
}

