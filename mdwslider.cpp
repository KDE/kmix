/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include <qobject.h>
#include <qcursor.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qwmatrix.h>

#include "mdwslider.h"
#include "mixer.h"
#include "viewbase.h"
#include "kledbutton.h"
#include "ksmallslider.h"
#include "verticaltext.h"

/**
 * MixDeviceWidget that represents a single mix device, inlcuding PopUp, muteLED, ...
 *
 * Used in KMix main window and DockWidget and PanelApplet.
 * It can be configured to include or exclude the recordLED and the muteLED.
 * The direction (horizontal, vertical) can be configured and whether it should
 * be "small"  (uses KSmallSlider instead of QSlider then).
 *
 * Due to the many options, this is the most complicated MixDeviceWidget subclass.
 */
MDWSlider::MDWSlider(Mixer *mixer, MixDevice* md,
                                 bool showMuteLED, bool showRecordLED,
                                 bool small, Qt::Orientation orientation,
                                 QWidget* parent, ViewBase* mw, const char* name) :
    MixDeviceWidget(mixer,md,small,orientation,parent,mw,name),
    m_linked(true), m_valueStyle( NNONE), m_iconLabel( 0 ), m_muteLED( 0 ), m_recordLED( 0 ), m_label( 0 ), _layout(0)
{
	// create actions (on _mdwActions, see MixDeviceWidget)

	new KToggleAction( i18n("&Split Channels"), 0, this, SLOT(toggleStereoLinked()),
			_mdwActions, "stereo" );
	new KToggleAction( i18n("&Hide"), 0, this, SLOT(setDisabled()), _mdwActions, "hide" );

	KToggleAction *a = new KToggleAction(i18n("&Muted"), 0, 0, 0, _mdwActions, "mute" );
	connect( a, SIGNAL(toggled(bool)), SLOT(toggleMuted()) );

	if( m_mixdevice->isRecordable() ) {
		a = new KToggleAction( i18n("Set &Record Source"), 0, 0, 0, _mdwActions, "recsrc" );
		connect( a, SIGNAL(toggled(bool)), SLOT( toggleRecsrc()) );
	}

	new KAction( i18n("C&onfigure Global Shortcuts..."), 0, this, SLOT(defineKeys()), _mdwActions, "keys" );

	// create widgets
	createWidgets( showMuteLED, showRecordLED );

	m_keys->insert( "Increase volume", i18n( "Increase Volume of '%1'" ).arg(m_mixdevice->name().utf8().data()), QString::null,
			KShortcut(), KShortcut(), this, SLOT( increaseVolume() ) );
	m_keys->insert( "Decrease volume", i18n( "Decrease Volume of '%1'" ).arg(m_mixdevice->name().utf8().data()), QString::null,
			KShortcut(), KShortcut(), this, SLOT( decreaseVolume() ) );
	m_keys->insert( "Toggle mute", i18n( "Toggle Mute of '%1'" ).arg(m_mixdevice->name().utf8().data()), QString::null,
			KShortcut(), KShortcut(), this, SLOT( toggleMuted() ) );

	installEventFilter( this ); // filter for popup

        update();
}


QSizePolicy MDWSlider::sizePolicy() const
{
	if ( _orientation == Qt::Vertical ) {
		return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Expanding );
	}
	else {
		return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	}
}


