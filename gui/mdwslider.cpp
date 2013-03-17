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

#include "gui/mdwslider.h"

#include <klocale.h>
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
#include <QLabel>
#include <qpixmap.h>
#include <qwmatrix.h>
#include <QBoxLayout>

#include "core/mixer.h"
#include "gui/guiprofile.h"
#include "gui/volumeslider.h"
#include "gui/viewbase.h"
#include "gui/ksmallslider.h"
#include "gui/verticaltext.h"
#include "gui/mdwmoveaction.h"


VolumeSliderExtraData MDWSlider::DummVolumeSliderExtraData;
bool MDWSlider::debugMe = false;
 /**
 * MixDeviceWidget that represents a single mix device, including PopUp, muteLED, ...
 *
 * Used in KMix main window and DockWidget and PanelApplet.
 * It can be configured to include or exclude the captureLED and the muteLED.
 * The direction (horizontal, vertical) can be configured and whether it should
 * be "small"  (uses KSmallSlider instead of a normal slider widget).
 *
 * Due to the many options, this is the most complicated MixDeviceWidget subclass.
 */
MDWSlider::MDWSlider(shared_ptr<MixDevice> md, bool showMuteLED, bool showCaptureLED,
        bool small, Qt::Orientation orientation, QWidget* parent
        , ViewBase* view
        , ProfControl* par_ctl
        ) :
	MixDeviceWidget(md,small,orientation,parent,view, par_ctl),
	m_linked(true),	muteButtonSpacer(0), captureSpacer(0), labelSpacer(0),
	m_iconLabelSimple(0), m_qcb(0), m_muteText(0),
	m_label( 0 ), /*m_captureLED( 0 ),*/
	m_captureCheckbox(0), m_captureText(0), labelSpacing(0),
	muteButtonSpacing(false), captureLEDSpacing(false), _mdwMoveActions(new KActionCollection(this)), m_moveMenu(0),
	m_sliderInWork(0), m_waitForSoundSetComplete(0)
{
    createActions();
    createWidgets( showMuteLED, showCaptureLED );
    createShortcutActions();
    installEventFilter( this ); // filter for popup
    update();
}

MDWSlider::~MDWSlider()
{
	foreach( QAbstractSlider* slider, m_slidersPlayback)
	{
		delete slider;
	}
	foreach( QAbstractSlider* slider, m_slidersCapture)
	{
		delete slider;
	}
}

void MDWSlider::createActions()
{
    // create actions (on _mdwActions, see MixDeviceWidget)
    KToggleAction *taction = _mdwActions->add<KToggleAction>( "stereo" );
    taction->setText( i18n("&Split Channels") );
    connect( taction, SIGNAL(triggered(bool)), SLOT(toggleStereoLinked()) );

    KAction *action;
    if ( ! m_mixdevice->mixer()->isDynamic() ) {
        action = _mdwActions->add<KToggleAction>( "hide" );
        action->setText( i18n("&Hide") );
        connect( action, SIGNAL(triggered(bool)), SLOT(setDisabled()) );
    }

    if( m_mixdevice->hasMuteSwitch() )
    {
        taction = _mdwActions->add<KToggleAction>( "mute" );
        taction->setText( i18n("&Muted") );
        connect( taction, SIGNAL(toggled(bool)), SLOT(toggleMuted()) );
    }

    if( m_mixdevice->captureVolume().hasSwitch() ) {
        taction = _mdwActions->add<KToggleAction>( "recsrc" );
        taction->setText( i18n("Set &Record Source") );
        connect( taction, SIGNAL(toggled(bool)), SLOT(toggleRecsrc()) );
    }

    if( m_mixdevice->isMovable() ) {
        m_moveMenu = new KMenu( i18n("Mo&ve"), this);
        connect( m_moveMenu, SIGNAL(aboutToShow()), SLOT(showMoveMenu()) );
    }

    action = _mdwActions->addAction( "keys" );
    action->setText( i18n("C&onfigure Shortcuts...") );
    connect( action, SIGNAL(triggered(bool)), SLOT(defineKeys()) );
}

