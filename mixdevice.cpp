//-*-mode: C++; mode: folding-*-
/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
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

#include "mixdevice.h"

#include <klocale.h>
#include <kled.h>
#include <kiconloader.h>
#include <kconfig.h>

#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qpopupmenu.h>

#include <iostream.h>

// {{{ ========== MixDeviceWidget implementation ==================

#include "mixdevice.h"

MixDeviceWidget::MixDeviceWidget(MixDevice* md,
                                 QWidget* parent, const char* name) :
  QWidget( parent, name ), m_mixdevice( md )
{
  m_iconLabel = 0L;
  m_popupMenu = 0L;
  setIcon( md->type() );

  m_muteLED = new KLed( Qt::green, KLed::On, KLed::Sunken, KLed::Circular, this,
                        "MuteLED" );
  m_muteLED->setFixedSize( QSize(16, 16) );
  QToolTip::add( m_muteLED, i18n("Muting") );
  QBoxLayout *layout = new QVBoxLayout( this, 3);
  layout->addWidget( m_iconLabel );
  layout->addWidget( m_muteLED );
  m_iconLabel->installEventFilter( this );
  m_muteLED->installEventFilter( this );


  QBoxLayout *sliders = new QHBoxLayout( layout );
  for( int i = 0; i < md->getVolume().channels(); i++ )
    {
      int maxvol = md->getVolume().maxVolume();
      QSlider* slider =
        new QSlider( 0, maxvol, maxvol/10, maxvol - md->getVolume( i ),
                     QSlider::Vertical, this, md->name() );
      slider->setTickmarks( QSlider::Right );
      QToolTip::add( slider, md->name() );
      slider->installEventFilter( this );
      if( i > 0 )
        {
          slider->setTickmarks( QSlider::Left );
          slider->hide();
        }
      sliders->addWidget( slider );
      m_sliders.append ( slider );
      connect( slider, SIGNAL( valueChanged(int) ), this, SLOT( volumeChange ( int ) ));
    }

  if( md->isRecordable() )
    {
      m_recordLED = new KLed( Qt::red, md->isRecsrc() ? KLed::On : KLed::Off,
                              KLed::Sunken, KLed::Circular, this,
                              "RecordLED" );
      QToolTip::add( m_recordLED, i18n("Recording") );
      m_recordLED->setFixedSize( QSize(16, 16) );

      layout->addWidget( m_recordLED );
    }
  else
    {
      m_recordLED = 0L;
      layout->addSpacing( 16 );
    }

  //  if( parent ) parent->installEventFilter( this );
};

MixDeviceWidget::~MixDeviceWidget()
{
  //TODO: delete Sliders
  delete m_muteLED;
  if( m_recordLED ) delete m_recordLED;
}

bool MixDeviceWidget::eventFilter( QObject* , QEvent* e)
{
  // Lets see, if we have a "Right mouse button press"
  if (e->type() == QEvent::MouseButtonPress)
    {
      QMouseEvent *qme = (QMouseEvent*)e;
      if (qme->button() == RightButton) {
        QPopupMenu *qpm = popupMenu();

        if (qpm) {
          QPoint KCMpopup_point = QCursor::pos();
          qpm->popup(KCMpopup_point);
          return true;
        }
      }
    }
  return false;
}

