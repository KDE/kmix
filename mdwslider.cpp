/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2007 Christian Esken <esken@kde.org>
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
#include <kdebug.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>

#include <qicon.h>
#include <qtoolbutton.h>
#include <QObject>
#include <qcursor.h>
#include <QCheckBox>
#include <QMouseEvent>
#include <qslider.h>
#include <QLabel>
#include <qpixmap.h>
#include <qwmatrix.h>
#include <QBoxLayout>

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
 * It can be configured to include or exclude the captureLED and the muteLED.
 * The direction (horizontal, vertical) can be configured and whether it should
 * be "small"  (uses KSmallSlider instead of QSlider then).
 *
 * Due to the many options, this is the most complicated MixDeviceWidget subclass.
 */
MDWSlider::MDWSlider(MixDevice* md,
                                 bool showMuteLED, bool showCaptureLED,
                                 bool small, Qt::Orientation orientation,
                                 QWidget* parent, ViewBase* mw) :
    MixDeviceWidget(md,small,orientation,parent,mw),
            m_linked(true), m_defaultLabelSpacer(0), m_iconLabelSimple(0), m_qcb(0), m_muteText(0),
            m_playbackSpacer(0), _layout(0), m_extraCaptureLabel( 0 ), m_label( 0 ),
            m_captureLED( 0 ), m_captureText(0), m_captureSpacer(0)
{
   // create actions (on _mdwActions, see MixDeviceWidget)

   KToggleAction *action = _mdwActions->add<KToggleAction>( "stereo" );
   action->setText( i18n("&Split Channels") );
   connect(action, SIGNAL(triggered(bool) ), SLOT(toggleStereoLinked()));
   action = _mdwActions->add<KToggleAction>( "hide" );
   action->setText( i18n("&Hide") );
   connect(action, SIGNAL(triggered(bool) ), SLOT(setDisabled()));

   if( m_mixdevice->playbackVolume().hasSwitch() ) {
      KToggleAction *a = _mdwActions->add<KToggleAction>( "mute" );
      a->setText( i18n("&Muted") );
      connect( a, SIGNAL(toggled(bool)), SLOT(toggleMuted()) );
   }

   if( m_mixdevice->captureVolume().hasSwitch() ) {
      KToggleAction *a = _mdwActions->add<KToggleAction>( "recsrc" );
      a->setText( i18n("Set &Record Source") );
      connect( a, SIGNAL(toggled(bool)), SLOT( toggleRecsrc()) );
   }

   KAction *c = _mdwActions->addAction( "keys" );
   c->setText( i18n("C&onfigure Shortcuts...") );
   connect(c, SIGNAL(triggered(bool) ), SLOT(defineKeys()));

   // create widgets
   createWidgets( showMuteLED, showCaptureLED );
   
   // The following actions are for the "Configure Shortcuts" dialog
   KAction *b;
   b = _mdwPopupActions->addAction( "Increase volume" );
   b->setText( i18n( "Increase Volume" ) );
   connect(b, SIGNAL(triggered(bool) ), SLOT(increaseVolume()));
   
   b = _mdwPopupActions->addAction( "Decrease volume" );
   b->setText( i18n( "Decrease Volume" ) );
   connect(b, SIGNAL(triggered(bool) ), SLOT( decreaseVolume() ));
   
   b = _mdwPopupActions->addAction( "Toggle mute" );
   b->setText( i18n( "Toggle mute" ) );
   connect(b, SIGNAL(triggered(bool) ), SLOT( toggleMuted() ));
/*
   b = _mdwActions->addAction( "Set Record Source" );
   b->setText( i18n( "Set Record Source" ) );
   connect(b, SIGNAL(triggered(bool) ), SLOT( toggleRecsrc() ));
*/ 
   installEventFilter( this ); // filter for popup

        update();
}


QSizePolicy MDWSlider::sizePolicy() const
{
    if ( _orientation == Qt::Vertical ) {
        return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::MinimumExpanding );
    }
    else {
        return QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    }
}

/* This method is a helper for users of this class who would like to show multiple MDWSlider, and align the sliders.
 * It returns the "height" (if Vertical) or width (of Horizontal) of the "playback portion" (e.g. Icon, Label, QCheckBox)
 */
