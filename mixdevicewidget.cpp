/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *               1996-2000 Christian Esken <esken@kde.org>
 *                         Sven Fischer <herpes@kawo2.rwth-aachen.de>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <klocale.h>
#include <kled.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>
#include <kdebug.h>

#include <qcursor.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qtimer.h>
#include <qwmatrix.h>

#include "mixer.h"
#include "mixdevicewidget.h"
#include "kledbutton.h"
#include "ksmallslider.h"
#include "verticaltext.h"

/**
 * Class that represents a single mix device, inlcuding PopUp, muteLED, ...
 * Used of KMix main window and DockWidget and PanelApplet?
 * It can be configured to include or exclude the recordLED and the muteLED.
 * The direction (horizontal, vertical) can be configured and whether it should
 * be "small"  (uses KSmallSlider instead of QSlider then).
 */
MixDeviceWidget::MixDeviceWidget(Mixer *mixer, MixDevice* md,
                                 bool showMuteLED, bool showRecordLED,
                                 bool small, KPanelApplet::Direction dir,
                                 QWidget* parent,  const char* name) :
   QWidget( parent, name ), m_mixer(mixer), m_mixdevice( md ),
   m_linked( true ), m_disabled( false ), m_direction( dir ), m_small( small ),
   m_iconLabel( 0 ), m_muteLED( 0 ), m_recordLED( 0 ), m_label( 0 )
{
   // global stuff
   connect( this, SIGNAL(newVolume(int, Volume)), m_mixer, SLOT(writeVolumeToHW(int, Volume) ));
   connect( this, SIGNAL(newRecsrc(int, bool)), m_mixer, SLOT(setRecordSource(int, bool)) );
   connect( m_mixer, SIGNAL(newRecsrc()), SLOT(update()) );
   if( m_mixdevice->num()==m_mixer->masterDevice() )
      connect( m_mixer, SIGNAL(newBalance(Volume)), SLOT(update()) );

   connect( this, SIGNAL(rightMouseClick()), SLOT(contextMenu()) );

   // create actions
   m_actions = new KActionCollection( this );

   if (parent->isA("KMixerWidget"))
       new KToggleAction( i18n("&Split Channels"), 0, this, SLOT(toggleStereoLinked()),
                      m_actions, "stereo" );
	
	new KAction( i18n("&Hide"), 0, this, SLOT(setDisabled()), m_actions, "hide" );

   KToggleAction *a = new KToggleAction( i18n("&Muted"), 0, 0, 0, m_actions, "mute" );
   	connect( a, SIGNAL(toggled(bool)), SLOT(setMuted(bool)) );

   if (parent->isA("KMixerWidget")) {
     new KAction( i18n("Show &All"), 0, parent, SLOT(showAll()), m_actions, "show_all" );
   }

   if( m_mixdevice->isRecordable() )
   {
      a = new KToggleAction( i18n("Set &Record Source"), 0, 0, 0, m_actions, "recsrc" );
      	connect( a, SIGNAL(toggled(bool)), SLOT(setRecsrc(bool)) );
   }

   new KAction( i18n("Define &Keys..."), 0, this, SLOT(defineKeys()), m_actions, "keys" );

   // create widgets
   createWidgets( showMuteLED, showRecordLED );

   // create update timer
   m_updateTimer = new QTimer( this );
   connect( m_updateTimer, SIGNAL(timeout()), this, SLOT(update()) );
   m_updateTimer->start( 200, FALSE );

   m_keys = new KGlobalAccel( this, "Keys" );
   m_keys->insert( "Increase volume", i18n( "Increase Volume" ), QString::null,
      KShortcut(), KShortcut(), this, SLOT( increaseVolume() ) );
   m_keys->insert( "Decrease volume", i18n( "Decrease Volume" ), QString::null,
      KShortcut(), KShortcut(), this, SLOT( decreaseVolume() ) );
   m_keys->insert( "Toggle mute", i18n( "Toggle Mute" ), QString::null,
      KShortcut(), KShortcut(), this, SLOT( toggleMuted() ) );
   // No need for m_keys->readSettings(), the keys are loaded in KMixerWidget::loadConfig, see kmixerwidget.cpp
   m_keys->updateConnections();
}

MixDeviceWidget::~MixDeviceWidget()
{
}


void MixDeviceWidget::addActionToPopup( KAction *action ) {
  m_actions->insert( action );
}


#define GET_NEWLAYOUT(x) \
	QBoxLayout *x; \
	if ((m_direction == KPanelApplet::Up) || (m_direction == KPanelApplet::Down)) \
		x = new QHBoxLayout( layout ); \
       	else \
		x = new QVBoxLayout( layout );


