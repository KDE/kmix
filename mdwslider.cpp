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
#include "mdwmoveaction.h"

static const int MIN_SLIDER_SIZE = 50;

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
MDWSlider::MDWSlider(MixDevice* md, bool showMuteLED, bool showCaptureLED,
		     bool includePlayback, bool includeCapture, 
                     bool small, Qt::Orientation orientation, QWidget* parent, ViewBase* mw) :
	MixDeviceWidget(md,small,orientation,parent,mw),
	m_linked(true),	muteButtonSpacer(0), captureSpacer(0), labelSpacer(0),
	m_iconLabelSimple(0), m_qcb(0), m_muteText(0),
	m_extraCaptureLabel( 0 ), m_label( 0 ), /*m_captureLED( 0 ),*/
	m_captureCheckbox(0), m_captureText(0), labelSpacing(0),
       muteButtonSpacing(false), captureLEDSpacing(false), m_moveMenu(0)
{
    _mdwMoveActions = new KActionCollection( this );

	// create actions (on _mdwActions, see MixDeviceWidget)

	KToggleAction *taction = _mdwActions->add<KToggleAction>( "stereo" );
	taction->setText( i18n("&Split Channels") );
	connect( taction, SIGNAL( triggered(bool) ), SLOT( toggleStereoLinked() ) );
	KAction *action = _mdwActions->add<KToggleAction>( "hide" );
	action->setText( i18n("&Hide") );
	connect( action, SIGNAL( triggered(bool) ), SLOT( setDisabled() ) );

	if( m_mixdevice->playbackVolume().hasSwitch() ) {
		taction = _mdwActions->add<KToggleAction>( "mute" );
		taction->setText( i18n("&Muted") );
		connect( taction, SIGNAL( toggled(bool) ), SLOT( toggleMuted() ) );
	}

	if( m_mixdevice->captureVolume().hasSwitch() ) {
		taction = _mdwActions->add<KToggleAction>( "recsrc" );
		taction->setText( i18n("Set &Record Source") );
		connect( taction, SIGNAL( toggled(bool) ), SLOT( toggleRecsrc() ) );
	}

	if( m_mixdevice->isMovable() ) {
		m_moveMenu = new KMenu( i18n("Mo&ve"), this);
		connect( m_moveMenu, SIGNAL(aboutToShow()), SLOT( showMoveMenu()) );
	}

	action = _mdwActions->addAction( "keys" );
	action->setText( i18n("C&onfigure Shortcuts...") );
	connect( action, SIGNAL( triggered(bool) ), SLOT( defineKeys() ) );

	// create widgets
	createWidgets( showMuteLED, showCaptureLED, includePlayback, includeCapture );

	// The following actions are for the "Configure Shortcuts" dialog
	/* PLEASE NOTE THAT global shortcuts are saved with the name as set with setName(), instead of their action name.
	   This is a bug according to the thread "Global shortcuts are saved with their text-name and not their action-name - Bug?" on kcd.
	   I work around this by using a text with setText() that is unique, but still readable to the user.
	 */
	QString actionSuffix  = QString(" - %1, %2").arg( mixDevice()->readableName() ).arg( mixDevice()->mixer()->readableName() );
	KAction *b;

	b = _mdwPopupActions->addAction( QString("Increase volume %1").arg( actionSuffix ) );
	QString increaseVolumeName = i18n( "Increase Volume" );
	increaseVolumeName += " - " + mixDevice()->readableName() + ", " + mixDevice()->mixer()->readableName();
	b->setText( increaseVolumeName  );
#ifdef __GNUC__
#warning GLOBAL SHORTCUTS ARE NOW ASSIGNED TO ALL CONTROLS, as enableGlobalShortcut(), has not been committed
#endif
	b->setGlobalShortcut(dummyShortcut);  // -<- enableGlobalShortcut() is not there => use workaround
	//   b->enableGlobalShortcut();
	connect( b, SIGNAL( triggered(bool) ), SLOT( increaseVolume() ) );

	b = _mdwPopupActions->addAction( QString("Decrease volume %1").arg( actionSuffix ) );
	QString decreaseVolumeName = i18n( "Decrease Volume" );
	decreaseVolumeName += " - " + mixDevice()->readableName() + ", " + mixDevice()->mixer()->readableName();
	b->setText( decreaseVolumeName );
#ifdef __GNUC__
#warning GLOBAL SHORTCUTS ARE NOW ASSIGNED TO ALL CONTROLS, as enableGlobalShortcut(), has not been committed
#endif
	b->setGlobalShortcut(dummyShortcut);  // -<- enableGlobalShortcut() is not there => use workaround
	//   b->enableGlobalShortcut();
	connect( b, SIGNAL( triggered(bool) ), SLOT( decreaseVolume() ) );

	b = _mdwPopupActions->addAction( QString("Toggle mute %1").arg( actionSuffix ) );
	QString muteVolumeName = i18n( "Toggle Mute" );
	muteVolumeName += " - " + mixDevice()->readableName() + ", " + mixDevice()->mixer()->readableName();
	b->setText( muteVolumeName );
#ifdef __GNUC__
#warning GLOBAL SHORTCUTS ARE NOW ASSIGNED TO ALL CONTROLS, as enableGlobalShortcut(), has not been committed
#endif
	b->setGlobalShortcut(dummyShortcut);  // -<- enableGlobalShortcut() is not there => use workaround
	//   b->enableGlobalShortcut();

	connect( b, SIGNAL( triggered(bool) ), SLOT( toggleMuted() ) );
	if (mw) mw->actionCollection()->addAction( QString("Toggle mute %1").arg( actionSuffix ), b );
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
		return QSizePolicy(  QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
	}
	else {
		return QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
	}
}