int MDWSlider::playbackExtentHint() const
{
    int currentExtent = 0;
    if ( _orientation == Qt::Vertical ) {
        if ( m_qcb ) currentExtent += m_qcb->height();
        if ( m_muteText ) currentExtent += m_muteText->height();
    }
    else {
        if ( m_qcb ) currentExtent += m_qcb->width();
        if ( m_muteText ) currentExtent += m_muteText->width();
    }
    return currentExtent;
}

void MDWSlider::setPlaybackExtent(int extent) {
    if ( playbackExtentHint() < extent ) {
        if ( _orientation == Qt::Vertical )
             if ( playbackExtentHint() < extent )
               m_playbackSpacer->setFixedHeight(extent-playbackExtentHint());
        else
            if ( playbackExtentHint() < extent )
               m_playbackSpacer->setFixedWidth(extent-playbackExtentHint());
    }
}

/* This method is a helper for users of this class who would like to show multiple MDWSlider, and align the sliders.
 * It returns the "height" (if Vertical) or width (of Horizontal) of the "capture portion" (e.g. Label, QCheckBox)
 */
int MDWSlider::captureExtentHint() const
{
    int currentExtent = 0;
    if ( _orientation == Qt::Vertical ) {
        if ( m_captureLED ) currentExtent += m_captureLED->height();
        if ( m_captureText ) currentExtent += m_captureText->height();
    }
    else {
        if ( m_captureLED ) currentExtent += m_captureLED->width();
        if ( m_captureText ) currentExtent += m_captureText->width();
    }
    return currentExtent;
}

void MDWSlider::setCaptureExtent(int extent) {

    if ( _orientation == Qt::Vertical ) {
        m_defaultLabelSpacer->setFixedHeight(extent);
        if ( captureExtentHint() < extent )
            m_captureSpacer->setFixedHeight(extent-captureExtentHint());
    }
    else {
        m_defaultLabelSpacer->setFixedWidth(extent);
        if ( captureExtentHint() < extent )
            m_captureSpacer->setFixedWidth(extent-captureExtentHint());
    }
}


void MDWSlider::addDefaultLabel(QBoxLayout *layout, Qt::Orientation orientation)
{
    QBoxLayout *labelLayout;
    if ( orientation == Qt::Vertical ) {
        labelLayout = new QVBoxLayout( );
        labelLayout->setAlignment(Qt::AlignHCenter|Qt::AlignBottom);
        m_label = new VerticalText( this, m_mixdevice->readableName() ); // .toUtf8().data()
    }
    else {
        labelLayout = new QHBoxLayout();
        labelLayout->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
        m_label = new QLabel(this);
        static_cast<QLabel*>(m_label) ->setText(m_mixdevice->readableName());
    }
    //m_label->setToolTip( m_mixdevice->readableName() );  // @todo: Whatsthis, explaining the device
    m_label->installEventFilter( this );
    labelLayout->addWidget( m_label );
    layout->addItem( labelLayout );
    
    m_defaultLabelSpacer = new QWidget(this);
    labelLayout->addWidget( m_defaultLabelSpacer );
    m_defaultLabelSpacer->installEventFilter(this);
}


/**
 * Creates all widgets - Icon/Mute-Button, Slider(s) and capture-Button.
 *
 * Those widgets are placed into
 */