#define GET_NEWLAYOUT_LAS(x) \
	QBoxLayout *x; \
	if ((m_direction == KPanelApplet::Up) || (m_direction == KPanelApplet::Down)) \
		x = new QHBoxLayout( labelAndSliders ); \
       	else \
		x = new QVBoxLayout( labelAndSliders );



void 
MixDeviceWidget::createWidgets( bool showMuteLED, bool showRecordLED )
{
   QBoxLayout *layout;
   if ( (m_direction == KPanelApplet::Up) ||  (m_direction == KPanelApplet::Down) )
      layout = new QVBoxLayout( this );
   else
      layout = new QHBoxLayout( this );

   QToolTip::add( this, m_mixdevice->name() );
	
   // create channel icon
	if( ! isSwitch() )
	{
		if ((m_direction == KPanelApplet::Up) || (m_direction == KPanelApplet::Left)) 
		{
			m_iconLabel = 0L;
			setIcon( m_mixdevice->type() );
			layout->addWidget( m_iconLabel );
		} //  otherwise it is created after the slider
	}
	
   // create mute LED
   m_muteLED = new KLedButton( Qt::green, KLed::On, KLed::Sunken,
                               KLed::Circular, this, "MuteLED" );
	if( !isSwitch() )
		if (!showMuteLED) m_muteLED->hide();
	
   m_muteLED->setFixedSize( QSize(16, 16) );
	QString switchLabel;
	if( m_mixdevice->name().length() > 10 )
	{
		switchLabel = m_mixdevice->name().mid(0, 10 );
		switchLabel.append("...");
	}
	else
		switchLabel = m_mixdevice->name();

   GET_NEWLAYOUT( ledlayout );
   ledlayout->addWidget( m_muteLED );
	// Add name to switch
	if( isSwitch() )
		ledlayout->addWidget( new QLabel( switchLabel, this, "LedName" ) );
   m_muteLED->installEventFilter( this );
	
   connect( m_muteLED, SIGNAL(stateChanged(bool)), this, SLOT(setUnmuted(bool)) );

   layout->addSpacing( 1 );

		// create label
		GET_NEWLAYOUT( labelAndSliders );
		m_label = new VerticalText( this, m_mixdevice->name().latin1() );
		m_label->hide();
		labelAndSliders->addWidget( m_label );
		m_label->installEventFilter( this );

		if( isSwitch() )
			m_label->hide();
		
		// create sliders
		GET_NEWLAYOUT_LAS( sliders );
		for( int i = 0; i < m_mixdevice->getVolume().channels(); i++ )
		{
			int maxvol = m_mixdevice->getVolume().maxVolume();
			QWidget* slider;
			if ( m_small )
				slider = new KSmallSlider( 0, maxvol, maxvol/10,
						m_mixdevice->getVolume( i ),
						m_direction,
						this, m_mixdevice->name().ascii() );
			else 
			{
				slider = new QSlider( 0, maxvol, maxvol/10,
						maxvol - m_mixdevice->getVolume( i ),
						(m_direction == KPanelApplet::Up ||
						 m_direction == KPanelApplet::Down) ?
						QSlider::Vertical : QSlider::Horizontal,
						this, m_mixdevice->name().ascii() );
				slider->setMinimumSize( slider->sizeHint() );
			}
			
			slider->installEventFilter( this );
			if( i>0 && isStereoLinked() ) slider->hide();
			if( isSwitch() )
				slider->hide();
			sliders->addWidget( slider );
			m_sliders.append ( slider );
			connect( slider, SIGNAL(valueChanged(int)), SLOT(volumeChange(int)) );
		}

		// create channel icon
		if ((m_direction == KPanelApplet::Right) || (m_direction == KPanelApplet::Down)) 
		{
			m_iconLabel = 0L;
			setIcon( m_mixdevice->type() );
			if( isSwitch() )
				m_iconLabel->hide();
			layout->addWidget( m_iconLabel );
			m_iconLabel->installEventFilter( this );
		} //  otherwise it is created before the slider

		// create record source LED
		if( m_mixdevice->isRecordable() )
		{
			m_recordLED = new KLedButton( Qt::red,
					m_mixdevice->isRecordable()?KLed::On:KLed::Off,
					KLed::Sunken, KLed::Circular, this, "RecordLED" );
			if (!showRecordLED) m_recordLED->hide();
			m_recordLED->setFixedSize( QSize(16, 16) );
			
			GET_NEWLAYOUT( reclayout );
			if( isSwitch() )
				m_recordLED->hide();
			reclayout->addWidget( m_recordLED );
			connect(m_recordLED, SIGNAL(stateChanged(bool)), this, SLOT(setRecsrc(bool)));
			m_recordLED->installEventFilter( this );
		}
		else
		{
			m_recordLED = 0L;
			if ( showRecordLED ) layout->addSpacing( 16 );
		}
}
#undef GET_NEWLAYOUT

