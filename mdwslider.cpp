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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <klocale.h>
#include <kled.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kaction.h>
#include <kmenu.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>
#include <kdebug.h>
#include <kactioncollection.h>
#include <kseparatoraction.h>
#include <ktoggleaction.h>

#include <qicon.h>
#include <qtoolbutton.h>
#include <QObject>
#include <qcursor.h>
#include <QMouseEvent>
#include <qslider.h>
#include <QLabel>
#include <QLayout>
#include <qpixmap.h>
#include <QToolTip>
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
                                 QWidget* parent, ViewBase* mw) :
    MixDeviceWidget(mixer,md,small,orientation,parent,mw),
    m_linked(true), m_iconLabel( 0 ), m_recordLED( 0 ), m_label( 0 ), _layout(0)
{
	// create actions (on _mdwActions, see MixDeviceWidget)

	KToggleAction *action = new KToggleAction( i18n("&Split Channels"), _mdwActions, "stereo" );
	connect(action, SIGNAL(triggered(bool) ), SLOT(toggleStereoLinked()));
	action = new KToggleAction( i18n("&Hide"), _mdwActions, "hide" );
	connect(action, SIGNAL(triggered(bool) ), SLOT(setDisabled()));

	KToggleAction *a = new KToggleAction(i18n("&Muted"), _mdwActions, "mute" );
	connect( a, SIGNAL(toggled(bool)), SLOT(toggleMuted()) );

	if( m_mixdevice->isRecordable() ) {
		a = new KToggleAction( i18n("Set &Record Source"), _mdwActions, "recsrc" );
		connect( a, SIGNAL(toggled(bool)), SLOT( toggleRecsrc()) );
	}

	KAction *c = new KAction( i18n("C&onfigure Shortcuts..."), _mdwActions, "keys" );
	connect(c, SIGNAL(triggered(bool) ), SLOT(defineKeys()));

	// create widgets
	createWidgets( showMuteLED, showRecordLED );

	KAction *b;
	b = new KAction( i18n( "Increase Volume" ), _mdwActions, "Increase volume" );
	connect(b, SIGNAL(triggered(bool) ), SLOT(increaseVolume()));

	b = new KAction( i18n( "Decrease Volume" ), _mdwActions, "Decrease volume" );
	connect(b, SIGNAL(triggered(bool) ), SLOT( decreaseVolume() ));

	b = new KAction( i18n( "Toggle mute" ), _mdwActions, "Toggle mute" );
	connect(b, SIGNAL(triggered(bool) ), SLOT( toggleMuted() ));

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
void MDWSlider::createWidgets( bool /*showMuteLED*/, bool showRecordLED )
{
   // !! remove the "showMuteLED" parameter (or let it apply to the icons)
   if ( _orientation == Qt::Vertical ) {
      _layout = new QVBoxLayout( this );
      _layout->setAlignment(Qt::AlignCenter);
   }
   else {
      _layout = new QHBoxLayout( this );
      _layout->setAlignment(Qt::AlignCenter);
   }
   _layout->setSpacing(0);
   _layout->setMargin(0);

   // -- MAIN SLIDERS LAYOUT  ---
   QBoxLayout *slidersLayout;
   if ( _orientation == Qt::Vertical ) {
      slidersLayout = new QHBoxLayout( );
      slidersLayout->setAlignment(Qt::AlignVCenter);
   }
   else {
      slidersLayout = new QVBoxLayout();
      slidersLayout->setAlignment(Qt::AlignHCenter);
   }
   _layout->addItem( slidersLayout );

   // -- LABEL LAYOUT TO POSITION
   QBoxLayout *labelLayout;
   if ( _orientation == Qt::Vertical ) {
      labelLayout = new QVBoxLayout( );
      slidersLayout->addItem( labelLayout );
      labelLayout->setAlignment(Qt::AlignHCenter);
   }
   else {
      labelLayout = new QHBoxLayout();
      slidersLayout->addItem( labelLayout );
      labelLayout->setAlignment(Qt::AlignVCenter);
   }
   if ( _orientation == Qt::Vertical ) {
      m_label = new VerticalText( this, m_mixdevice->name().toUtf8().data() );
   }
   else {
      m_label = new QLabel(this);
      static_cast<QLabel*>(m_label) ->setText(m_mixdevice->name());
   }

   m_label->hide();

   labelLayout->addWidget( m_label );
   m_label->installEventFilter( this );

   // -- SLIDERS, LEDS AND ICON
   QBoxLayout *sliLayout;
   if ( _orientation == Qt::Vertical ) {
      sliLayout = new QVBoxLayout();
      sliLayout->setAlignment(Qt::AlignHCenter);
   }
   else {
      sliLayout = new QHBoxLayout();
      sliLayout->setAlignment(Qt::AlignVCenter);
   }

   slidersLayout->addItem( sliLayout );
   // --- ICON  ----------------------------
   QBoxLayout *iconLayout;
   if ( _orientation == Qt::Vertical ) {
      iconLayout = new QVBoxLayout( );
      iconLayout->setAlignment(Qt::AlignCenter);
   }
   else {
      iconLayout = new QHBoxLayout( );
      iconLayout->setAlignment(Qt::AlignCenter);
   }
   sliLayout->addItem( iconLayout );
   iconLayout->setSizeConstraint(QLayout::SetFixedSize);

   m_iconLabel = 0L;
   setIcon( m_mixdevice->type() );
   iconLayout->addWidget( m_iconLabel );
   m_iconLabel->installEventFilter( this );
   QString muteTip( i18n( "Mute/Unmute %1", m_mixdevice->name() ) );
   m_iconLabel->setToolTip( muteTip );

   sliLayout->addSpacing( 3 );


    // --- SLIDERS ---------------------------
   QBoxLayout *volLayout;
   if ( _orientation == Qt::Vertical ) {
      volLayout = new QHBoxLayout( );
      volLayout->setAlignment(Qt::AlignVCenter);
   }
   else {
      volLayout = new QVBoxLayout(  );
      volLayout->setAlignment(Qt::AlignHCenter);
   }
   sliLayout->addItem( volLayout );


   // Sliders
   for( int i = 0; i < m_mixdevice->getVolume().count(); i++ )
   {
      Volume::ChannelID chid = Volume::ChannelID(i);
      // @todo !!! Normally the mixdevicewidget SHOULD know, which slider represents which channel.
      // We should look up the mapping here, but for now, we simply assume "chid == i".
   
      int maxvol = m_mixdevice->getVolume().maxVolume();
      int minvol = m_mixdevice->getVolume().minVolume();
   
      QWidget* slider;
      if ( m_small ) {
         slider = new KSmallSlider( minvol, maxvol, maxvol/10,
         m_mixdevice->getVolume( chid ), _orientation, this );
         slider->setObjectName(m_mixdevice->name());
      } // small
      else	{
         QSlider* sliderBig = new QSlider( _orientation, this );
         slider = sliderBig;
         sliderBig->setMinimum(0); // 
         sliderBig->setMaximum(maxvol);
         sliderBig->setPageStep(maxvol/10);
         sliderBig->setValue(maxvol - m_mixdevice->getVolume( chid ));
         sliderBig->setObjectName(m_mixdevice->name());
         if ( _orientation == Qt::Vertical ) {
            static_cast<QSlider*>(sliderBig)->setInvertedAppearance(true);
            static_cast<QSlider*>(sliderBig)->setInvertedControls(true);
         }
      } // not small

      slider->installEventFilter( this );
      slider->setToolTip( m_mixdevice->name() );

      if( i>0 && isStereoLinked() ) {
         // show only one (the first) slider, when the user wants it so
         slider->hide();
      }
      volLayout->addWidget( slider );  // add to layout
      m_sliders.append ( slider );   // add to list
      _slidersChids.append(chid);        // Remember slider-chid association
      connect( slider, SIGNAL(valueChanged(int)), SLOT(volumeChange(int)) );
   } // for all channels of this device


   // --- RECORD SOURCE LED --------------------------
   sliLayout->addSpacing( 3 );
   if ( showRecordLED )
   {
      // --- LED LAYOUT TO CENTER ---
      QBoxLayout *reclayout;
      if ( _orientation == Qt::Vertical ) {
         reclayout = new QVBoxLayout( );
         reclayout->setAlignment(Qt::AlignVCenter);
      }
      else {
         reclayout = new QHBoxLayout( );
         reclayout->setAlignment(Qt::AlignHCenter);
      }
      sliLayout->addItem( reclayout );
      reclayout->setSizeConstraint(QLayout::SetFixedSize);

      sliLayout->addSpacing( 3 );
      if( m_mixdevice->isRecordable() ) {
         m_recordLED = new KLedButton( Qt::red,
         m_mixdevice->isRecSource()?KLed::On:KLed::Off,
         KLed::Sunken, KLed::Circular, this, "RecordLED" );
         m_recordLED->setFixedSize( QSize(16, 16) );
         reclayout->addWidget( m_recordLED );
         connect(m_recordLED, SIGNAL(stateChanged(bool)), this, SLOT(setRecsrc(bool)));
         m_recordLED->installEventFilter( this );
         m_recordLED->setToolTip( i18n( "Record" ) );
      } // has Record LED
      else
      {
         // we don't have a RECORD LED. We create a dummy widget
         // !! possibly not necessary any more (we are layouted)
         QWidget *qw = new QWidget(this );
         qw->setObjectName( "Spacer" );
         qw->setFixedSize( QSize(16, 16) );
         reclayout->addWidget(qw);
         qw->installEventFilter( this );
      } // has no Record LED
   } // showRecordLED

   layout()->activate(); // Activate it explicitly in KDE3 because of PanelApplet/kicker issues
}


QPixmap MDWSlider::icon( int icontype )
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
      m_iconLabel = new QToolButton(this); //!!! TODO
      m_iconLabel->setCheckable(true);
      if( m_mixdevice->hasMute() ) {
        connect ( m_iconLabel, SIGNAL( toggled(bool) ), this, SLOT(toggleMuted() ) );
      }
      installEventFilter( m_iconLabel );
   }

   QPixmap miniDevPM = icon( icontype );
   if ( !miniDevPM.isNull() )
   {
      if ( m_small )
      {
         // scale icon
         QMatrix t;
         t = t.scale( 10.0/miniDevPM.width(), 10.0/miniDevPM.height() );
         // setIcon(QIcon(pixmap))
         m_iconLabel->setIcon( miniDevPM.transformed( t ) );
         m_iconLabel->resize( 10, 10 );
      } // small size
      else
      {
         QIcon icon(miniDevPM);
         icon.addPixmap( miniDevPM, QIcon::Normal, QIcon::On ) ;
         QPixmap pixmapOff = icon.pixmap(miniDevPM.size(), QIcon::Disabled, QIcon::Off);
         icon.addPixmap( pixmapOff, QIcon::Normal, QIcon::Off );
         m_iconLabel->setIcon( icon );
      } // normal size
   } else
   {
      kError(67100) << "Pixmap missing." << endl;
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
   if (m_sliders.count() == 0) return;
   m_linked = value;

   QWidget *slider = m_sliders[0];

   /***********************************************************
      Remember value of first slider, so that it can be copied
      to the other sliders.
    ***********************************************************/
   int firstSliderValue = 0;
   bool firstSliderValueValid = false;
   if (slider->metaObject()->className() == "QSlider") {
      QSlider *sld = static_cast<QSlider*>(slider);
      firstSliderValue = sld->value();
      firstSliderValueValid = true;
   }
   else if ( slider->metaObject()->className()=="KSmallSlider")  {
      KSmallSlider *sld = static_cast<KSmallSlider*>(slider);
      firstSliderValue = sld->value();
      firstSliderValueValid = true;
   }

   for( int i=1; i<m_sliders.count(); ++i ) {
      slider = m_sliders[i];
      if ( slider == 0 ) {
         continue;
      }
      if ( m_linked ) {
         slider->hide();
      }
      else {
         // When splitting, make the next sliders show the same value as the first.
         // This might not be entirely true, but better than showing the random value
         // that was used to be shown before hot-fixing this. !! must be revised
         if ( firstSliderValueValid ) {
            // Remark: firstSlider== 0 could happen, if the static_cast<QRangeControl*> above fails.
            //         It's a safety measure, if we got other Slider types in the future.
            if (slider->metaObject()->className()=="QSlider")  {
               QSlider *sld = static_cast<QSlider*>(slider);
               sld->setValue( firstSliderValue );
            }
            if (slider->metaObject()->className()=="KSmallSlider")  {
               KSmallSlider *sld = static_cast<KSmallSlider*>(slider);
               sld->setValue( firstSliderValue );
            }
         }
         slider->show();
      }
   }

   // Add tickmarks to last slider in the slider list
   slider = m_sliders.last();
   if( slider && static_cast<QSlider *>(slider)->tickPosition() )  // @todo How does this work?
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
   if (m_sliders.count() == 0) return;

   QWidget* slider = m_sliders[0];

	if ( slider->inherits( "QSlider" ) )
	{
		if( ticks )
			if( isStereoLinked() )
				static_cast<QSlider *>(slider)->setTickPosition( QSlider::Right );
			else
			{
				static_cast<QSlider *>(slider)->setTickPosition( QSlider::NoMarks );
				slider = m_sliders.last();
				static_cast<QSlider *>(slider)->setTickPosition( QSlider::Left );
			}
		else
		{
			static_cast<QSlider *>(slider)->setTickPosition( QSlider::NoMarks );
			slider = m_sliders.last();
			static_cast<QSlider *>(slider)->setTickPosition( QSlider::NoMarks );
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
    for( int i=0; i<m_sliders.count(); ++i ) {
        QWidget *slider = m_sliders[i];
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setColors( high, low, back );
    }
}

void
MDWSlider::setMutedColors( QColor high, QColor low, QColor back )
{
    for( int i=0; i<m_sliders.count(); ++i ) {
        QWidget *slider = m_sliders[i];
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setGrayColors( high, low, back );
    }
}


/** This slot is called, when a user has changed the volume via the KMix Slider */
void MDWSlider::volumeChange( int )
{
   if (m_sliders.count() == 0) return;

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
					sliderValue= slider->maximum() - slider->value();
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
         kDebug(67100) << "MDWSlider::volumeChange(), unknown chid " << chid << endl;
      }
   } // joined
   else {
      int n = 0;
      QList<Volume::ChannelID>::Iterator it = _slidersChids.begin();
      for( int i=0; i<m_sliders.count(); ++i ) {
          QWidget *slider = m_sliders[i];
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
					vol.setVolume( chid, bigSlider->maximum() - bigSlider->value() );
				else
					vol.setVolume( chid, bigSlider->value() );
	  }
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
		m_mixer->setRecordSource( m_mixdevice->id(), value );
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
   if (m_sliders.count() == 0) return;

	// update volumes
	Volume vol = m_mixdevice->getVolume();
	if( isStereoLinked() )
	{
		QList<Volume::ChannelID>::Iterator it = _slidersChids.begin();

		long avgVol = vol.getAvgVolume( Volume::MMAIN );

		QWidget *slider =  m_sliders.first();
		if ( slider == 0 ) {
			return;
		}
		slider->blockSignals( true );
		if ( slider->inherits( "KSmallSlider" ) )
		{
			KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
			if (smallSlider) {
				smallSlider->setValue( avgVol );
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

		slider->blockSignals( false );
	} // only 1 slider (stereo-linked)
	else {
		QList<Volume::ChannelID>::Iterator it = _slidersChids.begin();
		for( int i=0; i<vol.count(); i++, ++it ) {
			QWidget *slider = m_sliders.at( i );
			Volume::ChannelID chid = *it;
			if (slider == 0) {
				// !! The CHID thing should be done preperly
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

			slider->blockSignals( false );
		} // for all sliders
	} // more than 1 slider


        if( m_mixdevice->hasMute() ) {
           m_iconLabel->blockSignals( true );
           m_iconLabel->setChecked( m_mixdevice->isMuted() ? false : true );
           m_iconLabel->blockSignals( false );
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

	KMenu *menu = m_mixerwidget->getPopup();
	menu->addTitle( SmallIcon( "kmix" ), m_mixdevice->name() );

	if ( m_sliders.count()>1 ) {
		KToggleAction *stereo = (KToggleAction *)_mdwActions->action( "stereo" );
		if ( stereo ) {
			stereo->setChecked( !isStereoLinked() );
			menu->addAction( stereo );
		}
	}

	KToggleAction *ta = (KToggleAction *)_mdwActions->action( "recsrc" );
	if ( ta ) {
		ta->setChecked( m_mixdevice->isRecSource() );
		menu->addAction( ta );
	}

	if ( m_mixdevice->hasMute() ) {
		ta = ( KToggleAction* )_mdwActions->action( "mute" );
		if ( ta ) {
			ta->setChecked( m_mixdevice->isMuted() );
			menu->addAction( ta );
		}
	}

	KAction *a = _mdwActions->action(  "hide" );
	if ( a )
		menu->addAction( a );

	a = _mdwActions->action( "keys" );
	if ( a ) {
		KSeparatorAction sep( _mdwActions );
		menu->addAction( &sep );
		menu->addAction( a );
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
    else if ( (e->type() == QEvent::Wheel) && ! (obj->metaObject()->className()=="KSmallSlider") )  {
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