void MDWSlider::createWidgets( bool showMuteLED, bool showCaptureLED )
{
    // Base layout with two items: Left the Label, and Right the Sliders
   if ( _orientation == Qt::Vertical ) {
       _layout = new QHBoxLayout( this );
   }
   else {
       _layout = new QVBoxLayout( this );
   }
   _layout->setAlignment(Qt::AlignLeft|Qt::AlignTop);
   _layout->setSpacing(0);
   _layout->setMargin(0);

   bool hasVolumeSliders =  ( m_mixdevice->playbackVolume().count() + m_mixdevice->captureVolume().count() > 0 );
   
   // --- LABEL -----------------------------------------------
    if ( hasVolumeSliders ) {
        // When we have volume sliders, we put the label left of them (in an own layout). Otherwise see below at "if ( ! hasVolumeSliders )"
        addDefaultLabel( _layout, _orientation);
    }

    // The controlLayout holds three items: TopPart (Icon + MuteCheckBox + Label) ; MiddlePart (Sliders); Lower Part (CaptureCheckBox + Label)
    QBoxLayout *controlLayout;
    if ( _orientation == Qt::Vertical ) {
        controlLayout = new QVBoxLayout();
        controlLayout->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
    }
    else {
        controlLayout = new QHBoxLayout();
        controlLayout->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    }
    _layout->addItem( controlLayout );

   // --- ICON + Mute Switch  ----------------------------
    createWidgetsTopPart(controlLayout, showMuteLED);

    controlLayout->addSpacing( 3 );

    // --- SLIDERS ---------------------------
    QBoxLayout *volLayout;
    if ( _orientation == Qt::Vertical ) {
        volLayout = new QHBoxLayout( );
       // we use AlignBottom, so that Switches will be aligned with the bottom of the sliders
        volLayout->setAlignment(Qt::AlignHCenter|Qt::AlignBottom);
    }
    else {
        volLayout = new QVBoxLayout(  );
        volLayout->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    }
    controlLayout->addItem( volLayout );


   // -- SLIDERS, LEDS AND ICON
   if ( ! hasVolumeSliders ) {
        // When we don't have volume sliders, we but the label left of them (in an own layout). Otherwise see below
       addDefaultLabel( volLayout, _orientation );
   }
    else {
        bool bothCaptureANDPlaybackExist = ( m_mixdevice->playbackVolume().count() > 0 && m_mixdevice->captureVolume().count() > 0 );
        // --- SLIDERS -----------------------------------------------
        if ( m_mixdevice->playbackVolume().count() > 0 )
            addSliders( volLayout, 'p', false );
        if ( m_mixdevice->captureVolume().count() > 0 )
            addSliders( volLayout, 'c', bothCaptureANDPlaybackExist );
    }
   
    createWidgetsBottomPart(controlLayout, showCaptureLED);

   layout()->activate(); // Activate it explicitly in KDE3 because of PanelApplet/kicker issues
}


/* Creates the top part: Icon, PlaybackSwitch (Switch + Text) */
void MDWSlider::createWidgetsTopPart(QBoxLayout *layout, bool showMuteLED)
{
   QBoxLayout *m_iconLayout;
   if ( _orientation == Qt::Vertical ) {
      m_iconLayout = new QVBoxLayout( );
      m_iconLayout->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
   }
   else {
      m_iconLayout = new QHBoxLayout( );
      m_iconLayout->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
   }
   layout->addItem(m_iconLayout);

   m_iconLabelSimple = 0L;
   if ( showMuteLED ) {
        //kDebug(67100) << ">>> MixDevice " << m_mixdevice->readableName() << " icon calculation:";
       setIcon( m_mixdevice->type() );
       m_iconLayout->addWidget( m_iconLabelSimple );
       QString muteTip( m_mixdevice->readableName() );
       m_iconLabelSimple->setToolTip( muteTip );
       m_iconLayout->addSpacing( 3 );
       if ( m_mixdevice->playbackVolume().hasSwitch() ) {
            if ( m_mixdevice->playbackVolume().switchType() == Volume::OnSwitch ) {
               m_muteText = new QLabel("On", this); // l10n() this AFTER KDE4.0 !!
            }
            else if ( m_mixdevice->playbackVolume().switchType() == Volume::OffSwitch ) {
               m_muteText = new QLabel( "Off", this); // l10n() this AFTER KDE4.0 !!
            }
           else {
               m_muteText = new QLabel(i18n( "Mute"), this);
            }
           m_iconLayout->addWidget( m_muteText );
           m_muteText->installEventFilter(this);

           m_qcb =  new QCheckBox(this);
           m_iconLayout->addWidget( m_qcb );
           m_qcb->installEventFilter(this);
           connect ( m_qcb, SIGNAL( toggled(bool) ), this, SLOT(toggleMuted() ) );
           QString muteTip( i18n( "Mute/Unmute %1", m_mixdevice->readableName() ) );
           m_qcb->setToolTip( muteTip );
       } // can be muted
   }
   m_playbackSpacer = new QWidget(this);
   m_iconLayout->addWidget( m_playbackSpacer );
   m_playbackSpacer->installEventFilter(this);
}