QPixmap 
MixDeviceWidget::icon( int icontype )
{
   QPixmap miniDevPM;
   switch (icontype) {
      case MixDevice::AUDIO:
         miniDevPM = UserIcon("mix_audio"); break;
      case MixDevice::BASS:
         miniDevPM = UserIcon("mix_bass"); break;
      case MixDevice::CD:
         miniDevPM = UserIcon("mix_cd"); break;
      case MixDevice::EXTERNAL:
         miniDevPM = UserIcon("mix_ext"); break;
      case MixDevice::MICROPHONE:
         miniDevPM = UserIcon("mix_microphone");break;
      case MixDevice::MIDI:
         miniDevPM = UserIcon("mix_midi"); break;
      case MixDevice::RECMONITOR:
         miniDevPM = UserIcon("mix_recmon"); break;
      case MixDevice::TREBLE:
         miniDevPM = UserIcon("mix_treble"); break;
      case MixDevice::UNKNOWN:
         miniDevPM = UserIcon("mix_unknown"); break;
      case MixDevice::VOLUME:
         miniDevPM = UserIcon("mix_volume"); break;
      case MixDevice::VIDEO:
         miniDevPM = UserIcon("mix_video"); break;
      case MixDevice::SURROUND:
         miniDevPM = UserIcon("mix_surround"); break;
      case MixDevice::HEADPHONE:
         miniDevPM = UserIcon( "mix_headphone" ); break;
      case MixDevice::DIGITAL:
         miniDevPM = UserIcon( "mix_digital" ); break;
      case MixDevice::AC97:
         miniDevPM = UserIcon( "mix_ac97" ); break;
      default:
         miniDevPM = UserIcon("mix_unknown"); break;
   }

   return miniDevPM;
}

void 
MixDeviceWidget::setIcon( int icontype )
{
   if( !m_iconLabel )
   {
      m_iconLabel = new QLabel(this);
      m_iconLabel->installEventFilter( parent() );
   }

   QPixmap miniDevPM = icon( icontype );
   if ( !miniDevPM.isNull() )
   {
      if ( m_small )
      {
         // scale icon
         QWMatrix t;
         t = t.scale( 10.0/miniDevPM.width(), 10.0/miniDevPM.height() );
         m_iconLabel->setPixmap( miniDevPM.xForm( t ) );
         m_iconLabel->resize( 10, 10 );
      } else
         m_iconLabel->setPixmap( miniDevPM );
      m_iconLabel->setAlignment( Qt::AlignCenter );
   } else
   {
      kdDebug() << "Pixmap missing." << endl;
   }

   layout()->activate();
}

bool 
MixDeviceWidget::isLabeled() const
{
   return !m_label->isHidden();
}

bool 
MixDeviceWidget::isDisabled() const
{
   return m_disabled;
}

bool 
MixDeviceWidget::isMuted() const
{
   return m_mixdevice->isMuted();
}

bool 
MixDeviceWidget::isRecsrc() const
{
   return m_mixdevice->isRecordable();
}

bool 
MixDeviceWidget::isSwitch() const
{
	return m_mixdevice->isSwitch();
}

void 
MixDeviceWidget::toggleStereoLinked()
{
   setStereoLinked( !isStereoLinked() );
}

void 
MixDeviceWidget::setMuted(bool value)
{
   m_mixdevice->setMuted( value );
   update();
   emit newVolume( m_mixdevice->num(), m_mixdevice->getVolume() );

   if ( m_mixdevice->num()==m_mixer->masterDevice() )
       emit masterMuted( value );
}

void 
MixDeviceWidget::toggleMuted()
{
   setMuted( !m_mixdevice->isMuted() );
}

void 
MixDeviceWidget::setRecsrc( bool value )
{
   if( m_mixdevice->isRecordable()!=value )
   {
      m_mixdevice->setRecordable( value );
      emit newRecsrc( m_mixdevice->num(), value );
   }
}

void 
MixDeviceWidget::setStereoLinked(bool value)
{
   m_linked = value;

   QWidget *slider = m_sliders.first();
   for( slider=m_sliders.next(); slider!=0 ; slider=m_sliders.next() )
      value ? slider->hide() : slider->show();

   layout()->activate();
   emit updateLayout();
}