/**
 * Creates up to 4 widgets - Icon, Mute-Button, Slider and Record-Button.
 *
 * Those widgets are placed into

*/
void MDWSlider::createWidgets( bool showMuteLED, bool showRecordLED )
{
    if ( _orientation == Qt::Vertical ) {
	_layout = new QVBoxLayout( this );
	_layout->setAlignment(Qt::AlignCenter);
    }
    else {
	_layout = new QHBoxLayout( this );
	_layout->setAlignment(Qt::AlignCenter);
    }

	 // -- MAIN SLIDERS LAYOUT  ---
	 QBoxLayout *slidersLayout;
	 if ( _orientation == Qt::Vertical ) {
		 slidersLayout = new QHBoxLayout( _layout );
		 slidersLayout->setAlignment(Qt::AlignVCenter);
	 }
	 else {
		 slidersLayout = new QVBoxLayout( _layout );
		 slidersLayout->setAlignment(Qt::AlignHCenter);
	 }

	 /* cesken: This is inconsistent. Why should vertical and horizontal layout differ?
	  *         Also it eats too much space - especially when you don't show sliders at all.
	  *         Even more on the vertical panel applet (see Bug #97667)
	     if ( _orientation == Qt::Horizontal )
		 slidersLayout->addSpacing( 10 );
	 */


	 // -- LABEL LAYOUT TO POSITION
	 QBoxLayout *labelLayout;
	 if ( _orientation == Qt::Vertical ) {
		 labelLayout = new QVBoxLayout( slidersLayout );
		 labelLayout->setAlignment(Qt::AlignHCenter);
	 }
	 else {
		 labelLayout = new QHBoxLayout( slidersLayout );
		 labelLayout->setAlignment(Qt::AlignVCenter);
	 }
    if ( _orientation == Qt::Vertical ) {
		 m_label = new VerticalText( this, m_mixdevice->name().utf8().data() );
		QToolTip::add( m_label, m_mixdevice->name() );

	 }
	 else {
		 m_label = new QLabel(this);
		 static_cast<QLabel*>(m_label) ->setText(m_mixdevice->name());
		QToolTip::add( m_label, m_mixdevice->name() );
	 }

	 m_label->hide();

/* This addSpacing() looks VERY bizarre => removing it (cesken, 21.2.2006).
   Also horizontal and vertical spacing differs. This doesn't look sensible.
    if ( _orientation == Qt::Horizontal )
		 labelLayout->addSpacing( 36 );
*/
	 labelLayout->addWidget( m_label );
	 m_label->installEventFilter( this );

/* This addSpacing() looks VERY bizarre => removing it (cesken, 21.2.2006)
   Also horizontal and vertical spacing differs. This doesn't look sensible.
    if ( _orientation == Qt::Vertical ) {
		 labelLayout->addSpacing( 18 );
	 }
*/

	 // -- SLIDERS, LEDS AND ICON
	 QBoxLayout *sliLayout;
	 if ( _orientation == Qt::Vertical ) {
		 sliLayout = new QVBoxLayout( slidersLayout );
		 sliLayout->setAlignment(Qt::AlignHCenter);
    }
    else {
		 sliLayout = new QHBoxLayout( slidersLayout );
		 sliLayout->setAlignment(Qt::AlignVCenter);
	 }

	 // --- ICON  ----------------------------
    QBoxLayout *iconLayout;
    if ( _orientation == Qt::Vertical ) {
		 iconLayout = new QHBoxLayout( sliLayout );
		 iconLayout->setAlignment(Qt::AlignVCenter);
	 }
	 else {
		 iconLayout = new QVBoxLayout( sliLayout );
		 iconLayout->setAlignment(Qt::AlignHCenter);
	 }

	 m_iconLabel = 0L;
	 setIcon( m_mixdevice->type() );
	 iconLayout->addStretch();
	 iconLayout->addWidget( m_iconLabel );
	 iconLayout->addStretch();
	 m_iconLabel->installEventFilter( this );

	 sliLayout->addSpacing( 5 );


	 // --- MUTE LED
	 if ( showMuteLED ) {
		 QBoxLayout *ledlayout;
		 if ( _orientation == Qt::Vertical ) {
			 ledlayout = new QHBoxLayout( sliLayout );
			 ledlayout->setAlignment(Qt::AlignVCenter);
		 }
		 else {
			 ledlayout = new QVBoxLayout( sliLayout );
			 ledlayout->setAlignment(Qt::AlignHCenter);
		 }

		 if( m_mixdevice->hasMute() )
		 {
			 ledlayout->addStretch();
			 // create mute LED
			 m_muteLED = new KLedButton( Qt::green, KLed::On, KLed::Sunken,
					 KLed::Circular, this, "MuteLED" );
			 m_muteLED->setFixedSize( QSize(16, 16) );
			 m_muteLED->resize( QSize(16, 16) );
			 ledlayout->addWidget( m_muteLED );
			 QToolTip::add( m_muteLED, i18n( "Mute" ) );
			 connect( m_muteLED, SIGNAL(stateChanged(bool)), this, SLOT(toggleMuted()) );
			 m_muteLED->installEventFilter( this );
			 ledlayout->addStretch();
		 } // has Mute LED
		 else {
			 // we don't have a MUTE LED. We create a dummy widget
			 // !! possibly not neccesary any more (we are layouted)
			 QWidget *qw = new QWidget(this, "Spacer");
			 qw->setFixedSize( QSize(16, 16) );
			 ledlayout->addWidget(qw);
			 qw->installEventFilter( this );
		 } // has no Mute LED

		 sliLayout->addSpacing( 3 );
    } // showMuteLED

    // --- SLIDERS ---------------------------
	 QBoxLayout *volLayout;
	 if ( _orientation == Qt::Vertical ) {
		 volLayout = new QHBoxLayout( sliLayout );
		 volLayout->setAlignment(Qt::AlignVCenter);
	 }
	 else {
		 volLayout = new QVBoxLayout( sliLayout );
		 volLayout->setAlignment(Qt::AlignHCenter);
	 }

	 // Sliders and volume number indication
	 QBoxLayout *slinumLayout;
	 for( int i = 0; i < m_mixdevice->getVolume().count(); i++ )
    {
		 Volume::ChannelID chid = Volume::ChannelID(i);
		 // @todo !! Normally the mixdevicewidget SHOULD know, which slider represents which channel.
		 // We should look up the mapping here, but for now, we simply assume "chid == i".

		 int maxvol = m_mixdevice->getVolume().maxVolume();
		 int minvol = m_mixdevice->getVolume().minVolume();

		 if ( _orientation == Qt::Vertical ) {
		   slinumLayout = new QVBoxLayout( volLayout );
		   slinumLayout->setAlignment(Qt::AlignHCenter);
		 }
		 else {
		   slinumLayout = new QHBoxLayout( volLayout );
		   slinumLayout->setAlignment(Qt::AlignVCenter);
		 }

		 // create labels to hold volume values (taken from qamix/kamix)
		 QLabel *number = new QLabel( "100", this );
		 slinumLayout->addWidget( number );
		 number->setFrameStyle( QFrame::Panel | QFrame::Sunken );
		 number->setLineWidth( 2 );
		 number->setMinimumWidth( number->sizeHint().width() );
		 number->setPaletteBackgroundColor( QColor(190, 250, 190) );
		 // don't show the value by default
		 number->hide();
		 updateValue( number, chid );
		 _numbers.append( number );

		 QWidget* slider;
		 if ( m_small ) {
			 slider = new KSmallSlider( minvol, maxvol, maxvol/10,
					 m_mixdevice->getVolume( chid ), _orientation,
					 this, m_mixdevice->name().ascii() );
		 }
		 else	{
			 slider = new QSlider( 0, maxvol, maxvol/10,
					 maxvol - m_mixdevice->getVolume( chid ), _orientation,
					 this, m_mixdevice->name().ascii() );
			 slider->setMinimumSize( slider->sizeHint() );
		 }

		 slider->setBackgroundOrigin(AncestorOrigin);
		 slider->installEventFilter( this );
		 QToolTip::add( slider, m_mixdevice->name() );

		 if( i>0 && isStereoLinked() ) {
			 // show only one (the first) slider, when the user wants it so
			 slider->hide();
			 number->hide();
		 }
		 slinumLayout->addWidget( slider );  // add to layout
		 m_sliders.append ( slider );   // add to list
		 _slidersChids.append(chid);        // Remember slider-chid association
		 connect( slider, SIGNAL(valueChanged(int)), SLOT(volumeChange(int)) );
	 } // for all channels of this device


    // --- RECORD SOURCE LED --------------------------
    if ( showRecordLED )
	 {
		 sliLayout->addSpacing( 5 );

		 // --- LED LAYOUT TO CENTER ---
		 QBoxLayout *reclayout;
		 if ( _orientation == Qt::Vertical ) {
			 reclayout = new QHBoxLayout( sliLayout );
			 reclayout->setAlignment(Qt::AlignVCenter);
		 }
		 else {
			 reclayout = new QVBoxLayout( sliLayout );
			 reclayout->setAlignment(Qt::AlignHCenter);
		 }

		 if( m_mixdevice->isRecordable() ) {
			 reclayout->addStretch();
			 m_recordLED = new KLedButton( Qt::red,
					 m_mixdevice->isRecSource()?KLed::On:KLed::Off,
					 KLed::Sunken, KLed::Circular, this, "RecordLED" );
			 m_recordLED->setFixedSize( QSize(16, 16) );
			 reclayout->addWidget( m_recordLED );
			 connect(m_recordLED, SIGNAL(stateChanged(bool)), this, SLOT(setRecsrc(bool)));
			 m_recordLED->installEventFilter( this );
                         QToolTip::add( m_recordLED, i18n( "Record" ) );
			 reclayout->addStretch();
		 }
		 else
		 {
			 // we don't have a RECORD LED. We create a dummy widget
			 // !! possibly not neccesary any more (we are layouted)
			 QWidget *qw = new QWidget(this, "Spacer");
			 qw->setFixedSize( QSize(16, 16) );
			 reclayout->addWidget(qw);
			 qw->installEventFilter( this );
		 } // has no Record LED
	 } // showRecordLED

	layout()->activate();
}