void MDWSlider::createWidgetsBottomPart(QBoxLayout *layout, bool showCaptureLED)
{
       // --- capture SOURCE LED --------------------------
   if ( showCaptureLED ) {
      layout->addSpacing( 3 );
   }

      QBoxLayout *reclayout;
      if ( _orientation == Qt::Vertical ) {
         reclayout = new QVBoxLayout( );
         reclayout->setAlignment(Qt::AlignHCenter|Qt::AlignBottom);
      }
      else {
         reclayout = new QHBoxLayout( );
         reclayout->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
      }
      layout->addItem( reclayout );

      m_captureSpacer = new QWidget(this);
      reclayout->addWidget( m_captureSpacer );
      m_captureSpacer->installEventFilter(this);
        
   if ( showCaptureLED  && m_mixdevice->captureVolume().hasSwitch() )
   {
      m_captureLED =  new QCheckBox(this);
      reclayout->addWidget( m_captureLED );
      m_captureLED->installEventFilter( this );
      connect(m_captureLED, SIGNAL(toggled(bool)), this, SLOT(setRecsrc(bool)));
      QString muteTip( i18n( "Capture/Uncapture %1", m_mixdevice->readableName() ) );
      m_captureLED->setToolTip( muteTip );  // @todo: Whatsthis, explaining the device
      
      m_captureText = new QLabel(i18n("Capture"), this);
      reclayout->addWidget( m_captureText );
      m_captureText->installEventFilter(this);
   } // has capture LED
}
   
void MDWSlider::addSliders( QBoxLayout *volLayout, char type, bool addLabel)
{
   Volume* volP;
   QList<Volume::ChannelID>* ref_slidersChidsP;
   QList<QWidget *>* ref_slidersP;

   if ( type == 'c' ) { // capture
      volP              = & m_mixdevice->captureVolume();
      ref_slidersChidsP = & _slidersChidsCapture;
      ref_slidersP      = & m_slidersCapture;
   }
   else { // playback
      volP              = & m_mixdevice->playbackVolume();
      ref_slidersChidsP = & _slidersChidsPlayback;
      ref_slidersP      = & m_slidersPlayback;
   }

   Volume& vol = *volP;
   QList<Volume::ChannelID>& ref_slidersChids = *ref_slidersChidsP;
   QList<QWidget *>& ref_sliders = *ref_slidersP;


   if (addLabel)
   {
        static QString capture = i18n("(capture)");
        QString sliderDescription = m_mixdevice->readableName();
        if ( type == 'c' ) { // capture
            sliderDescription += ' ' + capture;
        }
        
        QWidget *label;
        if ( _orientation == Qt::Vertical ) {
            label = new VerticalText( this, sliderDescription );
        }
        else {
            label = new QLabel(this);
            static_cast<QLabel*>(m_label)->setText(sliderDescription);
        }
        volLayout->addWidget( label );
        label->installEventFilter( this );
        if ( type == 'c' ) { // capture
            m_extraCaptureLabel = label;
        }
        label->installEventFilter( this );
   }

    for( int i = 0; i < vol.count(); i++ )
    {
        Volume::ChannelID chid = Volume::ChannelID(i);

        long minvol = vol.minVolume();
        long maxvol = vol.maxVolume();

        QWidget* slider;
        if ( m_small ) {
            slider = new KSmallSlider( minvol, maxvol, (maxvol-minvol)/10, // @todo !! User definable steps
            vol.getVolume( chid ), _orientation, this );
        } // small
        else  {
            QSlider* sliderBig = new QSlider( _orientation, this );
            slider = sliderBig;
            sliderBig->setMinimum(0);
            sliderBig->setMaximum(maxvol);
            sliderBig->setPageStep(maxvol/10);
            sliderBig->setValue(maxvol - vol.getVolume( chid ));
        } // not small

        slider->installEventFilter( this );
        if ( type == 'p' ) {
            slider->setToolTip( m_mixdevice->readableName() );
        }
        else {
            QString captureTip( i18n( "%1 (capture)", m_mixdevice->readableName() ) );
            slider->setToolTip( captureTip );
        }
        
        if( i>0 && isStereoLinked() ) {
            // show only one (the first) slider, when the user wants it so
            slider->hide();
        }
        volLayout->addWidget( slider );  // add to layout
        ref_sliders.append ( slider );   // add to list
        ref_slidersChids.append(chid);
        connect( slider, SIGNAL(valueChanged(int)), SLOT(volumeChange(int)) );
    } // for all channels of this device
}


