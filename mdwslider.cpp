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
    m_linked(true), m_iconLabel( 0 ), m_muteLED( 0 ), m_recordLED( 0 ), m_label( 0 ), _layout(0)
{
   // create actions (on _mdwActions, see MixDeviceWidget)

    // KStdAction::showMenubar() is in MixDeviceWidget now
    new KToggleAction( i18n("&Split Channels"), 0, this, SLOT(toggleStereoLinked()),
		      _mdwActions, "stereo" );
    new KToggleAction( i18n("&Hide"), 0, this, SLOT(setDisabled()), _mdwActions, "hide" );

    KToggleAction *a = new KToggleAction(i18n("&Muted"), 0, 0, 0, _mdwActions, "mute" );
    connect( a, SIGNAL(toggled(bool)), SLOT(toggleMuted()) );

    if( m_mixdevice->isRecordable() ) {
	a = new KToggleAction( i18n("Set &Record Source"), 0, 0, 0, _mdwActions, "recsrc" );
	connect( a, SIGNAL(toggled(bool)), SLOT( toggleRecsrc()) );
    }

    new KAction( i18n("C&onfigure Shortcuts..."), 0, this, SLOT(defineKeys()), _mdwActions, "keys" );

    // create widgets
    createWidgets( showMuteLED, showRecordLED );

    m_keys->insert( "Increase volume", i18n( "Increase Volume" ), QString::null,
		    KShortcut(), KShortcut(), this, SLOT( increaseVolume() ) );
    m_keys->insert( "Decrease volume", i18n( "Decrease Volume" ), QString::null,
		    KShortcut(), KShortcut(), this, SLOT( decreaseVolume() ) );
    m_keys->insert( "Toggle mute", i18n( "Toggle Mute" ), QString::null,
		    KShortcut(), KShortcut(), this, SLOT( toggleMuted() ) );

    // The keys are loaded in KMixerWidget::loadConfig, see kmixerwidget.cpp (now: kmixtoolbox.cpp)
    //m_keys->readSettings();
    //m_keys->updateConnections();

    installEventFilter( this ); // filter for popup
}

MDWSlider::~MDWSlider()
{
}