QSize MDWSlider::sizeHint() const
{
	return QSize( 90, QWidget::sizeHint().height());
}

/**
 * This method is a helper for users of this class who would like
 * to show multiple MDWSlider, and align the sliders.
 * It returns the "height" (if vertical) of this widgets label.
 * Warning: Line wraps are computed for a fixed size (100), this may be unaccurate in case,
 * the widgets have different sizes.
 */
int MDWSlider::labelExtentHint() const
{
	if ( _orientation == Qt::Vertical && m_label ) {
		return m_label->heightForWidth(m_label->minimumWidth());
	}
	return 0;
}

/**
 * If a label from another widget has more lines than this widget, then a spacer is added under the label
 */
void MDWSlider::setLabelExtent(int extent) {
	if ( _orientation == Qt::Vertical ) {
		if ( labelExtentHint() < extent )
			labelSpacer->setFixedHeight( extent - labelExtentHint() );
		else
			labelSpacer->setFixedHeight(0);
	}
}

/**
 * Alignment helper
 */
bool MDWSlider::hasMuteButton() const
{
	return m_qcb!=0;
}


/**
 * If this widget does not have a mute button, but another widget has, we add a spacer here with the
 * size of a QToolButton (don't know how to make a better estimate)
 */
void MDWSlider::setMuteButtonSpace(bool value)
{
	if (hasMuteButton() || !value) {
		muteButtonSpacer->setFixedSize(0,0);
		muteButtonSpacer->setVisible(false);
	} else {
		QToolButton b;
		muteButtonSpacer->setFixedSize( b.sizeHint() );
	}
}

/**
 * See "hasMuteButton"
 */
bool MDWSlider::hasCaptureLED() const
{
//	return m_captureLED!=0;
	return m_captureCheckbox!=0;
}

/**
 * See "setMuteButtonSpace"
 */
void MDWSlider::setCaptureLEDSpace(bool value)
{
	if ( !value || hasCaptureLED() ) {
		captureSpacer->setFixedSize(0,0);
		captureSpacer->setVisible(false);
	} else
		captureSpacer->setFixedSize(QCheckBox().sizeHint());
//		captureSpacer->setFixedSize(16,16);
}


/**
 * Creates all widgets : Icon, Label, Mute-Button, Slider(s) and Capture-Button.
 */