QPixmap MDWSlider::icon( int icontype )
{
   QPixmap miniDevPM;
   switch (icontype) {
      case MixDevice::AUDIO:
         miniDevPM = loadIcon( "mix_audio"); break;
      case MixDevice::BASS:
      case MixDevice::SURROUND_LFE:  // "LFE" SHOULD have an own icon
          miniDevPM = loadIcon("mix_bass"); break;
      case MixDevice::CD:
          miniDevPM = loadIcon("mix_cd"); break;
      case MixDevice::EXTERNAL:
         miniDevPM = loadIcon( "audio-input-line"); break;
      case MixDevice::MICROPHONE:
          miniDevPM = loadIcon("audio-input-microphone");break;
      case MixDevice::MIDI:
          miniDevPM = loadIcon("mix_midi"); break;
      case MixDevice::RECMONITOR:
          miniDevPM = loadIcon("mix_recmon"); break;
      case MixDevice::TREBLE:
          miniDevPM = loadIcon("mix_treble"); break;
      case MixDevice::UNKNOWN:
          miniDevPM = loadIcon("mix_unknown"); break;
      case MixDevice::VOLUME:
          miniDevPM = loadIcon("mix_volume"); break;
      case MixDevice::VIDEO:
          miniDevPM = loadIcon("mix_video"); break;
      case MixDevice::SURROUND:
      case MixDevice::SURROUND_BACK:
      case MixDevice::SURROUND_CENTERFRONT:
      case MixDevice::SURROUND_CENTERBACK:
          miniDevPM = loadIcon("mix_surround"); break;
      case MixDevice::HEADPHONE:
         miniDevPM = loadIcon( "audio-headset" ); break;
      case MixDevice::DIGITAL:
          miniDevPM = loadIcon( "mix_digital" ); break;
      case MixDevice::AC97:
          miniDevPM = loadIcon( "mix_ac97" ); break;
      default:
          miniDevPM = loadIcon("mix_unknown"); break;
   }

   return miniDevPM;
}

QPixmap MDWSlider::loadIcon( const char*  filename )
{
    return  KIconLoader::global()->loadIcon( filename, KIconLoader::Small, KIconLoader::SizeSmallMedium );
}

void
MDWSlider::setIcon( int icontype )
{
   if( !m_iconLabelSimple )
   {
         m_iconLabelSimple = new QLabel(this);
         installEventFilter( m_iconLabelSimple );
   }

   QPixmap miniDevPM = icon( icontype );
   if ( !miniDevPM.isNull() )
   {
      if ( m_small )
      {
         // scale icon
         QMatrix t;
         t = t.scale( 10.0/miniDevPM.width(), 10.0/miniDevPM.height() );
         m_iconLabelSimple->setPixmap( miniDevPM.transformed( t ) );
         m_iconLabelSimple->resize( 10, 10 );
      } // small size
      else
      {
            m_iconLabelSimple->setPixmap( miniDevPM );
	    //kDebug(67100) << " > simple > icontype=" <<icontype<< "size=" << miniDevPM.size();
      } // normal size
   }
   else
   {
      kError(67100) << "Pixmap missing." << endl;
   }

   layout()->activate();
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
   if (m_slidersPlayback.count() != 0) setStereoLinkedInternal(m_slidersPlayback);
   if (m_slidersCapture.count() != 0) setStereoLinkedInternal(m_slidersCapture);
   update(); // Call update(), so that the sliders can adjust EITHER to the individual values OR the average value.
}

void
MDWSlider::setStereoLinkedInternal(QList<QWidget *>& ref_sliders)
{
   QWidget *slider = ref_sliders[0];

   /***********************************************************
      Remember value of first slider, so that it can be copied
      to the other sliders.
    ***********************************************************/
   int firstSliderValue = 0;
   bool firstSliderValueValid = false;
   if (::qobject_cast<QSlider*>(slider)) {
      QSlider *sld = static_cast<QSlider*>(slider);
      firstSliderValue = sld->value();
      firstSliderValueValid = true;
   }
   else if ( ::qobject_cast<KSmallSlider*>(slider))  {
      KSmallSlider *sld = static_cast<KSmallSlider*>(slider);
      firstSliderValue = sld->value();
      firstSliderValueValid = true;
   }

   for( int i=1; i<ref_sliders.count(); ++i ) {
      slider = ref_sliders[i];
      if ( slider == 0 ) {
         continue;
      }
      if ( m_linked ) {
         slider->hide();
      }
      else {
         slider->show();
      }
   }

   // Redo the tickmarks to last slider in the slider list.
   // The implementation is not obvious, so lets explain:
   // We ALWAYS have tickmarks on the LAST slider. Sometimes the slider is not shown, and then we just don't bother.
   // a) So, if the last slider has tickmarks, we can always call setTicks( true ).
   // b) if the last slider has NO tickmarks, there ae no tickmarks at all, and we don't need to redo the tickmarks.
   slider = ref_sliders.last();
   if( slider && ( static_cast<QSlider *>(slider)->tickPosition() != QSlider::NoTicks) )
      setTicks( true );

}