QPopupMenu* MixDeviceWidget::popupMenu()
{
  if( m_popupMenu ) delete m_popupMenu;
  m_popupMenu = new QPopupMenu();
  if ( m_mixdevice->isMuted() )
    m_popupMenu->insertItem(i18n("Un&mute")    , this, SLOT( toggleMuted() ));
  else
    m_popupMenu->insertItem(i18n("&Mute")      , this, SLOT( toggleMuted() ));
  if ( m_mixdevice->isStereo() )
    {
      if ( m_mixdevice->isStereoLinked() )
        m_popupMenu->insertItem(i18n("&Split")  , this, SLOT(toggleStereoLinked() ));
      else
        m_popupMenu->insertItem(i18n("Un&split"), this, SLOT(toggleStereoLinked() ));
    }
  if ( m_mixdevice->isRecordable() )
    m_popupMenu->insertItem(i18n("&RecSource") , this, SLOT( toggleRecsrc() ));

  return m_popupMenu;
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
    miniDevPM = UserIcon("mix_audio");	break;
  case MixDevice::BASS:
    miniDevPM = UserIcon("mix_bass");	break;
  case MixDevice::CD:
    miniDevPM = UserIcon("mix_cd");	break;
  case MixDevice::EXTERNAL:
    miniDevPM = UserIcon("mix_ext");	break;
  case MixDevice::MICROPHONE:
    miniDevPM = UserIcon("mix_microphone");break;
  case MixDevice::MIDI:
    miniDevPM = UserIcon("mix_midi");	break;
  case MixDevice::RECMONITOR:
    miniDevPM = UserIcon("mix_recmon");	break;
  case MixDevice::TREBLE:
    miniDevPM = UserIcon("mix_treble");	break;
  case MixDevice::UNKNOWN:
    miniDevPM = UserIcon("mix_unknown");	break;
  case MixDevice::VOLUME:
    miniDevPM = UserIcon("mix_volume");	break;
  default:
    miniDevPM = UserIcon("mix_unknown");	break;
  }

  if (! miniDevPM.isNull()) {
    m_iconLabel->setPixmap(miniDevPM);
    m_iconLabel->resize(miniDevPM.width(),miniDevPM.height());
    m_iconLabel->setAlignment( Qt::AlignCenter );
  }
  else {
    cerr << "Pixmap missing.\n";
  }

  updateGeometry();
}

void MixDeviceWidget::setStereoLinked(bool value)
{
  m_mixdevice->setStereoLinked( value );
  QSlider* slider;
  for( slider = m_sliders.at( 1 ); slider != 0 ; slider = m_sliders.next() )
    value ? slider->hide() : slider->show();
}

void MixDeviceWidget::toggleStereoLinked()
{
  setStereoLinked( !m_mixdevice->isStereoLinked() );
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
}

void MixDeviceWidget::updateTicks( bool ticks )
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
      if( index == 0 && m_mixdevice->isStereoLinked() )
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
  if( m_mixdevice->isStereoLinked() )
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

// }}}
// {{{ ========== MixDevice implementation ==================

MixDevice::MixDevice(int num, Volume vol, bool recordable,
                     QString name, ChannelType type ) :
  m_volume( vol ), m_type( type ), m_num( num ), m_recordable( recordable )
{
  m_stereoLink = true;

  if( name.isEmpty() )
    m_name = i18n("unknown");
  else
    m_name = name;
};


int MixDevice::getVolume( int channel ) const
{
  return m_volume[ channel ];
}
int MixDevice::rightVolume() const
{
  return m_volume.getVolume( Volume::RIGHT );
}
int MixDevice::leftVolume() const
{
  return m_volume.getVolume( Volume::LEFT );
}

void MixDevice::setVolume( int channel, int vol )
{
  m_volume.setVolume( channel, vol );
}

void MixDevice::read(int set)
{
  QString grp;
  grp.sprintf("Set%i.Dev%i", set, m_num);
  KConfig* config = KGlobal::config();
  config->setGroup(grp);
  //  m_num = devnum;
  setVolume( Volume::LEFT, config->readNumEntry("volumeL", 50) );
  setVolume( Volume::RIGHT, config->readNumEntry("volumeR", 50) );

  setDisabled( config->readNumEntry("is_disabled", 0) );
  setMuted( config->readNumEntry("is_muted", 0) );
  setStereoLinked( config->readNumEntry("StereoLink", 1) );
  m_name         = config->readEntry("name", "unnamed");
}

void MixDevice::write(int set)
{
  QString grp;
  grp.sprintf("Set%i.Dev%i",set, m_num);
  KConfig* config = KGlobal::config();
  config->setGroup(grp);
  config->writeEntry("volumeL", getVolume( Volume::LEFT ) );
  config->writeEntry("volumeR", getVolume( Volume::RIGHT ) );
  config->writeEntry("is_disabled", isDisabled() );
  config->writeEntry("is_muted", isMuted() );
  config->writeEntry("StereoLink", isStereoLinked() );
  config->writeEntry("name", m_name);
}

// }}}

#include "mixdevice.moc"