void MDWSlider::createWidgets( bool showMuteButton, bool showCaptureLED, bool includePlayback, bool includeCapture )
{
    bool wantsPlaybackSliders = includePlayback && ( m_mixdevice->playbackVolume().count() > 0 );
    bool wantsCaptureSliders  = includeCapture && ( m_mixdevice->playbackVolume().count() > 0 );
	bool hasVolumeSliders = wantsPlaybackSliders || wantsCaptureSliders;
	bool bothCaptureANDPlaybackExist = wantsPlaybackSliders && wantsCaptureSliders;
	
	// case of vertical sliders:
	if ( _orientation == Qt::Vertical )
	{
		QVBoxLayout *controlLayout = new QVBoxLayout(this);
		controlLayout->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
		setLayout(controlLayout);

		//add device icon
		m_iconLabelSimple = 0L;
		setIcon( m_mixdevice->iconName() );
		m_iconLabelSimple->setToolTip( m_mixdevice->readableName() );
		controlLayout->addWidget( m_iconLabelSimple, 0, Qt::AlignHCenter );

		//5px space
		controlLayout->addSpacing( 5 );

		//the device label
		m_label = new QLabel( m_mixdevice->readableName(), this);
		m_label->setWordWrap(true);
		int max = 0;
		QStringList words = m_mixdevice->readableName().split(QChar(' '));
		foreach (QString name, words)
			max = qMax(max,QLabel(name).sizeHint().width());
//		if (words.size()>1 && m_label)
//			m_label->setMinimumWidth(80);
//		if (m_label->sizeHint().width()>max && m_label->sizeHint().width()>80)
//			m_label->setMinimumWidth(max);
		m_label->setMinimumWidth(qMax(80,max));
		m_label->setMinimumHeight(m_label->heightForWidth(m_label->minimumWidth()));
		m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		m_label->setAlignment(Qt::AlignHCenter);
		controlLayout->addWidget(m_label, 0, Qt::AlignHCenter );

		//spacer with height to match height difference to other sliderwidgets
		labelSpacer = new QWidget(this);
		controlLayout->addWidget( labelSpacer );
		labelSpacer->installEventFilter(this);

		controlLayout->addSpacing( 3 );


		// sliders
		QHBoxLayout *volLayout = new QHBoxLayout( );
		volLayout->setAlignment(Qt::AlignHCenter|Qt::AlignBottom);
		volLayout->setSpacing(5);
		controlLayout->addItem( volLayout );

		if ( hasVolumeSliders )
		{
			if ( wantsPlaybackSliders )
				addSliders( volLayout, 'p', false );
			if ( wantsCaptureSliders )
				addSliders( volLayout, 'c', bothCaptureANDPlaybackExist );
			controlLayout->addSpacing( 3 );
		} else {
			controlLayout->addStretch(1);
		}

		//capture button
		if ( showCaptureLED  && m_mixdevice->captureVolume().hasSwitch() )
		{
//			m_captureLED =  new KLedButton(Qt::red,this);
//			m_captureLED->setFixedSize(16, 16 );
//			m_captureLED->installEventFilter( this );
//			controlLayout->addWidget( m_captureLED, 0, Qt::AlignHCenter );
//			connect( m_captureLED, SIGNAL( stateChanged(bool) ), this, SLOT( setRecsrc(bool) ) );
//			QString muteTip( i18n( "Capture/Uncapture %1", m_mixdevice->readableName() ) );
//			m_captureLED->setToolTip( muteTip );

			m_captureCheckbox = new QCheckBox( i18n("capture") , this);
			m_captureCheckbox->installEventFilter( this );
			controlLayout->addWidget( m_captureCheckbox, 0, Qt::AlignHCenter );
			connect( m_captureCheckbox, SIGNAL( toggled(bool)), this, SLOT( setRecsrc(bool) ) );
			QString muteTip( i18n( "Capture/Uncapture %1", m_mixdevice->readableName() ) );
			m_captureCheckbox->setToolTip( muteTip );

		}

		// spacer which is shown when no capture button present
		captureSpacer = new QWidget(this);
		controlLayout->addWidget( captureSpacer );
		captureSpacer->installEventFilter(this);


		//mute button
		if ( showMuteButton && m_mixdevice->playbackVolume().hasSwitch() )
		{
			m_qcb =  new QToolButton(this);
			m_qcb->setAutoRaise(true);
			m_qcb->setCheckable(false);

			m_qcb->setIcon( QIcon( loadIcon("audio-volume-muted") ) );

			controlLayout->addWidget( m_qcb , 0, Qt::AlignHCenter);
			m_qcb->installEventFilter(this);
			connect ( m_qcb, SIGNAL( clicked(bool) ), this, SLOT( toggleMuted() ) );
			QString muteTip( i18n( "Mute/Unmute %1", m_mixdevice->readableName() ) );
			m_qcb->setToolTip( muteTip );
		}

		//spacer shown, when no mute button is displayed
		muteButtonSpacer = new QWidget(this);
		controlLayout->addWidget( muteButtonSpacer );
		muteButtonSpacer->installEventFilter(this);



	}
	else
	{
		QVBoxLayout *_layout = new QVBoxLayout( this );

		QHBoxLayout *row1 = new QHBoxLayout();
		_layout->addItem( row1 );

		m_label = new QLabel(this);
		m_label->setText( m_mixdevice->readableName() );
		m_label->installEventFilter( this );
		row1->addWidget( m_label );
		row1->setAlignment(m_label, Qt::AlignVCenter);

		row1->addStretch();

		if ( showCaptureLED  && m_mixdevice->captureVolume().hasSwitch() )
		{

//			m_captureLED =  new KLedButton(Qt::red,this);
//			m_captureLED->setFixedSize(16, 16 );
//			controlLayout->addWidget( m_captureLED );
//			m_captureLED->installEventFilter( this );
//			connect( m_captureLED, SIGNAL( stateChanged(bool) ), this, SLOT( setRecsrc(bool) ) );
//			QString muteTip( i18n( "Capture/Uncapture %1", m_mixdevice->readableName() ) );
//			m_captureLED->setToolTip( muteTip );  // @todo: Whatsthis, explaining the device

			m_captureCheckbox = new QCheckBox( i18n("capture") , this);
			m_captureCheckbox->installEventFilter( this );
			row1->addWidget( m_captureCheckbox);
			row1->setAlignment(m_captureCheckbox, Qt::AlignRight);
			connect( m_captureCheckbox, SIGNAL( toggled(bool)), this, SLOT( setRecsrc(bool) ) );
			QString muteTip( i18n( "Capture/Uncapture %1", m_mixdevice->readableName() ) );
			m_captureCheckbox->setToolTip( muteTip );
		}


		QHBoxLayout *row2 = new QHBoxLayout();
		row2->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
		_layout->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
		_layout->addItem( row2 );


		m_iconLabelSimple = 0L;
		setIcon( m_mixdevice->iconName() );
		QString toolTip( m_mixdevice->readableName() );
		m_iconLabelSimple->setToolTip( toolTip );
		row2->addWidget( m_iconLabelSimple );
		row2->setAlignment(m_iconLabelSimple, Qt::AlignVCenter);


		captureSpacer = new QWidget(this);
//		controlLayout->addWidget( captureSpacer );
//		captureSpacer->installEventFilter(this);

		row2->addSpacing( 10 );

		// --- SLIDERS ---------------------------
		QBoxLayout *volLayout;
		volLayout = new QVBoxLayout(  );
		volLayout->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
		row2->addItem( volLayout );

		if ( hasVolumeSliders )
		{
			if ( m_mixdevice->playbackVolume().count() > 0 )
				addSliders( volLayout, 'p', false );
			if ( m_mixdevice->captureVolume().count() > 0 )
				addSliders( volLayout, 'c', bothCaptureANDPlaybackExist );
		}
		else
		{
			row2->addStretch(1);
		}

		if ( showMuteButton && m_mixdevice->playbackVolume().hasSwitch() )
		{
			row2->addSpacing( 3 );
			m_qcb =  new QToolButton(this);
			m_qcb->setAutoRaise(true);
			m_qcb->setCheckable(false);
			m_qcb->setIcon( QIcon( loadIcon("audio-volume-muted") ) );
			row2->addWidget( m_qcb );
			m_qcb->installEventFilter(this);
			connect ( m_qcb, SIGNAL( clicked(bool) ), this, SLOT( toggleMuted() ) );
			QString muteTip( i18n( "Mute/Unmute %1", m_mixdevice->readableName() ) );
			m_qcb->setToolTip( muteTip );
		}

		muteButtonSpacer = new QWidget(this);
		row2->addWidget( muteButtonSpacer );
		muteButtonSpacer->installEventFilter(this);
	}

	layout()->activate(); // Activate it explicitly in KDE3 because of PanelApplet/kicker issues
}