void
MDWSlider::setLabeled(bool value)
{
    if ( m_label == 0  && m_extraCaptureLabel == 0 )
      return;

   if (value ) {
       if ( m_label != 0) m_label->show();
       if ( m_extraCaptureLabel != 0) m_extraCaptureLabel->show();
       if ( m_muteText != 0) m_muteText->show();
       if ( m_captureText != 0) m_captureText->show();
   }
   else {
       if ( m_label != 0) m_label->hide();
       if ( m_extraCaptureLabel != 0) m_extraCaptureLabel->hide();
       if ( m_muteText != 0) m_muteText->hide();
       if ( m_captureText != 0) m_captureText->hide();
   }
   layout()->activate();
}

void
MDWSlider::setTicks( bool value )
{
   if (m_slidersPlayback.count() != 0) setTicksInternal(m_slidersPlayback, value);
   if (m_slidersCapture.count() != 0) setTicksInternal(m_slidersCapture, value);
}

void
MDWSlider::setTicksInternal(QList<QWidget *>& ref_sliders, bool ticks)
{
  QWidget* slider = ref_sliders[0];

   if ( slider->inherits( "QSlider" ) )
   {
      if( ticks )
         if( isStereoLinked() )
            static_cast<QSlider *>(slider)->setTickPosition( QSlider::TicksRight );
         else
         {
            static_cast<QSlider *>(slider)->setTickPosition( QSlider::NoTicks );
            slider = ref_sliders.last();
            static_cast<QSlider *>(slider)->setTickPosition( QSlider::TicksLeft );
         }
      else
      {
         static_cast<QSlider *>(slider)->setTickPosition( QSlider::NoTicks );
         slider = ref_sliders.last();
         static_cast<QSlider *>(slider)->setTickPosition( QSlider::NoTicks );
      }
   }
}

void
MDWSlider::setIcons(bool value)
{
   if ( m_iconLabelSimple != 0 ) {
      if ( ( ! m_iconLabelSimple->isHidden()) !=value ) {
         if (value)
            m_iconLabelSimple->show();
         else
            m_iconLabelSimple->hide();

         layout()->activate();
      }
    } // if it has an icon
}

void
MDWSlider::setColors( QColor high, QColor low, QColor back )
{
    for( int i=0; i<m_slidersPlayback.count(); ++i ) {
        QWidget *slider = m_slidersPlayback[i];
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setColors( high, low, back );
    }
    for( int i=0; i<m_slidersCapture.count(); ++i ) {
        QWidget *slider = m_slidersCapture[i];
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setColors( high, low, back );
    }
}

void
MDWSlider::setMutedColors( QColor high, QColor low, QColor back )
{
    for( int i=0; i<m_slidersPlayback.count(); ++i ) {
        QWidget *slider = m_slidersPlayback[i];
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setGrayColors( high, low, back );
    }
    for( int i=0; i<m_slidersCapture.count(); ++i ) {
        QWidget *slider = m_slidersCapture[i];
        KSmallSlider *smallSlider = dynamic_cast<KSmallSlider*>(slider);
        if ( smallSlider ) smallSlider->setGrayColors( high, low, back );
    }
}


/** This slot is called, when a user has changed the volume via the KMix Slider. */
void MDWSlider::volumeChange( int )
{
   if (m_slidersPlayback.count() > 0) volumeChangeInternal(m_mixdevice->playbackVolume(), _slidersChidsPlayback, m_slidersPlayback);
   if (m_slidersCapture.count()  > 0) volumeChangeInternal(m_mixdevice->captureVolume() , _slidersChidsCapture, m_slidersCapture);
   m_mixdevice->mixer()->commitVolumeChange(m_mixdevice);
}