QSizePolicy MDWSlider::sizePolicy() const
{

    if ( _orientation == Qt::Vertical ) {
//	kdDebug(67100) << "MDWSlider::sizePolicy() vertical value=(Fixed,MinimumExpanding)\n";
	return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Expanding );
    }
    else {
//	kdDebug(67100) << "MDWSlider::sizePolicy() horizontal value=(MinimumExpanding,Fixed)\n";
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
    QToolTip::add( this, m_mixdevice->name() );


    // --- DEVICE ICON --------------------------
    // !!! Fixme: Correct check would be: "Left or Right". But we will add another parameter
    //            to the constructor (ViewFlags).
    if (true /* _orientation == Qt::Horizontal*/ ) {
	m_iconLabel = 0L;
	setIcon( m_mixdevice->type() );
	_layout->addWidget( m_iconLabel );
	 m_iconLabel->installEventFilter( this );
    } //  otherwise it is created after the slider

    _layout->addSpacing( 2 );


    // --- MUTE LED --------------------------
    if ( showMuteLED ) {
	if( m_mixdevice->hasMute() )
        {
	    // create mute LED
	    m_muteLED = new KLedButton( Qt::green, KLed::On, KLed::Sunken,
					KLed::Circular, this, "MuteLED" );
	    m_muteLED->setFixedSize( QSize(16, 16) );
	    m_muteLED->resize( QSize(16, 16) );
	    //m_muteLED->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
	    _layout->addWidget( m_muteLED );
	    QToolTip::add( m_muteLED, i18n( "Mute" ) );

	    connect( m_muteLED, SIGNAL(stateChanged(bool)), this, SLOT(toggleMuted()) );
	    m_muteLED->installEventFilter( this );
	} // has Mute LED
	else {
	    // we don't have a MUTE LED. We create a dummy widget
	    // !! possibly not neccesary any more (we are layouted)
	    QWidget *qw = new QWidget(this, "Spacer");
	    qw->setFixedSize( QSize(16, 16) );
	    _layout->addWidget(qw);
	    qw->installEventFilter( this );
	} // has no Mute LED

	_layout->addSpacing( 2 );
    } // showMuteLED


    // --- LABEL and SLIDER(S) --------------------------

    // create label
    QBoxLayout *labelAndSliders;
    if ( _orientation == Qt::Vertical ) {
	labelAndSliders = new QHBoxLayout( _layout );
	labelAndSliders->setAlignment(Qt::AlignVCenter);
    }
    else {
	labelAndSliders = new QVBoxLayout( _layout );
	labelAndSliders->setAlignment(Qt::AlignHCenter);
    }

    // --- Part 1: LABEL ---
    //m_label = new VerticalText( this, m_mixdevice->name().latin1() );
    if ( _orientation == Qt::Vertical ) {
	m_label = new VerticalText( this, m_mixdevice->name().utf8().data() );
	m_label->hide();
	labelAndSliders->addWidget( m_label );
	m_label->installEventFilter( this );
    }
    else {
	// !! later
	m_label = 0;
    }
		
    // --- Part 2: SLIDERS ---
    QBoxLayout *sliders;
    if ( _orientation == Qt::Vertical ) {
	sliders = new QHBoxLayout( labelAndSliders );
	sliders->setAlignment(Qt::AlignVCenter);
    }
    else {
	sliders = new QVBoxLayout( labelAndSliders );
	sliders->setAlignment(Qt::AlignHCenter);
    }


    for( int i = 0; i < m_mixdevice->getVolume().channels(); i++ )
    {
	Volume::ChannelID chid = Volume::ChannelID(i);
	// @todo !! Normally the mixdevicewidget SHOULD know, which slider represents which channel.
	// We should look up the mapping here, but for now, we simply assume "chid == i".

	int maxvol = m_mixdevice->getVolume().maxVolume();
	QWidget* slider;
	if ( m_small )
	{
	    slider = new KSmallSlider( 0, maxvol, maxvol/10,
				       m_mixdevice->getVolume( chid ),
				       _orientation,
				       this, m_mixdevice->name().ascii() );
	}
	else
	{
	    slider = new QSlider( 0, maxvol, maxvol/10,
				  maxvol - m_mixdevice->getVolume( chid ),
				  _orientation,
				  this, m_mixdevice->name().ascii() );
	    slider->setMinimumSize( slider->sizeHint() );
	}

	slider->installEventFilter( this );

	if( i>0 && isStereoLinked() ) {
	    // show only one (the first) slider, when the user wants it so
	    slider->hide();
	}
	sliders->addWidget( slider );  // add to layout
	m_sliders.append ( slider );   // add to list
	connect( slider, SIGNAL(valueChanged(int)), SLOT(volumeChange(int)) );
    } // for all channels of this device



    // --- DEVICE ICON --------------------------
    if ( false /*_orientation == Qt::Horizontal*/ ) {  // Fixme !!! see above
	/*    if ((m_direction == KPanelApplet::Right) || (m_direction == KPanelApplet::Down)) */
	m_iconLabel = 0L;
	setIcon( m_mixdevice->type() );
	_layout->addWidget( m_iconLabel );
	m_iconLabel->installEventFilter( this );
    } //  otherwise it is created before the slider


    if (showRecordLED) {
	_layout->addSpacing( 2 );
    }

    // --- RECORD SOURCE LED --------------------------
    if ( showRecordLED ) {
	if( m_mixdevice->isRecordable() )
        {
	    m_recordLED = new KLedButton( Qt::red,
					  m_mixdevice->isRecSource()?KLed::On:KLed::Off,
					  KLed::Sunken, KLed::Circular, this, "RecordLED" );
	    m_recordLED->setFixedSize( QSize(16, 16) );
	    _layout->addWidget( m_recordLED );
	    // @todo add Tooltip !!
	    connect(m_recordLED, SIGNAL(stateChanged(bool)), this, SLOT(setRecsrc(bool)));
	    m_recordLED->installEventFilter( this );
	}
	else
	{
	    // we don't have a RECORD LED. We create a dummy widget
	    // !! possibly not neccesary any more (we are layouted)
	    QWidget *qw = new QWidget(this, "Spacer");
	    qw->setFixedSize( QSize(16, 16) );
	    _layout->addWidget(qw);
	    qw->installEventFilter( this );
	} // has no Record LED
    } // showRecordLED
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
   for( slider=m_sliders.next(); slider!=0 ; slider=m_sliders.next() )
      value ? slider->hide() : slider->show();

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


/** This slot is called, when a user has changed the volume via the KMix Slider */
void MDWSlider::volumeChange( int )
{
    /* !! @todo What about the balance slider, when there is no master volume */
    //    kdDebug(67100) << "MDWSlider::volumeChange()" << endl;

   // --- Step 1: Get a REFERENCE of the volume Object ---
   Volume& vol = m_mixdevice->getVolume();

   // --- Step 2: Change the volumes directly in the Volume object to reflect the Sliders ---
   if ( isStereoLinked() )
   {
       //      kdDebug(67100) << "MDWSlider::volumeChange() stereoLinked vol=" << vol << endl;
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
   }
   else {
      int n = 0;
      for( QWidget *slider=m_sliders.first(); slider!=0; slider=m_sliders.next() )
      {
	  // @todo !! Normally the mixdevicewidget SHOULD know, which slider represents which channel.
	  // We should look up the mapping here, but for now, we simply assume "chid == n".
	  Volume::ChannelID chid = Volume::ChannelID(n);
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
		  vol.setVolume( chid, bigSlider->maxValue() - bigSlider->value() );
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

    //connect( m_mixer, SIGNAL(newRecsrc()), someview, SLOT(update());
    if (  m_mixdevice->isRecordable() ) {
	/*
	m_mixdevice->setRecSource( value );
	m_mixer->commitVolumeChange(m_mixdevice);
	*/
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
    //kdDebug(67100) << "MDWSlider::setDisabled()" << endl;
    setDisabled( true );
}

void MDWSlider::setDisabled( bool value )
{
    //kdDebug(67100) << "MDWSlider::setDisabled(bool)" << endl;
    if ( m_disabled!=value)
    {
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
   int inc = vol.maxVolume() / 20;
   if ( inc == 0 )
      inc = 1;
   for ( int i = 0; i < vol.channels(); i++ ) {
      int newVal = vol[i] + inc;
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
   int inc = vol.maxVolume() / 20;
   if ( inc == 0 )
      inc = 1;
   for ( int i = 0; i < vol.channels(); i++ ) {
      int newVal = vol[i] - inc;
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
	long avgVol = vol.getAvgVolume();

	QWidget *slider =  m_sliders.first();
	slider->blockSignals( true );
	if ( slider->inherits( "KSmallSlider" ) )
	{
	    KSmallSlider *smallSlider = dynamic_cast<KSmallSlider *>(slider);
	    if (smallSlider) {
		smallSlider->setValue( avgVol ); // !! inverted ?!?
		smallSlider->setGray( m_mixdevice->isMuted() );
	    }
	} // small slider
	else
	{
	    QSlider *bigSlider = dynamic_cast<QSlider *>(slider);
	    if (bigSlider)
		bigSlider->setValue( vol.maxVolume() - avgVol );  // !! inverted ?!?
	} // big slider

	slider->blockSignals( false );
    } // only 1 slider (stereo-linked)
    else {
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
	} // for all sliders
    } // more than 1 slider

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
      m_recordLED->setState( m_mixdevice->isRecSource() ? KLed::On : KLed::Off );
      m_recordLED->blockSignals( false );
   }
}

void MDWSlider::showContextMenu() {
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
	ta->setChecked( m_mixdevice->isRecordable() );
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

/*
void MDWSlider::resizeEvent ( QResizeEvent* ) {
    // kdDebug(67100) << "MDWSlider::resizeEvent(). Please resize to " << ev->size() << endl;
}
*/

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