void MDWSlider::createShortcutActions()
{
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
//     #ifdef __GNUC__
//     #warning GLOBAL SHORTCUTS ARE NOW ASSIGNED TO ALL CONTROLS, as enableGlobalShortcut(), has not been committed
//     #endif
    if ( ! mixDevice()->mixer()->isDynamic() ) {
        // virtual / dynamic controls won't get shortcuts
        b->setGlobalShortcut(dummyShortcut);  // -<- enableGlobalShortcut() is not there => use workaround
        //   b->enableGlobalShortcut();
        connect( b, SIGNAL(triggered(bool)), SLOT(increaseVolume()) );
    }

    b = _mdwPopupActions->addAction( QString("Decrease volume %1").arg( actionSuffix ) );
    QString decreaseVolumeName = i18n( "Decrease Volume" );
    decreaseVolumeName += " - " + mixDevice()->readableName() + ", " + mixDevice()->mixer()->readableName();
    b->setText( decreaseVolumeName );
/*    #ifdef __GNUC__
    #warning GLOBAL SHORTCUTS ARE NOW ASSIGNED TO ALL CONTROLS, as enableGlobalShortcut(), has not been committed
    #endif*/
    if ( ! mixDevice()->mixer()->isDynamic() ) {
        // virtual / dynamic controls won't get shortcuts
        b->setGlobalShortcut(dummyShortcut);  // -<- enableGlobalShortcut() is not there => use workaround
        //   b->enableGlobalShortcut();
        connect( b, SIGNAL(triggered(bool)), SLOT(decreaseVolume()) );
    }

    b = _mdwPopupActions->addAction( QString("Toggle mute %1").arg( actionSuffix ) );
    QString muteVolumeName = i18n( "Toggle Mute" );
    muteVolumeName += " - " + mixDevice()->readableName() + ", " + mixDevice()->mixer()->readableName();
    b->setText( muteVolumeName );
/*    #ifdef __GNUC__
    #warning GLOBAL SHORTCUTS ARE NOW ASSIGNED TO ALL CONTROLS, as enableGlobalShortcut(), has not been committed
    #endif*/
    if ( ! mixDevice()->mixer()->isDynamic() ) {
        // virtual / dynamic controls won't get shortcuts
        b->setGlobalShortcut(dummyShortcut);  // -<- enableGlobalShortcut() is not there => use workaround
        //   b->enableGlobalShortcut();
        connect( b, SIGNAL(triggered(bool)), SLOT(toggleMuted()) );
    }

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
void MDWSlider::createWidgets( bool showMuteButton, bool showCaptureLED )
{
    bool includePlayback = _pctl->useSubcontrolPlayback();
    bool includeCapture = _pctl->useSubcontrolCapture();
    bool wantsPlaybackSliders = includePlayback && ( m_mixdevice->playbackVolume().count() > 0 );
    bool wantsCaptureSliders  = includeCapture && ( m_mixdevice->captureVolume().count() > 0 );
      bool hasVolumeSliders = wantsPlaybackSliders || wantsCaptureSliders;
     // bool bothCaptureANDPlaybackExist = wantsPlaybackSliders && wantsCaptureSliders;
	
      bool wantsMediaControls = ( m_mixdevice->hasMediaNextControl() || m_mixdevice->hasMediaPlayControl() || m_mixdevice->hasMediaPrevControl() );

      // case of vertical sliders:
	if ( _orientation == Qt::Vertical )
	{
		QVBoxLayout *controlLayout = new QVBoxLayout(this);
		controlLayout->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
		setLayout(controlLayout);
        controlLayout->setContentsMargins(0,0,0,0);

		//add device icon
		m_iconLabelSimple = 0L;
		setIcon( m_mixdevice->iconName() );
		m_iconLabelSimple->setToolTip( m_mixdevice->readableName() );
		controlLayout->addWidget( m_iconLabelSimple, 0, Qt::AlignHCenter );

		//5px space
		//controlLayout->addSpacing( 5 );

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

		// sliders
		QHBoxLayout *volLayout = new QHBoxLayout( );
		volLayout->setAlignment(Qt::AlignHCenter|Qt::AlignBottom);
		//volLayout->setSpacing(5);
		controlLayout->addItem( volLayout );

		if ( hasVolumeSliders )
		{
			if ( wantsPlaybackSliders )
				addSliders( volLayout, 'p', m_mixdevice->playbackVolume(), m_slidersPlayback);
			if ( wantsCaptureSliders )
				addSliders( volLayout, 'c', m_mixdevice->captureVolume() , m_slidersCapture );
			if ( wantsMediaControls )
				addMediaControls( volLayout ); // Please note that the addmediaControls() is in the hasVolumeSliders check onyl because it was easier to integrate
			controlLayout->addSpacing( 3 );
		} else {
			controlLayout->addStretch(1);
		}

		//capture button
		if ( showCaptureLED && includeCapture && m_mixdevice->captureVolume().hasSwitch() )
		{
			m_captureCheckbox = new QCheckBox( i18n("capture") , this);
			m_captureCheckbox->installEventFilter( this );
			controlLayout->addWidget( m_captureCheckbox, 0, Qt::AlignHCenter );
			connect( m_captureCheckbox, SIGNAL(toggled(bool)), this, SLOT(setRecsrc(bool)) );
			QString muteTip( i18n( "Capture/Uncapture %1", m_mixdevice->readableName() ) );
			m_captureCheckbox->setToolTip( muteTip );
		}

		// spacer which is shown when no capture button present
		captureSpacer = new QWidget(this);
		controlLayout->addWidget( captureSpacer );
		captureSpacer->installEventFilter(this);


		//mute button
		if ( showMuteButton && includePlayback && m_mixdevice->hasMuteSwitch() )
		{
			m_qcb =  new QToolButton(this);
			m_qcb->setAutoRaise(true);
			m_qcb->setCheckable(false);

			m_qcb->setIcon( QIcon( loadIcon("audio-volume-muted") ) );

			controlLayout->addWidget( m_qcb , 0, Qt::AlignHCenter);
			m_qcb->installEventFilter(this);
			connect ( m_qcb, SIGNAL(clicked(bool)), this, SLOT(toggleMuted()) );
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

		if ( showCaptureLED && includeCapture && m_mixdevice->captureVolume().hasSwitch() )
		{
			m_captureCheckbox = new QCheckBox( i18n("capture") , this);
			m_captureCheckbox->installEventFilter( this );
			row1->addWidget( m_captureCheckbox);
			row1->setAlignment(m_captureCheckbox, Qt::AlignRight);
			connect( m_captureCheckbox, SIGNAL(toggled(bool)), this, SLOT(setRecsrc(bool)) );
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

		//row2->addSpacing( 10 );

		// --- SLIDERS ---------------------------
		QBoxLayout *volLayout;
		volLayout = new QVBoxLayout(  );
		volLayout->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
		row2->addItem( volLayout );

		if ( hasVolumeSliders )
		{
			if ( wantsPlaybackSliders && m_mixdevice->playbackVolume().count() > 0 )
				addSliders( volLayout, 'p', m_mixdevice->playbackVolume(), m_slidersPlayback );
			if ( wantsCaptureSliders && m_mixdevice->captureVolume().count() > 0 )
				addSliders( volLayout, 'c', m_mixdevice->captureVolume() , m_slidersCapture );
			if ( wantsMediaControls )
				addMediaControls( volLayout );
		}

		if ( showMuteButton && includePlayback && m_mixdevice->hasMuteSwitch() )
		{
			m_qcb =  new QToolButton(this);
			m_qcb->setAutoRaise(true);
			m_qcb->setCheckable(false);
			m_qcb->setIcon( QIcon( loadIcon("audio-volume-muted") ) );
			row2->addWidget( m_qcb );
			m_qcb->installEventFilter(this);
			connect ( m_qcb, SIGNAL(clicked(bool)), this, SLOT(toggleMuted()) );
			QString muteTip( i18n( "Mute/Unmute %1", m_mixdevice->readableName() ) );
			m_qcb->setToolTip( muteTip );
		}

		muteButtonSpacer = new QWidget(this);
		row2->addWidget( muteButtonSpacer );
		muteButtonSpacer->installEventFilter(this);
	}

	bool stereoLinked = !_pctl->isSplit();
	setStereoLinked( stereoLinked );
	
	layout()->activate(); // Activate it explicitly in KDE3 because of PanelApplet/kicker issues
}

    void MDWSlider::addMediaControls(QBoxLayout* volLayout)
    {
      QBoxLayout *mediaLayout;
      if ( _orientation == Qt::Vertical )
	mediaLayout = new QVBoxLayout();
      else
	mediaLayout = new QHBoxLayout();

      if ( mixDevice()->hasMediaPrevControl())
      {
	QToolButton *lbl = addMediaButton("media-skip-backward", mediaLayout);
	connect(lbl, SIGNAL(clicked(bool)), this, SLOT(mediaPrev(bool)) ); 
      }
      if ( mixDevice()->hasMediaPlayControl())
      {
	QToolButton *lbl = addMediaButton("media-playback-start", mediaLayout);
	connect(lbl, SIGNAL(clicked(bool)), this, SLOT(mediaPlay(bool)) ); 
      }
      if ( mixDevice()->hasMediaNextControl())
      {
	QToolButton *lbl = addMediaButton("media-skip-forward", mediaLayout);
	connect(lbl, SIGNAL(clicked(bool)), this, SLOT(mediaNext(bool)) ); 
      }
      volLayout->addLayout(mediaLayout);
    }


QToolButton* MDWSlider::addMediaButton(QString iconName, QLayout* layout)
{
	QToolButton *lbl = new QToolButton(this);
	lbl->setIconSize(QSize(22,22));
	lbl->setAutoRaise(true);
	lbl->setCheckable(false);
	
	setIcon(iconName, lbl);
	layout->addWidget(lbl);

	return lbl;
}

void MDWSlider::mediaPrev(bool)
{
  mixDevice()->mediaPrev();
}

void MDWSlider::mediaNext(bool)
{
  mixDevice()->mediaNext();
}

void MDWSlider::mediaPlay(bool)
{
  mixDevice()->mediaPlay();
}

void MDWSlider::addSliders( QBoxLayout *volLayout, char type, Volume& vol, QList<QAbstractSlider *>& ref_sliders)
{
	const int minSliderSize = fontMetrics().height() * 10;
	long minvol = vol.minVolume();
	long maxvol = vol.maxVolume();

	QMap<Volume::ChannelID, VolumeChannel> vols = vol.getVolumes();

	foreach (VolumeChannel vc, vols )
	{
		//kDebug(67100) << "Add label to " << vc.chid << ": " <<  Volume::ChannelNameReadable[vc.chid];
		QWidget *subcontrolLabel;

		QString subcontrolTranslation;
		if ( type == 'c' ) subcontrolTranslation += i18n("Capture") + ' ';
		subcontrolTranslation += Volume::ChannelNameReadable[vc.chid]; //Volume::getSubcontrolTranslation(chid);
		subcontrolLabel = createLabel(this, subcontrolTranslation, volLayout, true);

		QAbstractSlider* slider;
		if ( m_small )
		{
			slider = new KSmallSlider( minvol, maxvol, (maxvol-minvol+1) / Mixer::VOLUME_PAGESTEP_DIVISOR,
				                           vol.getVolume( vc.chid ), _orientation, this );
		} // small
		else  {
			slider = new VolumeSlider( _orientation, this );
			slider->setMinimum(minvol);
			slider->setMaximum(maxvol);
			slider->setPageStep(maxvol / Mixer::VOLUME_PAGESTEP_DIVISOR);
			slider->setValue(  vol.getVolume( vc.chid ) );
			volumeValues.push_back( vol.getVolume( vc.chid ) );
			
			extraData(slider).setSubcontrolLabel(subcontrolLabel);

			if ( _orientation == Qt::Vertical ) {
				slider->setMinimumHeight( minSliderSize );
			}
			else {
				slider->setMinimumWidth( minSliderSize );
			}
			if ( ! _pctl->getBackgroundColor().isEmpty() ) {
				slider->setStyleSheet("QSlider { background-color: " + _pctl->getBackgroundColor() + " }");
			}
		} // not small

		extraData(slider).setChid(vc.chid);
		slider->installEventFilter( this );
		if ( type == 'p' ) {
			slider->setToolTip( m_mixdevice->readableName() );
		}
		else {
			QString captureTip( i18n( "%1 (capture)", m_mixdevice->readableName() ) );
			slider->setToolTip( captureTip );
		}

		volLayout->addWidget( slider ); // add to layout
		ref_sliders.append ( slider ); // add to list
		//ref_slidersChids.append(vc.chid);
		connect( slider, SIGNAL(valueChanged(int)), SLOT(volumeChange(int)) );
		connect( slider, SIGNAL(sliderPressed()), SLOT(sliderPressed()) );
		connect( slider, SIGNAL(sliderReleased()), SLOT(sliderReleased()) );
		
	} // for all channels of this device
}


VolumeSliderExtraData& MDWSlider::extraData(QAbstractSlider *slider)
{
  VolumeSlider* sl = qobject_cast<VolumeSlider*>(slider);
  if ( sl ) return sl->extraData;
  
  KSmallSlider* sl2 = qobject_cast<KSmallSlider*>(slider);
  if ( sl2 ) return sl2->extraData;
  
  kError(67100) << "Invalid slider";
  return MDWSlider::DummVolumeSliderExtraData;
}


void MDWSlider::sliderPressed()
{
  m_sliderInWork = true;
}


void MDWSlider::sliderReleased()
{
  m_sliderInWork = false;
}


QWidget* MDWSlider::createLabel(QWidget* parent, QString& label, QBoxLayout *layout, bool small)
{
  QFont qf;
  qf.setPointSize(8);

  QWidget* labelWidget;
	if (_orientation == Qt::Horizontal)
	{
		labelWidget = new QLabel(label, parent);
		if ( small ) ((QLabel*)labelWidget)->setFont(qf);
	}
	else {
		labelWidget = new VerticalText(parent, label);
		if ( small ) ((VerticalText*)labelWidget)->setFont(qf);
	}
	
	labelWidget->installEventFilter( parent );
	layout->addWidget(labelWidget);

	return labelWidget;
}


QPixmap MDWSlider::loadIcon( QString filename )
{
	return KIconLoader::global()->loadIcon( filename, KIconLoader::Small, KIconLoader::SizeSmallMedium );
}

void MDWSlider::setIcon( QString filename )
{
  setIcon(filename, &m_iconLabelSimple);
}

void MDWSlider::setIcon( QString filename, QLabel** label )
{
	if( (*label) == 0 )
	{
		*label = new QLabel(this);
		installEventFilter( *label );
	}
	setIcon(filename, *label);
}

void MDWSlider::setIcon( QString filename, QWidget* label )
{
	QPixmap miniDevPM = loadIcon( filename );
	if ( !miniDevPM.isNull() )
	{
		if ( m_small )
		{
			// scale icon
			QMatrix t;
			t = t.scale( 10.0/miniDevPM.width(), 10.0/miniDevPM.height() );
			miniDevPM = miniDevPM.transformed( t );
			label->resize( 10, 10 );
		} // small size
		label->setMinimumSize(22,22);
		
		QLabel* lbl = qobject_cast<QLabel*>(label);
		if ( lbl != 0 )
		{
		  lbl->setPixmap( miniDevPM );
		  lbl->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
		} // QLabel
		else
		{
		  QToolButton* tbt = qobject_cast<QToolButton*>(label);
		  if ( tbt != 0 )
		  {
/*		  QIcon ic(miniDevPM);
		  lbl->setIcon(ic);*/
		    tbt->setIcon( miniDevPM );
//		    tbt->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
		  } // QToolButton 
		  else {
		    kError() << "Invalid widget type ... cannot set icon";
		  }
		}
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

	int overallSlidersToShow = 0;
	if ( ! m_slidersPlayback.isEmpty() ) overallSlidersToShow += ( m_linked ? 1 : m_slidersPlayback.count() );
	if ( ! m_slidersCapture.isEmpty()  ) overallSlidersToShow += ( m_linked ? 1 : m_slidersCapture.count() );

	bool showSubcontrolLabels = (overallSlidersToShow >= 2);
	setStereoLinkedInternal(m_slidersPlayback, showSubcontrolLabels);
	setStereoLinkedInternal(m_slidersCapture  , showSubcontrolLabels);
	update(); // Call update(), so that the sliders can adjust EITHER to the individual values OR the average value.
}

void
MDWSlider::setStereoLinkedInternal(QList<QAbstractSlider *>& ref_sliders, bool showSubcontrolLabels)
{
	if ( ref_sliders.isEmpty())
	  return;

	bool first = true;
	foreach ( QAbstractSlider* slider1, ref_sliders )
	{
		slider1->setVisible(!m_linked || first); // One slider (the 1st) is always shown
		extraData(slider1).getSubcontrolLabel()->setVisible(!m_linked && showSubcontrolLabels); // (*)
		first = false;
		/* (*) cesken: I have excluded the "|| first" check because the text would not be nice:
		 *     It would be "Left" or "Capture Left", while it should be "Playback" and "Capture" in the "linked" case.
		 *
		 *     But the only affected situation is when we have playback AND capture on the same control, where we show no label.
		 *     It would be nice to put at least a "Capture" label on the capture subcontrol instead.
		 *     To achieve this we would need to exchange the Text on the first capture subcontrol dynamically. This can
		 *     be done, but I'll leave this open for now.
		 */
	}



	// Redo the tickmarks to last slider in the slider list.
	// The implementation is not obvious, so lets explain:
	// We ALWAYS have tickmarks on the LAST slider. Sometimes the slider is not shown, and then we just don't bother.
	// a) So, if the last slider has tickmarks, we can always call setTicks( true ).
	// b) if the last slider has NO tickmarks, there ae no tickmarks at all, and we don't need to redo the tickmarks.
	QSlider* slider = qobject_cast<QSlider*>( ref_sliders.last() );
	if( slider && slider->tickPosition() != QSlider::NoTicks) 
		setTicks( true );

}


void
MDWSlider::setLabeled(bool value)
{
	if ( m_label != 0) m_label->setVisible(value);
	if ( m_muteText != 0) m_muteText->setVisible(value);
	if ( m_captureText != 0) m_captureText->setVisible(value);
	layout()->activate();
}

void
MDWSlider::setTicks( bool value )
{
	if (m_slidersPlayback.count() != 0) setTicksInternal(m_slidersPlayback, value);
	if (m_slidersCapture.count() != 0) setTicksInternal(m_slidersCapture, value);
}

/**
 * Enables or disables tickmarks
 * Please note that always only the first and last slider has tickmarks.
 * 
 */
void MDWSlider::setTicksInternal(QList<QAbstractSlider *>& ref_sliders, bool ticks)
{
	VolumeSlider* slider = qobject_cast<VolumeSlider*>( ref_sliders[0]);
	if (slider == 0 ) return; // Ticks are only in VolumeSlider, but not in KSmallslider
	
	if( ticks )
	{
		if( isStereoLinked() )
			slider->setTickPosition( QSlider::TicksRight );
		else
		{
			slider->setTickPosition( QSlider::NoTicks );
			slider = qobject_cast<VolumeSlider*>(ref_sliders.last());
			slider->setTickPosition( QSlider::TicksLeft );
		}
	}
	else
	{
		slider->setTickPosition( QSlider::NoTicks );
		slider = qobject_cast<VolumeSlider*>(ref_sliders.last());
		slider->setTickPosition( QSlider::NoTicks );
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
//  if ( mixDevice()->id() == "Headphone:0" )
//  {
//    kDebug(67100) << "headphone bug";
//  }
	if (!m_slidersPlayback.isEmpty())
	{
		m_waitForSoundSetComplete ++;
		volumeValues.push_back(m_slidersPlayback.first()->value());
		volumeChangeInternal(m_mixdevice->playbackVolume(), m_slidersPlayback);
	}
	if (!m_slidersCapture.isEmpty())
	{
		volumeChangeInternal(m_mixdevice->captureVolume(), m_slidersCapture);
	}

	bool oldViewBlockSignalState = m_view->blockSignals(true);
	m_mixdevice->mixer()->commitVolumeChange(m_mixdevice);
	m_view->blockSignals(oldViewBlockSignalState);
}

void MDWSlider::volumeChangeInternal( Volume& vol, QList<QAbstractSlider *>& ref_sliders  )
{
	if ( isStereoLinked() )
	{
		QAbstractSlider* firstSlider = ref_sliders.first();
		m_mixdevice->setMuted(false);
		vol.setAllVolumes(firstSlider->value());
	}
	else
	{
		for( int i=0; i<ref_sliders.count(); i++ )
		{
		        if ( m_mixdevice->isMuted())
		        {   // changing from muted state: unmute (the "if" above is actually superfluous)
				m_mixdevice->setMuted(false);
			}
			QAbstractSlider *sliderWidget = ref_sliders[i];
			vol.setVolume( extraData(sliderWidget).getChid() ,sliderWidget->value());
		} // iterate over all sliders
	}
}


/**
   This slot is called, when a user has clicked the recsrc button. Also it is called by any other
    associated KAction like the context menu.
 */
void MDWSlider::toggleRecsrc()
{
	setRecsrc( m_mixdevice->isRecSource() );
}

void MDWSlider::setRecsrc(bool value )
{
	if ( m_mixdevice->captureVolume().hasSwitch() )
	{
		m_mixdevice->setRecSource( value );
		m_mixdevice->mixer()->commitVolumeChange( m_mixdevice );
	}
}


/**
   This slot is called, when a user has clicked the mute button. Also it is called by any other
    associated KAction like the context menu.
 */
void MDWSlider::toggleMuted()
{
	setMuted( !m_mixdevice->isMuted() );
}

void MDWSlider::setMuted(bool value)
{
	if ( m_mixdevice->hasMuteSwitch() )
	{
		m_mixdevice->setMuted( value );
		m_mixdevice->mixer()->commitVolumeChange(m_mixdevice);
	}
}


void MDWSlider::setDisabled()
{
	// We can only disable, not enable. Because if the slider is not shown the user can not
	// right-click it to show it. Once explained, it is obvious. :-)
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
  increaseOrDecreaseVolume(false);
}

/**
 * TOOD This should go to the Volume class, so we can use it anywhere,
 * like mouse wheel, OSD, Keyboard Shortcuts
 */
long MDWSlider::calculateStepIncrement ( Volume&vol, bool decrease )
{
  	long inc = vol.volumeSpan() / Mixer::VOLUME_STEP_DIVISOR;
	if ( inc == 0 )	inc = 1;
	if ( decrease ) inc *= -1;
	return inc;
}

void MDWSlider::increaseOrDecreaseVolume(bool decrease)
{
	Volume& volP = m_mixdevice->playbackVolume();
	long inc = calculateStepIncrement(volP, decrease);

	if ( mixDevice()->id() == "PCM:0" )
	  kDebug() << ( decrease ? "decrease by " : "increase by " ) << inc ;

	if (!decrease && m_mixdevice->isMuted())
	{
		// increasing from muted state: unmute and start with a low volume level
		if (mixDevice()->id() == "PCM:0")
			kDebug() << "set all to " << inc << "muted old=" << m_mixdevice->isMuted();

		m_mixdevice->setMuted(false);
		volP.setAllVolumes(inc);
	}
	else
	{
		volP.changeAllVolumes(inc);
		if (mixDevice()->id() == "PCM:0")
			kDebug()
			<< (decrease ? "decrease by " : "increase by ") << inc;
	}

	Volume& volC = m_mixdevice->captureVolume();
	inc = calculateStepIncrement(volC, decrease);
	volC.changeAllVolumes(inc);

	// I should possibly not block, as the changes that come back from the Soundcard
	//      will be ignored (e.g. because of capture groups)
// 	kDebug() << "MDWSlider is blocking signals for " << m_view->id();
// 	bool oldViewBlockSignalState = m_view->blockSignals(true);
	m_mixdevice->mixer()->commitVolumeChange(m_mixdevice);
// 	kDebug() << "MDWSlider is unblocking signals for " << m_view->id();
// 	m_view->blockSignals(oldViewBlockSignalState);
}

/**
   This slot is called on a MouseWheel event. Also it is called by any other
    associated KAction like the context menu.
 */
void MDWSlider::decreaseVolume()
{
  increaseOrDecreaseVolume(true);
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
 * This is called whenever there are volume updates pending from the hardware for this MDW.
 */
void MDWSlider::update()
{
	  bool debugMe = (mixDevice()->id() == "PCM:0" );
	  if (debugMe) kDebug() << "The update() PCM:0 playback state" << mixDevice()->isMuted()
	    << ", vol=" << mixDevice()->playbackVolume().getAvgVolumePercent(Volume::MALL);

	if ( m_slidersPlayback.count() != 0 || m_mixdevice->hasMuteSwitch() )
		updateInternal(m_mixdevice->playbackVolume(), m_slidersPlayback, m_mixdevice->isMuted() );
	if ( m_slidersCapture.count()  != 0 || m_mixdevice->captureVolume().hasSwitch() )
		updateInternal(m_mixdevice->captureVolume(), m_slidersCapture, m_mixdevice->isNotRecSource() );
	if (m_label) {
		QLabel *l;
		VerticalText *v;
		if ((l = dynamic_cast<QLabel*>(m_label)))
			l->setText(m_mixdevice->readableName());
		else if ((v = dynamic_cast<VerticalText*>(m_label)))
			v->setText(m_mixdevice->readableName());
	}
	updateAccesability();
}

// TODO passing "muted" should not be necessary any longer - due to getVolumeForGUI()
void MDWSlider::updateInternal(Volume& vol, QList<QAbstractSlider *>& ref_sliders, bool muted)
{
//  	  bool debugMe = (mixDevice()->id() == "PCM:0" );
//	  if (debugMe)
//	  {
//	    kDebug() << "The updateInternal() PCM:0 playback state" << mixDevice()->isMuted()
//	    << ", vol=" << mixDevice()->playbackVolume().getAvgVolumePercent(Volume::MALL);
//	  }
  
	for( int i=0; i<ref_sliders.count(); i++ )
	{
		QAbstractSlider *slider = ref_sliders.at( i );
		Volume::ChannelID chid = extraData(slider).getChid();
		long useVolume = muted ? 0 : vol.getVolumeForGUI(chid);
		int volume_index;

		bool oldBlockState = slider->blockSignals( true );

//		slider->setValue( useVolume );
		// --- Avoid feedback loops START -----------------
		if((volume_index = volumeValues.indexOf(useVolume)) > -1 && --m_waitForSoundSetComplete < 1)
		{
		    m_waitForSoundSetComplete = 0;
		    volumeValues.removeAt(volume_index);

		    if(!m_sliderInWork)
			  slider->setValue(useVolume);
		}
		else if(!m_sliderInWork && m_waitForSoundSetComplete < 1)
		{
			slider->setValue(useVolume);
		}
		// --- Avoid feedback loops END -----------------

		if ( slider->inherits( "KSmallSlider" ) )
		{
			((KSmallSlider*)slider)->setGray( m_mixdevice->isMuted() );
		}
		slider->blockSignals( oldBlockState );
	} // for all sliders


	// update mute

	if( m_qcb != 0 )
	{
		bool oldBlockState = m_qcb->blockSignals( true );
		if (m_mixdevice->isMuted())
			m_qcb->setIcon( QIcon( loadIcon("audio-volume-muted") ) );
		else
			m_qcb->setIcon( QIcon( loadIcon("audio-volume-high") ) );
		m_qcb->blockSignals( oldBlockState );
	}

	if( m_captureCheckbox )
	{
		bool oldBlockState = m_captureCheckbox->blockSignals( true );
		m_captureCheckbox->setChecked( m_mixdevice->isRecSource() );
		m_captureCheckbox->blockSignals( oldBlockState );
	}

}

#ifndef QT_NO_ACCESSIBILITY
void MDWSlider::updateAccesability()
{
        if (m_linked) {
                if (!m_slidersPlayback.isEmpty())
                        m_slidersPlayback[0]->setAccessibleName(m_slidersPlayback[0]->toolTip());
                if (!m_slidersCapture.isEmpty())
                        m_slidersCapture[0]->setAccessibleName(m_slidersCapture[0]->toolTip());
        } else {
                QList<VolumeChannel> vols = m_mixdevice->playbackVolume().getVolumes().values();
                foreach (QAbstractSlider *slider, m_slidersPlayback) {
                        slider->setAccessibleName(slider->toolTip()+ " (" +Volume::ChannelNameReadable[vols.first().chid]+")");
                        vols.pop_front();
                }
                vols = m_mixdevice->captureVolume().getVolumes().values();
                foreach (QAbstractSlider *slider, m_slidersCapture) {
                        slider->setAccessibleName(slider->toolTip()+ " (" +Volume::ChannelNameReadable[vols.first().chid]+")");
                        vols.pop_front();
                }
        }
}
#endif


void MDWSlider::showContextMenu( const QPoint& pos )
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

	if ( m_mixdevice->hasMuteSwitch() ) {
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
    connect(a, SIGNAL(triggered(bool)), SLOT(moveStreamAutomatic()), Qt::QueuedConnection);
    m_moveMenu->addAction( a );

    a = new KAction(_mdwMoveActions);
    a->setSeparator(true);
    _mdwMoveActions->addAction( QString("-"), a);

    m_moveMenu->addAction( a );
    foreach (shared_ptr<MixDevice> md, *ms)
    {
        a = new MDWMoveAction(md, _mdwMoveActions);
        _mdwMoveActions->addAction( QString("moveto") + md->id(), a);
        connect(a, SIGNAL(moveRequest(QString)), SLOT(moveStream(QString)), Qt::QueuedConnection);
        m_moveMenu->addAction( a );
    }
}

/**
 * An event filter for the various QWidgets. We watch for Mouse press Events, so
 * that we can popup the context menu.
 */
bool MDWSlider::eventFilter( QObject* obj, QEvent* e )
{
	QEvent::Type eventType = e->type();
	if (eventType == QEvent::MouseButtonPress) {
		QMouseEvent *qme = static_cast<QMouseEvent*>(e);
		if (qme->button() == Qt::RightButton) {
			showContextMenu();
			return true;
		}
	} else if (eventType == QEvent::ContextMenu) {
		QPoint pos = reinterpret_cast<QWidget *>(obj)->mapToGlobal(QPoint(0, 0));
		showContextMenu(pos);
		return true;
	}
	// Attention: We don't filter WheelEvents for KSmallSlider, because it handles WheelEvents itself
	else if ( eventType == QEvent::Wheel )
//	         && strcmp(obj->metaObject()->className(),"KSmallSlider") != 0 )  {  // Remove the KSmallSlider check. If KSmallSlider comes back, use a cheaper type check - e.g. a boolean value.
	{
		QWheelEvent *qwe = static_cast<QWheelEvent*>(e);

		bool increase = (qwe->delta() > 0);
		if (qwe->orientation() == Qt::Horizontal) // Reverse horizontal scroll: bko228780 
			increase = !increase;

		if (increase) {
			increaseVolume();
		}
		else {
			decreaseVolume();
		}
		
		Volume& volP = m_mixdevice->playbackVolume();
		volumeValues.push_back(volP.getVolume(extraData((QAbstractSlider*)obj).getChid()));
		return true;
	}
	return QWidget::eventFilter(obj,e);
}

#include "mdwslider.moc"