void MDWSlider::volumeChangeInternal( Volume& vol, QList<Volume::ChannelID>& ref_slidersChids, QList<QWidget *>& ref_sliders  )
{

   // --- Step 2: Change the volumes directly in the Volume object to reflect the Sliders ---
   if ( isStereoLinked() )
   {
      long firstVolume = 0;
      if ( ref_sliders.first()->inherits( "KSmallSlider" ) )
      {
         KSmallSlider *slider = dynamic_cast<KSmallSlider *>(ref_sliders.first());
         if (slider != 0 ) firstVolume = slider->value();
      }
      else
      {
         QSlider *slider = dynamic_cast<QSlider *>(ref_sliders.first());
         if (slider != 0 ) firstVolume = slider->value();
      }
      vol.setAllVolumes(firstVolume);
    } // stereoLinked()

    else {
        QList<Volume::ChannelID>::Iterator it = ref_slidersChids.begin();
        for( int i=0; i<ref_sliders.count(); i++, ++it ) {
            Volume::ChannelID chid = *it;
            QWidget *sliderWidget = ref_sliders[i];

            if ( sliderWidget->inherits( "KSmallSlider" ) )
            {
                KSmallSlider *slider = dynamic_cast<KSmallSlider *>(sliderWidget);
                if (slider) vol.setVolume( chid, slider->value() );
            }
            else
            {
                QSlider *slider = dynamic_cast<QSlider *>(sliderWidget);
                if (slider) vol.setVolume( chid, slider->value() );
            }
      } // iterate over all sliders
   } // !stereoLinked()

   // --- Step 3: Write back the new volumes to the HW ---
}


/**
   This slot is called, when a user has clicked the recsrc button. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::toggleRecsrc() {
   setRecsrc( m_mixdevice->isRecSource() );
}

void MDWSlider::setRecsrc(bool value )
{
   if (  m_mixdevice->captureVolume().hasSwitch() ) {
      m_mixdevice->setRecSource( value );
      m_mixdevice->mixer()->commitVolumeChange( m_mixdevice );
   }
}


/**
   This slot is called, when a user has clicked the mute button. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::toggleMuted() {
    setMuted( ! m_mixdevice->isMuted() );
}

void MDWSlider::setMuted(bool value)
{
     if (  m_mixdevice->playbackVolume().hasSwitch() ) {
      m_mixdevice->setMuted( value );
      m_mixdevice->mixer()->commitVolumeChange(m_mixdevice);
    }
}


void MDWSlider::setDisabled()
{
    setDisabled( true );
}

void MDWSlider::setDisabled( bool value )
{
    if ( m_disabled!=value)
    {
        value ? hide() : show();
        m_disabled = value;
         m_view->configurationUpdate();
    }
}


/**
   This slot is called on a MouseWheel event. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::increaseVolume()
{
   Volume& volP = m_mixdevice->playbackVolume();
    long inc = volP.maxVolume() / 20;
    if ( inc == 0 )
        inc = 1;
    for ( int i = 0; i < volP.count(); i++ ) {
        long newVal = (volP[i]) + inc;
        volP.setVolume( (Volume::ChannelID)i, newVal < volP.maxVolume() ? newVal : volP.maxVolume() );
    }

    Volume& volC = m_mixdevice->captureVolume();
    inc = volC.maxVolume() / 20;
    if ( inc == 0 )
        inc = 1;
    for ( int i = 0; i < volC.count(); i++ ) {
        long newVal = (volC[i]) + inc;
        volC.setVolume( (Volume::ChannelID)i, newVal < volC.maxVolume() ? newVal : volC.maxVolume() );
    }
    m_mixdevice->mixer()->commitVolumeChange(m_mixdevice);
}

/**
   This slot is called on a MouseWheel event. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSlider::decreaseVolume()
{
    Volume& volP = m_mixdevice->playbackVolume();
    long inc = volP.maxVolume() / 20;
    if ( inc == 0 )
        inc = 1;
    for ( int i = 0; i < volP.count(); i++ ) {
        long newVal = (volP[i]) - inc;
        volP.setVolume( (Volume::ChannelID)i, newVal > 0 ? newVal : 0 );
    }

    Volume& volC = m_mixdevice->captureVolume();
     inc = volC.maxVolume() / 20;
    if ( inc == 0 )
        inc = 1;
    for ( int i = 0; i < volC.count(); i++ ) {
        long newVal = (volC[i]) - inc;
        volC.setVolume( (Volume::ChannelID)i, newVal > 0 ? newVal : 0 );
    }
    m_mixdevice->mixer()->commitVolumeChange(m_mixdevice);
}


/**
   This is called whenever there are volume updates pending from the hardware for this MDW.
   At the moment it is called regulary via a QTimer (implicitely).
*/
void MDWSlider::update()
{
   if (m_slidersPlayback.count() != 0 || m_mixdevice->playbackVolume().hasSwitch())
      updateInternal(m_mixdevice->playbackVolume(), m_slidersPlayback, _slidersChidsPlayback);
   if (m_slidersCapture.count()  != 0 || m_mixdevice->captureVolume().hasSwitch())
      updateInternal(m_mixdevice->captureVolume(), m_slidersCapture , _slidersChidsCapture );
}