QPixmap
MDWSlider::icon( int icontype )
{
   QPixmap miniDevPM;
   switch (icontype) {
      case MixDevice::AUDIO:
         miniDevPM = UserIcon("mix_audio"); break;
      case MixDevice::BASS:
      case MixDevice::SURROUND_LFE:  // "LFE" SHOULD have an own icon
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
      case MixDevice::SURROUND_BACK:
      case MixDevice::SURROUND_CENTERFRONT:
      case MixDevice::SURROUND_CENTERBACK:
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
MDWSlider::setIcon( int icontype )
{
   if( !m_iconLabel )
   {
      m_iconLabel = new QLabel(this);
      m_iconLabel->setBackgroundOrigin(AncestorOrigin);
      installEventFilter( m_iconLabel );
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
      kdError(67100) << "Pixmap missing." << endl;
   }

   layout()->activate();
}

bool
MDWSlider::isLabeled() const
{
    if ( m_label == 0 )
	return false;
    else
	return !m_label->isHidden();
}

void
MDWSlider::toggleStereoLinked()
{
   setStereoLinked( !isStereoLinked() );
}

void
MDWSlider::setStereoLinked(bool value)
{
   m_linked = value;

   QWidget *slider = m_sliders.first();
   QLabel *number = _numbers.first();
   QString qs = number->text();

   /***********************************************************
      Remember value of first slider, so that it can be copied
      to the other sliders.
    ***********************************************************/
   int firstSliderValue = 0;
   bool firstSliderValueValid = false;
   if (slider->isA("QSlider") ) {
      QSlider *sld = static_cast<QSlider*>(slider);
      firstSliderValue = sld->value();
      firstSliderValueValid = true;
   }
   else if ( slider->isA("KSmallSlider") ) {
      KSmallSlider *sld = static_cast<KSmallSlider*>(slider);
      firstSliderValue = sld->value();
      firstSliderValueValid = true;
   }

   for( slider=m_sliders.next(), number=_numbers.next();  slider!=0 && number!=0; slider=m_sliders.next(), number=_numbers.next() ) {
      if ( m_linked ) {
         slider->hide();
	 number->hide();
      }
      else {
         // When splitting, make the next sliders show the same value as the first.
         // This might not be entirely true, but better than showing the random value
         // that was used to be shown before hot-fixing this. !! must be revised
         if ( firstSliderValueValid ) {
            // Remark: firstSlider== 0 could happen, if the static_cast<QRangeControl*> above fails.
            //         It's a safety measure, if we got other Slider types in the future.
            if (slider->isA("QSlider") ) {
               QSlider *sld = static_cast<QSlider*>(slider);
               sld->setValue( firstSliderValue );
            }
            if (slider->isA("KSmallSlider") ) {
               KSmallSlider *sld = static_cast<KSmallSlider*>(slider);
               sld->setValue( firstSliderValue );
            }
         }
         slider->show();
	 number->setText(qs);
         if (m_valueStyle != NNONE)
	     number->show();
      }
   }

   slider = m_sliders.last();
   if( slider && static_cast<QSlider *>(slider)->tickmarks() )
      setTicks( true );

   layout()->activate();
}


void
MDWSlider::setLabeled(bool value)
{
    if ( m_label == 0 )
	return;

   if (value )
      m_label->show();
   else
      m_label->hide();

   layout()->activate();
}

void
MDWSlider::setTicks( bool ticks )
{
	QWidget* slider;

		slider = m_sliders.first();

	if ( slider->inherits( "QSlider" ) )
	{
		if( ticks )
			if( isStereoLinked() )
				static_cast<QSlider *>(slider)->setTickmarks( QSlider::Right );
			else
			{
				static_cast<QSlider *>(slider)->setTickmarks( QSlider::NoMarks );
				slider = m_sliders.last();
				static_cast<QSlider *>(slider)->setTickmarks( QSlider::Left );
			}
		else
		{
			static_cast<QSlider *>(slider)->setTickmarks( QSlider::NoMarks );
			slider = m_sliders.last();
			static_cast<QSlider *>(slider)->setTickmarks( QSlider::NoMarks );
		}
	}

	layout()->activate();
}

void
MDWSlider::setValueStyle( ValueStyle valueStyle )
{
    m_valueStyle = valueStyle;

    int i = 0;
    QValueList<Volume::ChannelID>::Iterator it = _slidersChids.begin();
    for(QLabel *number = _numbers.first(); number!=0; number = _numbers.next(), ++i, ++it) {
        Volume::ChannelID chid = *it;
        switch ( m_valueStyle ) {
	case NNONE: number->hide(); break;
	default: 
	    if ( !isStereoLinked() || (i == 0)) {
	      updateValue( number, chid );
	      number->show();
	    }
	}
    }
    layout()->activate();
}

void
MDWSlider::setIcons(bool value)
{
    if ( m_iconLabel != 0 ) {
	if ( ( !m_iconLabel->isHidden()) !=value ) {
	    if (value)
		m_iconLabel->show();
	    else
		m_iconLabel->hide();

	    layout()->activate();
	}
    } // if it has an icon
}

void
MDWSlider::setColors( QColor high, QColor low, QColor back )
{
    for( QWidget *slider=m_sliders.first(); slider!=0; slider=m_sliders.next() ) {
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setColors( high, low, back );
    }
}

void
MDWSlider::setMutedColors( QColor high, QColor low, QColor back )
{
    for( QWidget *slider=m_sliders.first(); slider!=0; slider=m_sliders.next() ) {
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setGrayColors( high, low, back );
    }
}

void 
MDWSlider::updateValue( QLabel *value, Volume::ChannelID chid ) {
    QString qs;
    Volume& vol = m_mixdevice->getVolume();

    if (m_valueStyle == NABSOLUTE )
      qs.sprintf("%3d",  (int) vol.getVolume( chid ) );
    else
        qs.sprintf("%3d", (int)( vol.getVolume( chid ) / (double)vol.maxVolume() * 100 ) );
    value->setText(qs);
}


/** This slot is called, when a user has changed the volume via the KMix Slider */
void MDWSlider::volumeChange( int )
{
   // --- Step 1: Get a REFERENCE of the volume Object ---
   Volume& vol = m_mixdevice->getVolume();

   // --- Step 2: Change the volumes directly in the Volume object to reflect the Sliders ---
   if ( isStereoLinked() )
   {
      QWidget *slider = m_sliders.first();
      Volume::ChannelID chid  = _slidersChids.first();

      int sliderValue = 0;
      if ( slider->inherits( "KSmallSlider" ) )
      {
         KSmallSlider *slider = dynamic_cast<KSmallSlider *>(m_sliders.first());
         if (slider) {
	    sliderValue= slider->value();
	 }
      }
      else {
         QSlider *slider = dynamic_cast<QSlider *>(m_sliders.first());
         if (slider) {
				if ( _orientation == Qt::Vertical )
					sliderValue= slider->maxValue() - slider->value();
				else
					sliderValue= slider->value();

         }
      }

		// With balance proper working, we must change relative volumes,
		// not absolute, which leads a make some difference calc related
		// to new sliders position against the top volume on channels
		long volumeDif =  sliderValue - vol.getTopStereoVolume( Volume::MMAIN );

      if ( chid == Volume::LEFT ) {
			vol.setVolume( Volume::LEFT , vol.getVolume( Volume::LEFT ) + volumeDif );
			vol.setVolume( Volume::RIGHT, vol.getVolume( Volume::RIGHT ) + volumeDif );
      }
      else {
         kdDebug(67100) << "MDWSlider::volumeChange(), unknown chid " << chid << endl;
      }

      updateValue( _numbers.first(), Volume::LEFT );
   } // joined
   else {
      int n = 0;
      QValueList<Volume::ChannelID>::Iterator it = _slidersChids.begin();
      QLabel *number = _numbers.first();
      for( QWidget *slider=m_sliders.first(); slider!=0 && number!=0; slider=m_sliders.next(), number=_numbers.next(), ++it )
      {
          Volume::ChannelID chid = *it;
	  if ( slider->inherits( "KSmallSlider" ) )
	  {
	      KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
	      if (smallSlider)
		  vol.setVolume( chid, smallSlider->value() );
	  }
	  else
	  {
	      QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
	      if (bigSlider)
				if ( _orientation == Qt::Vertical )
					vol.setVolume( chid, bigSlider->maxValue() - bigSlider->value() );
				else
					vol.setVolume( chid, bigSlider->value() );
	  }
	  updateValue( number, chid );
	  n++;
      }
   }

   // --- Step 3: Write back the new volumes to the HW ---
   m_mixer->commitVolumeChange(m_mixdevice);
}


/**
   This slot is called, when a user has clicked the recsrc button. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::toggleRecsrc() {
    setRecsrc( !m_mixdevice->isRecSource() );
}


void MDWSlider::setRecsrc(bool value )
{
	if (  m_mixdevice->isRecordable() ) {
		m_mixer->setRecordSource( m_mixdevice->num(), value );
	}
}


/**
   This slot is called, when a user has clicked the mute button. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::toggleMuted() {
    setMuted( !m_mixdevice->isMuted() );
}

void MDWSlider::setMuted(bool value)
{
    if (  m_mixdevice->hasMute() ) {
	m_mixdevice->setMuted( value );
	m_mixer->commitVolumeChange(m_mixdevice);
    }
}


void MDWSlider::setDisabled()
{
	setDisabled( true );
}

void MDWSlider::setDisabled( bool value )
{
	if ( m_disabled!=value)	{
		value ? hide() : show();
		m_disabled = value;
	}
}


/**
   This slot is called on a MouseWheel event. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::increaseVolume()
{
	Volume vol = m_mixdevice->getVolume();
	long inc = vol.maxVolume() / 20;
	if ( inc == 0 )
		inc = 1;
	for ( int i = 0; i < vol.count(); i++ ) {
		long newVal = (vol[i]) + inc;
		m_mixdevice->setVolume( i, newVal < vol.maxVolume() ? newVal : vol.maxVolume() );
	}
	m_mixer->commitVolumeChange(m_mixdevice);
}

/**
   This slot is called on a MouseWheel event. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::decreaseVolume()
{
	Volume vol = m_mixdevice->getVolume();
	long inc = vol.maxVolume() / 20;
	if ( inc == 0 )
		inc = 1;
	for ( int i = 0; i < vol.count(); i++ ) {
		long newVal = (vol[i]) - inc;
		m_mixdevice->setVolume( i, newVal > 0 ? newVal : 0 );
	}
	m_mixer->commitVolumeChange(m_mixdevice);
}


/**
   This is called whenever there are volume updates pending from the hardware for this MDW.
   At the moment it is called regulary via a QTimer (implicitely).
*/
void MDWSlider::update()
{
	// update volumes
	Volume vol = m_mixdevice->getVolume();
	if( isStereoLinked() )
	{
		QValueList<Volume::ChannelID>::Iterator it = _slidersChids.begin();

		long avgVol = vol.getAvgVolume( Volume::MMAIN );

		QWidget *slider =  m_sliders.first();
		if ( slider == 0 ) {
			return; // !!! Development version, check this !!!
		}
		slider->blockSignals( true );
		if ( slider->inherits( "KSmallSlider" ) )
		{
			KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
			if (smallSlider) {
				smallSlider->setValue( avgVol ); // !! inverted ?!?
				smallSlider->setGray( m_mixdevice->isMuted() );
			}
		} // small slider
		else {
			QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
			if (bigSlider)
			{
				// In case of stereo linked and single slider, slider must
				// show the top of both volumes, and not strangely low down
				// the main volume by half

				if ( _orientation == Qt::Vertical )
					bigSlider->setValue( vol.maxVolume() - vol.getTopStereoVolume( Volume::MMAIN ) );
				else
					bigSlider->setValue( vol.getTopStereoVolume( Volume::MMAIN ) );
			}
		} // big slider

		updateValue( _numbers.first(), Volume::LEFT );
		slider->blockSignals( false );
	} // only 1 slider (stereo-linked)
	else {
		QValueList<Volume::ChannelID>::Iterator it = _slidersChids.begin();
		for( int i=0; i<vol.count(); i++, ++it ) {
			QWidget *slider = m_sliders.at( i );
			Volume::ChannelID chid = *it;
			if (slider == 0) {
				// !!! not implemented !!!
				// not implemented: happens if there are record and playback
				// sliders in the same device. Or if you only show
				// the right slider (or any other fancy occasion)
				continue;
			}
			slider->blockSignals( true );

			if ( slider->inherits( "KSmallSlider" ) )
			{
				KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
				if (smallSlider) {
					smallSlider->setValue( vol[chid] );
					smallSlider->setGray( m_mixdevice->isMuted() );
				}
			}
			else
			{
				QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
				if (bigSlider)
					if ( _orientation == Qt::Vertical ) {
						bigSlider->setValue( vol.maxVolume() - vol[i] );
					}
					else {
						bigSlider->setValue( vol[i] );
				}
			}

			updateValue( _numbers.at ( i ), chid );

			slider->blockSignals( false );
		} // for all sliders
	} // more than 1 slider

	// update mute led
	if ( m_muteLED ) {
		m_muteLED->blockSignals( true );
		m_muteLED->setState( m_mixdevice->isMuted() ? KLed::Off : KLed::On );
		m_muteLED->blockSignals( false );
	}

	// update recsrc
	if( m_recordLED ) {
		m_recordLED->blockSignals( true );
		m_recordLED->setState( m_mixdevice->isRecSource() ? KLed::On : KLed::Off );
		m_recordLED->blockSignals( false );
	}
}

void MDWSlider::showContextMenu()
{
	if( m_mixerwidget == NULL )
		return;

	KPopupMenu *menu = m_mixerwidget->getPopup();
	menu->insertTitle( SmallIcon( "kmix" ), m_mixdevice->name() );

	if ( m_sliders.count()>1 ) {
		KToggleAction *stereo = (KToggleAction *)_mdwActions->action( "stereo" );
		if ( stereo ) {
			stereo->setChecked( !isStereoLinked() );
			stereo->plug( menu );
		}
	}

	KToggleAction *ta = (KToggleAction *)_mdwActions->action( "recsrc" );
	if ( ta ) {
		ta->setChecked( m_mixdevice->isRecSource() );
		ta->plug( menu );
	}

	if ( m_mixdevice->hasMute() ) {
		ta = ( KToggleAction* )_mdwActions->action( "mute" );
		if ( ta ) {
			ta->setChecked( m_mixdevice->isMuted() );
			ta->plug( menu );
		}
	}

	KAction *a = _mdwActions->action(  "hide" );
	if ( a )
		a->plug(  menu );

	a = _mdwActions->action( "keys" );
	if ( a && m_keys ) {
		KActionSeparator sep( this );
		sep.plug( menu );
		a->plug( menu );
	}

	QPoint pos = QCursor::pos();
	menu->popup( pos );
}

QSize MDWSlider::sizeHint() const {
	if ( _layout != 0 ) {
		return _layout->sizeHint();
	}
	else {
		// layout not (yet) created
		return QWidget::sizeHint();
	}
}

/**
 * An event filter for the various QWidgets. We watch for Mouse press Events, so
 * that we can popup the context menu.
 */
bool MDWSlider::eventFilter( QObject* obj, QEvent* e )
{
    if (e->type() == QEvent::MouseButtonPress) {
	QMouseEvent *qme = static_cast<QMouseEvent*>(e);
	if (qme->button() == Qt::RightButton) {
	    showContextMenu();
	    return true;
	}
    }
    // Attention: We don't filter WheelEvents for KSmallSlider, because it handles WheelEvents itself
    else if ( (e->type() == QEvent::Wheel) && !obj->isA("KSmallSlider") ) {
	QWheelEvent *qwe = static_cast<QWheelEvent*>(e);
	if (qwe->delta() > 0) {
	    increaseVolume();
	}
	else {
	    decreaseVolume();
	}
	return true;
    }
    return QWidget::eventFilter(obj,e);
}

#include "mdwslider.moc"