void MDWSlider::addSliders( QBoxLayout *volLayout, char type, bool addLabel)
{
	Volume* volP;
	QList<Volume::ChannelID>* ref_slidersChidsP;
	QList<QWidget *>* ref_slidersP;

	if ( type == 'c' ) { // capture
		volP              = &m_mixdevice->captureVolume();
		ref_slidersChidsP = &_slidersChidsCapture;
		ref_slidersP      = &m_slidersCapture;
	}
	else { // playback
		volP              = &m_mixdevice->playbackVolume();
		ref_slidersChidsP = &_slidersChidsPlayback;
		ref_slidersP      = &m_slidersPlayback;
	}

	Volume& vol = *volP;
	QList<Volume::ChannelID>& ref_slidersChids = *ref_slidersChidsP;
	QList<QWidget *>& ref_sliders = *ref_slidersP;


	if (addLabel)
	{
		static QString capture = i18n("capture");

		if ( type == 'c' ) { // capture
			if (_orientation == Qt::Horizontal)
				m_extraCaptureLabel = new QLabel(capture, this);
			else
				m_extraCaptureLabel = new VerticalText(this, capture);
		}
		m_extraCaptureLabel->installEventFilter( this );
		volLayout->addWidget(m_extraCaptureLabel);

	}

	for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
		if ( vol._chmask & Volume::_channelMaskEnum[i] ) {
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
				sliderBig->setValue( maxvol - vol.getVolume( chid ) );

				if ( _orientation == Qt::Vertical ) {
					sliderBig->setMinimumHeight( MIN_SLIDER_SIZE );
				}
				else {
					sliderBig->setMinimumWidth( MIN_SLIDER_SIZE );
				}
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
			volLayout->addWidget( slider ); // add to layout
			ref_sliders.append ( slider ); // add to list
			ref_slidersChids.append(chid);
			connect( slider, SIGNAL( valueChanged(int) ), SLOT( volumeChange(int) ) );
		} //if channel is present
	} // for all channels of this device
}