void MDWSlider::updateInternal(Volume& vol, QList<QWidget *>& ref_sliders, QList<Volume::ChannelID>& ref_slidersChids)
{
   // update volumes
   long useVolume = vol.getAvgVolume( Volume::MMAIN );


   QList<Volume::ChannelID>::Iterator it = ref_slidersChids.begin();
   for( int i=0; i<ref_sliders.count(); i++, ++it ) {
      if( ! isStereoLinked() ) {
         Volume::ChannelID chid = *it;
         useVolume = vol[chid];
      }
      QWidget *slider = ref_sliders.at( i );

      slider->blockSignals( true );

      if ( slider->inherits( "KSmallSlider" ) )
      {
         KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
         if (smallSlider) {
            smallSlider->setValue( useVolume );
            smallSlider->setGray( m_mixdevice->isMuted() );
         }
      }
      else
      {
         QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
         if (bigSlider)
               bigSlider->setValue( useVolume );
      }

      slider->blockSignals( false );
   } // for all sliders


   // update mute

   if( m_qcb != 0 ) {
/*
      m_iconLabelSimple->blockSignals( true );
      m_iconLabelSimple->setChecked( !m_mixdevice->isMuted() );
      m_iconLabelSimple->blockSignals( false );
*/
      m_qcb->blockSignals( true );
      m_qcb->setChecked( m_mixdevice->isMuted() );
      m_qcb->blockSignals( false );
   }

   // update recsrc
   if( m_captureLED ) {
      m_captureLED->blockSignals( true );
      m_captureLED->setChecked( m_mixdevice->isRecSource() );
 //     bool stateToShow = m_mixdevice->isRecSource();
//      m_captureLED->setState( stateToShow ? KLed::On : KLed::Off );
      m_captureLED->blockSignals( false );
   }
}

void MDWSlider::showContextMenu()
{
   if( m_view == 0 )
      return;
   
   KMenu *menu = m_view->getPopup();
   menu->addTitle( SmallIcon( "kmix" ), m_mixdevice->readableName() );
   
   if ( m_slidersPlayback.count()>1 || m_slidersCapture.count()>1) {
      KToggleAction *stereo = (KToggleAction *)_mdwActions->action( "stereo" );
      if ( stereo ) {
         stereo->setChecked( !isStereoLinked() );
         menu->addAction( stereo );
      }
   }
   
   if ( m_mixdevice->captureVolume().hasSwitch() ) {
      KToggleAction *ta = (KToggleAction *)_mdwActions->action( "recsrc" );
      if ( ta ) {
         ta->setChecked( m_mixdevice->isRecSource() );
         menu->addAction( ta );
      }
   }
   
   if ( m_mixdevice->playbackVolume().hasSwitch() ) {
      KToggleAction *ta = ( KToggleAction* )_mdwActions->action( "mute" );
      if ( ta ) {
         ta->setChecked( m_mixdevice->isMuted() );
         menu->addAction( ta );
      }
   }

   QAction *a = _mdwActions->action(  "hide" );
   if ( a )
      menu->addAction( a );
   
   QAction *b = _mdwActions->action( "keys" );
   if ( b ) {
//       QAction sep( _mdwPopupActions );
//       sep.setSeparator( true );
//       menu->addAction( &sep );
      menu->addAction( b );
   }
   
   QPoint pos = QCursor::pos();
   menu->popup( pos );
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
    else if ( (e->type() == QEvent::Wheel) 
            && strcmp(obj->metaObject()->className(),"KSmallSlider") != 0)  {
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