void 
MixDeviceWidget::setLabeled(bool value)
{
   if (value)
      m_label->show();
   else
      m_label->hide();

   layout()->activate();
   emit updateLayout();
}

void 
MixDeviceWidget::setTicks( bool ticks )
{
   for( QWidget* slider=m_sliders.first(); slider!=0; slider=m_sliders.next() )
   {
      if ( slider->inherits( "QSlider" ) )
      {
         if( ticks )
            if( m_sliders.at() == 0 )
               static_cast<QSlider *>(slider)->setTickmarks( QSlider::Right );
            else
               static_cast<QSlider *>(slider)->setTickmarks( QSlider::Left );
         else
            static_cast<QSlider *>(slider)->setTickmarks( QSlider::NoMarks );
      }
   }

   layout()->activate();
   emit updateLayout();
}

void 
MixDeviceWidget::setIcons(bool value)
{
   if ( ( !m_iconLabel->isHidden()) !=value )
   {
      if (value)
         m_iconLabel->show();
      else
         m_iconLabel->hide();

      layout()->activate();
      emit updateLayout();
   }
}

void 
MixDeviceWidget::setColors( QColor high, QColor low, QColor back )
{
    for( QWidget *slider=m_sliders.first(); slider!=0; slider=m_sliders.next() ) {
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setColors( high, low, back );
    }
}

void 
MixDeviceWidget::setMutedColors( QColor high, QColor low, QColor back )
{
    for( QWidget *slider=m_sliders.first(); slider!=0; slider=m_sliders.next() ) {
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setGrayColors( high, low, back );
    }
}

KGlobalAccel 
*MixDeviceWidget::keys( void )
{
    return m_keys;
}

void 
MixDeviceWidget::volumeChange( int )
{
   Volume vol = m_mixdevice->getVolume();

   if ( isStereoLinked() )
   {
      QWidget *slider = m_sliders.first();
      if ( slider->inherits( "KSmallSlider" ) )
      {
         KSmallSlider *slider = dynamic_cast<KSmallSlider *>(m_sliders.first());
         if (slider)
            vol.setAllVolumes( slider->value() );
      } else
      {
         QSlider *slider = dynamic_cast<QSlider *>(m_sliders.first());
         if (slider)
            vol.setAllVolumes( slider->maxValue() - slider->value() );
      }
   } else
   {
      int n = 0;
      for( QWidget *slider=m_sliders.first(); slider!=0; slider=m_sliders.next() )
      {
         if ( slider->inherits( "KSmallSlider" ) )
         {
            KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
            if (smallSlider)
               vol.setVolume( n, smallSlider->value() );
         } else
         {
            QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
            if (bigSlider)
               vol.setVolume( n, bigSlider->maxValue() - bigSlider->value() );
         }

         n++;
      }
   }

   setVolume( vol );
}

void 
MixDeviceWidget::toggleRecsrc()
{
	setRecsrc( !m_mixdevice->isRecordable() );
}

void 
MixDeviceWidget::setDisabled( bool value )
{
   if ( m_disabled!=value)
   {
      value ? hide() : show();
      m_disabled = value;
      emit updateLayout();
   }
}

void 
MixDeviceWidget::defineKeys()
{
   if (m_keys) {
      KKeyDialog::configure(m_keys, 0, false);
      // The keys are saved in KMixerWidget::saveConfig, see kmixerwidget.cpp
      m_keys->updateConnections();
   }
}

void 
MixDeviceWidget::increaseVolume()
{
   Volume vol = m_mixdevice->getVolume();
   int inc = vol.maxVolume() / 20;
   if ( inc == 0 )
      inc = 1;
   for ( int i = 0; i < vol.channels(); i++ ) {
      int newVal = vol[i] + inc;
      setVolume( i, newVal < vol.maxVolume() ? newVal : vol.maxVolume() );
   }
}

void 
MixDeviceWidget::decreaseVolume()
{
   Volume vol = m_mixdevice->getVolume();
   int inc = vol.maxVolume() / 20;
   if ( inc == 0 )
      inc = 1;
   for ( int i = 0; i < vol.channels(); i++ ) {
      int newVal = vol[i] - inc;
      setVolume( i, newVal > 0 ? newVal : 0 );
   }
}

void 
MixDeviceWidget::setVolume( int channel, int vol )
{
   m_mixdevice->setVolume( channel, vol );
   emit newVolume( m_mixdevice->num(), m_mixdevice->getVolume() );

   if ( m_mixdevice->num()==m_mixer->masterDevice() )
       emit newMasterVolume( vol );
}