QPixmap MDWSlider::loadIcon( QString filename )
{
	return KIconLoader::global()->loadIcon( filename, KIconLoader::Small, KIconLoader::SizeSmallMedium );
}

void MDWSlider::setIcon( QString filename )
{
	if( !m_iconLabelSimple )
	{
		m_iconLabelSimple = new QLabel(this);
		installEventFilter( m_iconLabelSimple );
	}

	QPixmap miniDevPM = loadIcon( filename );
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
		} // normal size

		m_iconLabelSimple->setMinimumSize(22,22);
		m_iconLabelSimple->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
	}
	else
	{
		kError(67100) << "Pixmap missing." << endl;
	}
}

QString MDWSlider::iconName()
{
    return m_mixdevice->iconName();
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
	if ( ::qobject_cast<QSlider*>(slider) ) {
		QSlider *sld = static_cast<QSlider*>(slider);
		firstSliderValue = sld->value();
		firstSliderValueValid = true;
	}
	else if ( ::qobject_cast<KSmallSlider*>(slider) )  {
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
		if ( ( !m_iconLabelSimple->isHidden() ) !=value ) {
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
	if (m_slidersCapture.count()  > 0) volumeChangeInternal(m_mixdevice->captureVolume(), _slidersChidsCapture, m_slidersCapture);
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
			KSmallSlider *slider = dynamic_cast<KSmallSlider *>( ref_sliders.first() );
			if (slider != 0 ) firstVolume = slider->value();
		}
		else
		{
			QSlider *slider = dynamic_cast<QSlider *>( ref_sliders.first() );
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
	if ( m_mixdevice->captureVolume().hasSwitch() ) {
		m_mixdevice->setRecSource( value );
		m_mixdevice->mixer()->commitVolumeChange( m_mixdevice );
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
	if ( m_mixdevice->playbackVolume().hasSwitch() ) {
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


void MDWSlider::moveStreamAutomatic()
{
    m_mixdevice->mixer()->moveStream(m_mixdevice->id(), "");
}

void MDWSlider::moveStream(QString destId)
{
    m_mixdevice->mixer()->moveStream(m_mixdevice->id(), destId);
}

/**
   This is called whenever there are volume updates pending from the hardware for this MDW.
   At the moment it is called regulary via a QTimer (implicitely).
 */
void MDWSlider::update()
{
	if ( m_slidersPlayback.count() != 0 || m_mixdevice->playbackVolume().hasSwitch() )
		updateInternal(m_mixdevice->playbackVolume(), m_slidersPlayback, _slidersChidsPlayback);
	if ( m_slidersCapture.count()  != 0 || m_mixdevice->captureVolume().hasSwitch() )
		updateInternal(m_mixdevice->captureVolume(), m_slidersCapture, _slidersChidsCapture );
	if (m_label) {
		QLabel *l;
		VerticalText *v;
		if ((l = dynamic_cast<QLabel*>(m_label)))
			l->setText(m_mixdevice->readableName());
		else if ((v = dynamic_cast<VerticalText*>(m_label)))
			v->setText(m_mixdevice->readableName());
	}
}

void MDWSlider::updateInternal(Volume& vol, QList<QWidget *>& ref_sliders, QList<Volume::ChannelID>& ref_slidersChids)
{
	// update volumes
	long useVolume = vol.getAvgVolume( Volume::MMAIN );


	QList<Volume::ChannelID>::Iterator it = ref_slidersChids.begin();
	for( int i=0; i<ref_sliders.count(); i++, ++it ) {
		if( !isStereoLinked() ) {
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
		m_qcb->blockSignals( true );
		if (m_mixdevice->isMuted())
			m_qcb->setIcon( QIcon( loadIcon("audio-volume-muted") ) );
		else
			m_qcb->setIcon( QIcon( loadIcon("audio-volume-high") ) );
		m_qcb->blockSignals( false );
	}

	// update recsrc
//	if( m_captureLED ) {
//		m_captureLED->blockSignals( true );
//		m_captureLED->setState( m_mixdevice->isRecSource() ? KLed::On : KLed::Off );
//		m_captureLED->blockSignals( false );
//	}
	if( m_captureCheckbox ) {
		m_captureCheckbox->blockSignals( true );
		m_captureCheckbox->setChecked( m_mixdevice->isRecSource() );
		m_captureCheckbox->blockSignals( false );
	}

}

void MDWSlider::showContextMenu()
{
	if( m_view == 0 )
		return;

	KMenu *menu = m_view->getPopup();
	menu->addTitle( SmallIcon( "kmix" ), m_mixdevice->readableName() );

	if (m_moveMenu) {
		MixSet *ms = m_mixdevice->getMoveDestinationMixSet();
		Q_ASSERT(ms);

		m_moveMenu->setEnabled((ms->count() > 1));
		menu->addMenu( m_moveMenu );
	}

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


void MDWSlider::showMoveMenu()
{
    MixSet *ms = m_mixdevice->getMoveDestinationMixSet();
    Q_ASSERT(ms);

    _mdwMoveActions->clear();
    m_moveMenu->clear();

    // Default
    KAction *a = new KAction(_mdwMoveActions);
    a->setText( i18n("Automatic According to Category") );
    _mdwMoveActions->addAction( QString("moveautomatic"), a);
    connect(a, SIGNAL(triggered(bool)), SLOT(moveStreamAutomatic()));
    m_moveMenu->addAction( a );

    a = new KAction(_mdwMoveActions);
    a->setSeparator(true);
    _mdwMoveActions->addAction( QString("-"), a);

    m_moveMenu->addAction( a );
    for (int i = 0; i < ms->count(); ++i) {
        MixDevice* md = (*ms)[i];
        a = new MDWMoveAction(md, _mdwMoveActions);
        _mdwMoveActions->addAction( QString("moveto") + md->id(), a);
        connect(a, SIGNAL(moveRequest(QString)), SLOT(moveStream(QString)));
        m_moveMenu->addAction( a );
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
	else if ( (e->type() == QEvent::Wheel)
	         && strcmp(obj->metaObject()->className(),"KSmallSlider") != 0 )  {
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