void 
MixDeviceWidget::setVolume( Volume vol )
{
   m_mixdevice->setVolume( vol );
   emit newVolume( m_mixdevice->num(), m_mixdevice->getVolume() );

   if ( m_mixdevice->num()==m_mixer->masterDevice() )
       emit newMasterVolume( vol );
}

void 
MixDeviceWidget::update()
{
   // update volumes
   Volume vol = m_mixdevice->getVolume();
   if( isStereoLinked() )
   {
		// calculate highest current volume (maxvol) of
		// all channels (typically: 1 <= vol.channels() <= 2 )
      int maxvol = 0;
      for( int i = 0; i < vol.channels(); i++ )
         maxvol = vol[i] > maxvol ? vol[i] : maxvol;

		// Set value of first Slider to maxvol
      QWidget *slider =  m_sliders.first();
      slider->blockSignals( true );
      if ( slider->inherits( "KSmallSlider" ) )
      {
         KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
         if (smallSlider) {
            smallSlider->setValue( maxvol );
            smallSlider->setGray( m_mixdevice->isMuted() );
         }
      } else
      {
         QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
         if (bigSlider)
            bigSlider->setValue( vol.maxVolume() - maxvol );
      }

      slider->blockSignals( false );
   }
   else
      for( int i=0; i<vol.channels(); i++ )
      {
         QWidget *slider = m_sliders.at( i );
         slider->blockSignals( true );

         if ( slider->inherits( "KSmallSlider" ) )
         {
            KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
            if (smallSlider) {
               smallSlider->setValue( vol[i] );
               smallSlider->setGray( m_mixdevice->isMuted() );
            }
         } 
			else
         {
            QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
            if (bigSlider)
               bigSlider->setValue( vol.maxVolume() - vol[i] );
         }

         slider->blockSignals( false );
      }

   // update mute led
   if ( m_muteLED )
   {
      m_muteLED->blockSignals( true );
      m_muteLED->setState( m_mixdevice->isMuted() ? KLed::Off : KLed::On );
      m_muteLED->blockSignals( false );
   }

   // update recsrc
   if( m_recordLED )
   {
      m_recordLED->blockSignals( true );
      m_recordLED->setState( m_mixdevice->isRecordable() ? KLed::On : KLed::Off );
      m_recordLED->blockSignals( false );
   }
}

void 
MixDeviceWidget::contextMenu()
{
   KPopupMenu *menu = new KPopupMenu( this );
   menu->insertTitle( SmallIcon( "kmix" ), m_mixdevice->name() );

   if ( m_sliders.count()>1 )
   {
      KToggleAction *stereo = (KToggleAction *)m_actions->action( "stereo" );
      if ( stereo )
      {
         stereo->setChecked( !isStereoLinked() );
         stereo->plug( menu );
      }
   }

   KToggleAction *ta = (KToggleAction *)m_actions->action( "recsrc" );
   if ( ta )
   {
      ta->setChecked( m_mixdevice->isRecordable() );
      ta->plug( menu );
   }

	ta = ( KToggleAction* )m_actions->action( "mute" );
   if ( ta )
   {
      ta->setChecked( m_mixdevice->isMuted() );
      ta->plug( menu );
   }
	
	KAction *a = m_actions->action(  "hide" );
	if ( a ) 
		a->plug(  menu );

   a = m_actions->action( "keys" );
   if ( a && m_keys ) 
	{
      KActionSeparator sep( this );
      sep.plug( menu );
		a->plug( menu );
   }

   KActionSeparator sep( this );
   sep.plug( menu );

   a = m_actions->action( "options_show_menubar" );
   if ( ta ) ta->plug( menu );

   a = m_actions->action( "toggle_channels" );
	if ( ta ) ta->plug( menu );
	
   QPoint pos = QCursor::pos();
   menu->popup( pos );
}

bool 
MixDeviceWidget::eventFilter( QObject* , QEvent* e )
{
   if (e->type() == QEvent::MouseButtonPress)
   {
      QMouseEvent *qme = (QMouseEvent*)e;
      if (qme->button() == RightButton) {
         emit rightMouseClick();
      }
   }
   else if (e->type() == QEvent::Wheel)
   {
      QWheelEvent *qwe = static_cast<QWheelEvent*>(e);
      if (qwe->delta() > 0)
      {
         increaseVolume();
      }
      else
      {
         decreaseVolume();
      }
      return true;
   }
   return false;
}

void 
MixDeviceWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      emit rightMouseClick();
}

#include "mixdevicewidget.moc"
