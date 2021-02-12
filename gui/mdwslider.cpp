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

#include <klocalizedstring.h>
#include <kconfig.h>
#include <kglobalaccel.h>
#include <kactioncollection.h>

#include <QCoreApplication>
#include <QToolButton>
#include <QMenu>
#include <QLabel>
#include <QBoxLayout>
#include <QGridLayout>
#include <QAction>

#include "core/ControlManager.h"
#include "core/mixer.h"
#include "gui/guiprofile.h"
#include "gui/volumeslider.h"
#include "gui/viewbase.h"
#include "gui/verticaltext.h"
#include "gui/toggletoolbutton.h"


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
MDWSlider::MDWSlider(shared_ptr<MixDevice> md, MixDeviceWidget::MDWFlags flags, ViewBase *view, ProfControl *pctl)
	: MixDeviceWidget(md, flags, view, pctl),
	  m_linked(true),
	  m_controlGrid(nullptr),
	  m_controlIcon(nullptr),
	  m_controlLabel(nullptr),
	  m_muteButton(nullptr),
	  m_captureButton(nullptr),
	  m_mediaPlayButton(nullptr),
	  m_controlButtonSize(QSize()),
	  m_moveMenu(nullptr),
	  m_sliderInWork(false),
	  m_waitForSoundSetComplete(0)
{
	//qCDebug(KMIX_LOG) << "for" << mixDevice()->readableName() << "flags" << MixDeviceWidget::flags();

    createActions();
    createWidgets();
    createGlobalActions();

    // Yes, this looks odd - monitor all events sent to myself by myself?
    // But it's so that wheel events over the MDWSlider background can be
    // handled by eventFilter() in the same way as wheel events over child
    // widgets.  Each child widget apart from the sliders themselves also
    // also needs to have the event filter installed on it, because QWidget
    // by default ignores the wheel event and does not propagate it.
    installEventFilter(this);

    update();
}

MDWSlider::~MDWSlider()
{
	qDeleteAll(m_slidersPlayback);
	qDeleteAll(m_slidersCapture);
}

void MDWSlider::createActions()
{
    // create actions (in channelActions(), see MixDeviceWidget)
	KToggleAction *taction = channelActions()->add<KToggleAction>( "stereo" );
    taction->setText( i18n("Split Channels") );
    connect( taction, SIGNAL(triggered(bool)), SLOT(toggleStereoLinked()) );

    if( mixDevice()->hasMuteSwitch() )
    {
        taction = channelActions()->add<KToggleAction>( "mute" );
        taction->setText( i18n("Mute") );
        connect( taction, SIGNAL(toggled(bool)), SLOT(toggleMuted()) );
    }

    if( mixDevice()->captureVolume().hasSwitch() ) {
        taction = channelActions()->add<KToggleAction>( "recsrc" );
        taction->setText( i18n("Capture") );
        connect( taction, SIGNAL(toggled(bool)), SLOT(toggleRecsrc()) );
    }

    if( mixDevice()->isMovable() ) {
        m_moveMenu = new QMenu( i18n("Use Device"), this);
        connect( m_moveMenu, SIGNAL(aboutToShow()), SLOT(showMoveMenu()) );
    }
}


void MDWSlider::createGlobalActions()
{
	const shared_ptr<MixDevice> md = mixDevice();
	const Mixer *mixer = md->mixer();

	// I don't understand the former logic here.  This variable and the comment
	// in MDWSlider::addGlobalShortcut() say that there are no shortcuts
	// assigned for a virtual or dynamic control - which makes sense.  However,
	// for such a control an action was created but its triggered() signal was
	// never connected to anything - so even if the action was accessible from
	// the GUI it would have done nothing.
	//
	// Possibly what was intended was that the shortcut should be local to KMix.
	// However, that doesn't make sense either - in MixDeviceWidget::defineKeys()
	// (which is now MixDeviceWidget::configureShortcuts()), only the global
	// shortcuts are allowed to be edited.
	//
	// In order to keep things simple, do not assign any shortcuts for a
	// virtual/dynamic control.
	if (mixer->isDynamic()) return;

	// The following actions are created for the desktop "Global Shortcuts" settings.
	// It would appear that the discussion and caution below was only required
	// for KDE4 and has been fixed in KF5.
	//
	// Note that global shortcuts are saved with the name as set with QAction::setText(),
	// instead of their internal action name which is the QObject::objectName().
        // This is a bug according to the kde-core-devel thread "Global shortcuts are saved with
	// their text-name and not..." at https://marc.info/?t=119901891400001&r=1&w=2
	//
	// Work around this by setting a text that is unique, but still readable for the user.

	// A suffix string to identify the control in the global shortcut configuration.
	// This should not be I18N'ed so that the assignment is independent of the
	// display language.
	const QString actionSuffix  = QString("%1@%2").arg(md->readableName(), mixer->readableName());

	// -3- MUTE VOLUME SHORTCUT -----------------------------------------
	QAction *act = new QAction(i18nc("1=device name, 2=control name",
					 "Mute %2 on %1",
					 mixer->readableName(), md->readableName()), this);
	globalActions()->addAction(QString("toggle-mute-")+actionSuffix, act);
	KGlobalAccel::setGlobalShortcut(act, QKeySequence());
	connect(act, &QAction::triggered, this, &MDWSlider::toggleMuted);

	// Only create volume actions if there are either any playback or capture
	// sliders present.  It is assumed that there cannot be both.
	if (m_slidersPlayback.count()!=0 || m_slidersCapture.count()!=0)
	{
		// -1- INCREASE VOLUME SHORTCUT -----------------------------------------
		act = new QAction(i18nc("1=device name, 2=control name",
					"Increase volume of %2 on %1",
					mixer->readableName(), md->readableName()), this);
		globalActions()->addAction(QString("increase-volume-")+actionSuffix, act);
		KGlobalAccel::setGlobalShortcut(act, QKeySequence());
		connect(act, &QAction::triggered, this, &MDWSlider::increaseVolume);

		// -2- DECREASE VOLUME SHORTCUT -----------------------------------------
		act = new QAction(i18nc("1=device name, 2=control name",
					"Decrease volume of %2 on %1",
					mixer->readableName(), md->readableName()), this);
		globalActions()->addAction(QString("decrease-volume-")+actionSuffix, act);
		KGlobalAccel::setGlobalShortcut(act, QKeySequence());
		connect(act, &QAction::triggered, this, &MDWSlider::decreaseVolume);
	}
}


QSizePolicy MDWSlider::sizePolicy() const
{
	if (orientation()==Qt::Vertical)
	{
		return QSizePolicy(  QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
	}
	else
	{
		return QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
	}
}

QSize MDWSlider::sizeHint() const
{
	return QSize( 90, QWidget::sizeHint().height());
}


/**
 * This method is a helper for users of this class who would like
 * to show multiple MDWSlider, and align the sliders.
 * It returns the "height" (if vertical) of this slider's label.
 * Warning: Line wraps are computed for a fixed size (100), this may be inaccurate in case,
 * the widgets have different sizes.
 */
int MDWSlider::labelExtentHint() const
{
	if (m_controlLabel==nullptr) return (0);
	if (orientation()==Qt::Vertical) return (m_controlLabel->heightForWidth(m_controlLabel->width()));
	else return (m_controlLabel->sizeHint().width());
}


/**
 * If a label from another widget has more lines than this widget, then a spacer is added under the label
 */
void MDWSlider::setLabelExtent(int extent)
{
	if (m_controlGrid==nullptr) return;

	if (orientation()==Qt::Vertical) m_controlGrid->setRowMinimumHeight(1, extent);
	else m_controlGrid->setColumnMinimumWidth(1, extent);
}


void MDWSlider::guiAddCaptureButton(const QString &captureTooltipText)
{
	m_captureButton = new ToggleToolButton("media-record", this);
	m_captureButton->installEventFilter(this);
	connect(m_captureButton, SIGNAL(clicked(bool)), this, SLOT(toggleRecsrc()));
	m_captureButton->setToolTip(captureTooltipText);
}

void MDWSlider::guiAddMuteButton(const QString &muteTooltipText)
{
	m_muteButton = new ToggleToolButton("audio-volume-high", this);
	m_muteButton->setInactiveIcon("audio-volume-muted");
	m_muteButton->installEventFilter(this);
	connect(m_muteButton, SIGNAL(clicked(bool)), this, SLOT(toggleMuted()));
	m_muteButton->setToolTip(muteTooltipText);
}

void MDWSlider::guiAddControlLabel(Qt::Alignment alignment, const QString &channelName)
{
	m_controlLabel = new QLabel(channelName, this);
	m_controlLabel->setWordWrap(true);
	m_controlLabel->setAlignment(alignment);
	m_controlLabel->installEventFilter(this);
}

void MDWSlider::guiAddControlIcon(const QString &tooltipText)
{
	m_controlIcon = new QLabel(this);
	ToggleToolButton::setIndicatorIcon(mixDevice()->iconName(), m_controlIcon);
	m_controlIcon->setToolTip(tooltipText);
	m_controlIcon->installEventFilter(this);

	// A MPRIS2 application's icon name is obtained from its desktop file.
	// Finding the desktop file is an asynchronous DBus operation which
	// could in theory happen after the MDWSlider has been created (which
	// would have used the fallback icon name).  So update the indicator
	// icon on a signal.
	connect(mixDevice().get(), &MixDevice::iconNameChanged,
		this, [this](const QString &newName) {
			      qCDebug(KMIX_LOG) << "for" << mixDevice()->readableName() << "new icon" << newName;
			      ToggleToolButton::setIndicatorIcon(newName, m_controlIcon);
		      });
}

QWidget *MDWSlider::guiAddButtonSpacer()
{
	if (hasMuteButton() || hasCaptureLED()) return (nullptr);
							// spacer not needed
	QWidget *buttonSpacer = new QWidget(this);
	if (orientation()==Qt::Vertical)		// vertical sliders
	{
		buttonSpacer->setMinimumHeight(controlButtonSize().height());
		buttonSpacer->setMaximumWidth(1);
	}
	else						// horizontal sliders
	{
		buttonSpacer->setMinimumWidth(controlButtonSize().width());
		buttonSpacer->setMaximumHeight(1);
	}

	buttonSpacer->installEventFilter(this);
	return (buttonSpacer);
}

QSize MDWSlider::controlButtonSize()
{
	if (!m_controlButtonSize.isValid())		// not calculated yet
	{
		auto *buttonSpacer = new QToolButton();
		ToggleToolButton::setIndicatorIcon("unknown", buttonSpacer);
		m_controlButtonSize = buttonSpacer->sizeHint();
		qCDebug(KMIX_LOG) << m_controlButtonSize;
		delete buttonSpacer;
	}

	return (m_controlButtonSize);
}


/**
 * Creates all widgets : Icon, Label, Mute-Button, Slider(s) and Capture-Button.
 */
void MDWSlider::createWidgets()
{
	const bool includePlayback = profileControl()->useSubcontrolPlayback();
	const bool includeCapture = profileControl()->useSubcontrolCapture();
	const bool wantsPlaybackSliders = includePlayback && (mixDevice()->playbackVolume().count()>0);
	const bool wantsCaptureSliders  = includeCapture && (mixDevice()->captureVolume().count()>0);
	const bool wantsCaptureLED = includeCapture && (flags() & MixDeviceWidget::ShowCapture);
	const bool wantsMuteButton = includePlayback && (flags() & MixDeviceWidget::ShowMute);
	
	const MediaController *mediaController = mixDevice()->mediaController();
	const bool wantsMediaControls = mediaController->hasControls();

	const QString channelName = mixDevice()->readableName();
	QString tooltipText = channelName;
	QString captureTooltipText = i18nc("%1=channel", "Capture/Uncapture %1", channelName);
	QString muteTooltipText = i18nc("%1=channel", "Mute/Unmute %1", channelName);
	if (flags() & MixDeviceWidget::ShowMixerName)
	{
		const QString mixerName = mixDevice()->mixer()->readableName();
		tooltipText = i18nc("%1=device %2=channel", "%1\n%2", mixerName, tooltipText);
		captureTooltipText = i18nc("%1=device %2=channel", "%1\n%2", mixerName, captureTooltipText);
		muteTooltipText = i18nc("%1=device %2=channel", "%1\n%2", mixerName, muteTooltipText);
	}

	m_controlGrid = new QGridLayout(this);
	setLayout(m_controlGrid);
	QBoxLayout *volLayout;

	if (orientation()==Qt::Vertical)		// vertical sliders
	{
		m_controlGrid->setContentsMargins(2, 0, 2, 0);
		const Qt::Alignment sliderAlign = Qt::AlignHCenter|Qt::AlignBottom;

		// Row 0: Control type icon
		guiAddControlIcon(tooltipText);
		m_controlGrid->addWidget(m_controlIcon, 0, 0, 1, -1, Qt::AlignHCenter|Qt::AlignTop);

		// Row 1: Device name label
		guiAddControlLabel(Qt::AlignHCenter, channelName);
		m_controlGrid->addWidget(m_controlLabel, 1, 0, 1, -1, Qt::AlignHCenter|Qt::AlignTop);

		// Row 2: Sliders

		int col = 0;				// current column being filled
		int playbackCol = 0;			// where these sliders ended up
		int captureCol = 1;			// or default button column if none

		if (wantsPlaybackSliders)
		{
			volLayout = new QHBoxLayout();
			volLayout->setAlignment(sliderAlign);
			addSliders(volLayout, 'p', mixDevice()->playbackVolume(), m_slidersPlayback, tooltipText);
			m_controlGrid->addLayout(volLayout, 2, col);
			playbackCol = col;
			++col;
		}

		if (wantsCaptureSliders)
		{
			volLayout = new QHBoxLayout();
			volLayout->setAlignment(sliderAlign);
			addSliders(volLayout, 'c', mixDevice()->captureVolume(), m_slidersCapture, tooltipText);
			m_controlGrid->addLayout(volLayout, 2, col);
			captureCol = col;
			++col;
		}

		if (wantsMediaControls)
		{
			volLayout = new QHBoxLayout();
			volLayout->setAlignment(sliderAlign);
			addMediaControls(volLayout);
			m_controlGrid->addLayout(volLayout, 2, col);
		}

		m_controlGrid->setRowStretch(2, 1);	// sliders need the most space

		// Row 3: Control buttons
		if (wantsMuteButton && mixDevice()->hasMuteSwitch())
		{
			guiAddMuteButton(muteTooltipText);
			m_controlGrid->addWidget(m_muteButton, 3, playbackCol, Qt::AlignHCenter|Qt::AlignTop);
		}

		if (wantsCaptureLED && mixDevice()->captureVolume().hasSwitch())
		{
			guiAddCaptureButton(captureTooltipText);
			m_controlGrid->addWidget(m_captureButton, 3, captureCol, Qt::AlignHCenter|Qt::AlignTop);
		}

		// If nether a mute nor a capture button is present, then put a
		// dummy spacer button (in column 0, where the mute button would
		// normally go).  This is to maintain the size of the slider
		// relative to others that do have one or both buttons.
		//
		// We have to do this, rather than setting a minimum height for row 3,
		// as in the case where it is needed row 3 will be empty and QGridLayout
		// ignores the minimum height set on it.
		QWidget *buttonSpacer = guiAddButtonSpacer();
		if (buttonSpacer!=nullptr) m_controlGrid->addWidget(buttonSpacer, 3, 0);
	}
	else						// horizontal sliders
	{
		const Qt::Alignment sliderAlign = Qt::AlignHCenter|Qt::AlignVCenter;

		// Column 0: Control type icon
		guiAddControlIcon(tooltipText);
		m_controlGrid->addWidget(m_controlIcon, 0, 0, -1, 1, Qt::AlignLeft|Qt::AlignVCenter);

		// Column 1: Device name label
		guiAddControlLabel(Qt::AlignLeft, channelName);
		m_controlGrid->addWidget(m_controlLabel, 0, 1, -1, 1, Qt::AlignLeft|Qt::AlignVCenter);

		// Column 2: Sliders

		int row = 0;				// current row being filled
		int playbackRow = 0;			// where these sliders ended up
		int captureRow = 1;			// or default button row if none

		if (wantsPlaybackSliders)
		{
			volLayout = new QVBoxLayout();
			volLayout->setAlignment(sliderAlign);
			addSliders(volLayout, 'p', mixDevice()->playbackVolume(), m_slidersPlayback, tooltipText);
			m_controlGrid->addLayout(volLayout, row, 2);
			playbackRow = row;
			++row;
		}

		if (wantsCaptureSliders)
		{
			volLayout = new QVBoxLayout();
			volLayout->setAlignment(sliderAlign);
			addSliders(volLayout, 'c', mixDevice()->captureVolume(), m_slidersCapture, tooltipText);
			m_controlGrid->addLayout(volLayout, row, 2);
			captureRow = row;
			++row;
		}

		if (wantsMediaControls)
		{
			volLayout = new QVBoxLayout();
			volLayout->setAlignment(sliderAlign);
			addMediaControls(volLayout);
			m_controlGrid->addLayout(volLayout, row, 2);
		}

		m_controlGrid->setColumnStretch(2, 1);	// sliders need the most space

		// Column 3: Control buttons
		if (wantsMuteButton && mixDevice()->hasMuteSwitch())
		{
			guiAddMuteButton(muteTooltipText);
			m_controlGrid->addWidget(m_muteButton, playbackRow, 3, Qt::AlignRight|Qt::AlignVCenter);
		}

		if (wantsCaptureLED && mixDevice()->captureVolume().hasSwitch())
		{
			guiAddCaptureButton(captureTooltipText);
			m_controlGrid->addWidget(m_captureButton, captureRow, 3, Qt::AlignRight|Qt::AlignVCenter);
		}

		// Dummy spacer button
		QWidget *buttonSpacer = guiAddButtonSpacer();
		if (buttonSpacer!=nullptr) m_controlGrid->addWidget(buttonSpacer, 0, 3);
	}

	const bool stereoLinked = !profileControl()->isSplit();
	setStereoLinked( stereoLinked );
	
	// Activate it explicitly in KDE3 because of PanelApplet/Kicker issues.
	// Not sure whether this is necessary 2 generations later.
	layout()->activate();
}

QString MDWSlider::calculatePlaybackIcon(MediaController::PlayState playState)
{
	QString mediaIconName;
	switch (playState)
	{
	case MediaController::PlayPlaying:
		// playing => show pause icon
		mediaIconName = "media-playback-pause";
		break;
	case MediaController::PlayPaused:
		// stopped/paused => show play icon
		mediaIconName = "media-playback-start";
		break;
	case MediaController::PlayStopped:
		// stopped/paused => show play icon
		mediaIconName = "media-playback-start";
		break;
	default:
		// unknown => not good, probably result from player has not yet arrived => show a play button
		mediaIconName = "media-playback-start";
		break;
	}

	return mediaIconName;
}

void MDWSlider::addMediaControls(QBoxLayout* volLayout)
{
	MediaController *mediaController =  mixDevice()->mediaController();

	QBoxLayout *mediaLayout;
	if (orientation()==Qt::Vertical) mediaLayout = new QVBoxLayout();
	else mediaLayout = new QHBoxLayout();

//	QFrame* frame1 = new QFrame(this);
//	frame1->setFrameShape(QFrame::StyledPanel);
	QWidget* frame = this; // or frame1
	mediaLayout->addStretch();
	if (mediaController->hasMediaPrevControl())
	{
		QToolButton *lbl = addMediaButton("media-skip-backward", mediaLayout, frame);
		connect(lbl, SIGNAL(clicked(bool)), this, SLOT(mediaPrev(bool)));
	}
	if (mediaController->hasMediaPlayControl())
	{
		MediaController::PlayState playState = mediaController->getPlayState();
		QString mediaIcon = calculatePlaybackIcon(playState);
		m_mediaPlayButton = addMediaButton(mediaIcon, mediaLayout, frame);
		connect(m_mediaPlayButton, SIGNAL(clicked(bool)), this, SLOT(mediaPlay(bool)));
	}

	if (mediaController->hasMediaNextControl())
	{
		QToolButton *lbl = addMediaButton("media-skip-forward", mediaLayout, frame);
		connect(lbl, SIGNAL(clicked(bool)), this, SLOT(mediaNext(bool)));
	}
	mediaLayout->addStretch();
	volLayout->addLayout(mediaLayout);
}


QToolButton* MDWSlider::addMediaButton(QString iconName, QLayout* layout, QWidget *parent)
{
	QToolButton *lbl = new QToolButton(parent);
	lbl->setAutoRaise(true);
	lbl->setCheckable(false);
	
	ToggleToolButton::setIndicatorIcon(iconName, lbl);
	layout->addWidget(lbl);

	return lbl;
}

/**
 * Updates the icon according to the data model.
 */
void MDWSlider::updateMediaButton()
{
	if (m_mediaPlayButton == 0)
		return; // has no media button

	MediaController *mediaController =  mixDevice()->mediaController();
	QString mediaIconName = calculatePlaybackIcon(mediaController->getPlayState());
	ToggleToolButton::setIndicatorIcon(mediaIconName, m_mediaPlayButton);
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


static QWidget *createLabel(QWidget *parent, const QString &label, Qt::Orientation orient, bool small)
{
	QFont qf;
	qf.setPointSize(8);

	QWidget *labelWidget;
	if (orient == Qt::Horizontal)
	{
		auto *ql = new QLabel(label, parent);
		if (small) ql->setFont(qf);
		labelWidget = ql;
	}
	else
	{
		auto *vt = new VerticalText(parent, label);
		if (small) vt->setFont(qf);
		labelWidget = vt;
	}

	return (labelWidget);
}


void MDWSlider::addSliders( QBoxLayout *volLayout, char type, Volume& vol,
                            QList<QAbstractSlider *>& ref_sliders, const QString &tooltipText)
{
	const int minSliderSize = fontMetrics().height() * 10;
	const long minvol = vol.minVolume();
	const long maxvol = vol.maxVolume();

	const QMap<Volume::ChannelID, VolumeChannel> vols = vol.getVolumes();
	for (const VolumeChannel &vc : vols)		// for all channels of this device
	{
		//qCDebug(KMIX_LOG) << "Add label to " << vc.chid << ": " <<  Volume::channelNameReadable(vc.chid);
		QWidget *subcontrolLabel;

		QString subcontrolTranslation;
		if ( type == 'c' ) subcontrolTranslation += i18n("Capture") + ' ';
		subcontrolTranslation += Volume::channelNameReadable(vc.chid);
		subcontrolLabel = createLabel(this, subcontrolTranslation, orientation(), true);
		volLayout->addWidget(subcontrolLabel);

		VolumeSlider *slider = new VolumeSlider(orientation(), this);
		slider->setMinimum(minvol);
		slider->setMaximum(maxvol);

		// Set the slider page step to be the same as the configured volume step.
		// If that volume step is the minimum possible value (1), then set it
		// to 2 so that it is bigger than the single step.
		const int pageStep = qMax(static_cast<int>(vol.volumeStep(false)), 2);
		slider->setPageStep(pageStep);

		// Need to set the single step too, because some devices have a substantial
		// range (e.g. PulseAudio always returns the range as from 0 to 65536).
		// Having the single step at the default setting (i.e. 1) makes key and
		// wheel events over the slider so slow that it appears not to be moving
		// (although it actually is).  See http://bugs.kde.org/show_bug.cgi?id=416405
		// for report.
		//
		// Since volume level is always displayed as a percentage, set the single
		// step to give an increment of 1% unless the range is <100 already.  This
		// will be subject to rounding unless the range is at least 200, but it's
		// the best that can be done.
		const int volRange = maxvol-minvol;
		if (volRange>100)
		{
			const int volStep = qMax(qRound(volRange/100.0), 1);
			slider->setSingleStep(volStep);
		}

		// Don't show too many tick marks if the page step is small.
		if (pageStep<10) slider->setTickInterval(qRound(volRange/10.0));

		slider->setValue(vol.getVolume(vc.chid));
		volumeValues.push_back(vol.getVolume(vc.chid));

		if (orientation()==Qt::Vertical) slider->setMinimumHeight(minSliderSize);
		else slider->setMinimumWidth(minSliderSize);
		if ( !profileControl()->getBackgroundColor().isEmpty() ) {
			slider->setStyleSheet("QSlider { background-color: " + profileControl()->getBackgroundColor() + " }");
		}

		slider->setSubControlLabel(subcontrolLabel);
		slider->setChannelId(vc.chid);

		if ( type == 'p' ) {
			slider->setToolTip( tooltipText );
		}
		else {
			QString captureTip( i18n( "%1 (capture)", tooltipText ) );
			slider->setToolTip( captureTip );
		}

		volLayout->addWidget( slider ); // add to layout
		ref_sliders.append ( slider ); // add to list

		connect(slider, &QAbstractSlider::valueChanged, this, &MDWSlider::volumeChange);
		// TODO: Could these two connections and the tracking of m_sliderInWork
		// be replaced by slider->isSliderDown()?
		connect(slider, &QAbstractSlider::sliderPressed, this, &MDWSlider::sliderPressed);
		connect(slider, &QAbstractSlider::sliderReleased, this, &MDWSlider::sliderReleased);
	}
}


void MDWSlider::sliderPressed()
{
  m_sliderInWork = true;
}


void MDWSlider::sliderReleased()
{
  m_sliderInWork = false;
}


QString MDWSlider::iconName()
{
    return mixDevice()->iconName();
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
	for (QAbstractSlider *slider : ref_sliders)
	{
		slider->setVisible(!m_linked || first); // One slider (the 1st) is always shown

		const VolumeSlider *vs = qobject_cast<VolumeSlider *>(slider);
		// cesken: Not including "|| first" in the check below because the text would
		// not look nice:  it would show "Left" or "Capture Left", while it should
		// be "Playback" and "Capture" in the "linked" case.
		//
		// But the only affected situation is when we have playback AND capture on
		// the same control, where we show no label.  It would be nice to at least
		// put a "Capture" label on the capture subcontrol instead.  To achieve this
		// we would need to change the text on the first capture subcontrol dynamically.
		// This can be done, but I'll leave this open for now.
		if (vs!=nullptr) vs->subControlLabel()->setVisible(!m_linked && showSubcontrolLabels);

		first = false;
	}

	// Redo the tickmarks for the last slider in the slider list.
	//
	// The implementation is not obvious, so let's explain:  We ALWAYS have tickmarks
	// on the LAST slider. Sometimes the slider is not shown, and then we just don't bother.
	//
	// a) So, if the last slider has tickmarks, we can always call setTicks(true).
	//
	// b) if the last slider has NO tickmarks, there are no tickmarks at all, and we
	// don't need to redo the tickmarks.
	QSlider *slider = qobject_cast<QSlider *>(ref_sliders.last());
	if (slider!=nullptr && slider->tickPosition()!=QSlider::NoTicks) setTicks(true);
}


void
MDWSlider::setLabeled(bool value)
{
	if ( m_controlLabel != 0) m_controlLabel->setVisible(value);
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
 * Please note that always only the first and last slider have tickmarks.
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
	if ( m_controlIcon != 0 ) {
		if ( ( !m_controlIcon->isHidden() ) !=value ) {
			if (value)
				m_controlIcon->show();
			else
				m_controlIcon->hide();

			layout()->activate();
		}
	} // if it has an icon
}


/** This slot is called, when a user has changed the volume via the KMix Slider. */
void MDWSlider::volumeChange( int )
{
	if (!m_slidersPlayback.isEmpty())
	{
		++m_waitForSoundSetComplete;
		volumeValues.push_back(m_slidersPlayback.first()->value());
		volumeChangeInternal(mixDevice()->playbackVolume(), m_slidersPlayback);
	}
	if (!m_slidersCapture.isEmpty())
	{
		volumeChangeInternal(mixDevice()->captureVolume(), m_slidersCapture);
	}

	QSignalBlocker blocker(view());
	mixDevice()->mixer()->commitVolumeChange(mixDevice());
}

void MDWSlider::volumeChangeInternal(Volume& vol, QList<QAbstractSlider *>& ref_sliders)
{
	// Changing from the muted state, so ensure unmuted
	mixDevice()->setMuted(false);

	if (isStereoLinked())
	{
		const QAbstractSlider *firstSlider = ref_sliders.first();
		vol.setAllVolumes(firstSlider->value());
	}
	else
	{
		for (int i = 0; i<ref_sliders.count(); ++i)
		{					// iterate over all sliders
			const VolumeSlider *vs = qobject_cast<VolumeSlider *>(ref_sliders.at(i));
			if (vs!=nullptr) vol.setVolume(vs->channelId(), vs->value());
		}
	}
}


/**
   This slot is called, when a user has clicked the recsrc button. Also it is called by any other
    associated QAction like the context menu.
 */
void MDWSlider::toggleRecsrc()
{
	setRecsrc( !mixDevice()->isRecSource() );
}

void MDWSlider::setRecsrc(bool value)
{
	if ( mixDevice()->captureVolume().hasSwitch() )
	{
		mixDevice()->setRecSource( value );
		mixDevice()->mixer()->commitVolumeChange( mixDevice() );
	}
}


/**
   This slot is called, when a user has clicked the mute button. Also it is called by any other
    associated QAction like the context menu.
 */
void MDWSlider::toggleMuted()
{
	setMuted( !mixDevice()->isMuted() );
}

void MDWSlider::setMuted(bool value)
{
	if ( mixDevice()->hasMuteSwitch() )
	{
		mixDevice()->setMuted( value );
		mixDevice()->mixer()->commitVolumeChange(mixDevice());
	}
}


/**
 * This slot is called on a Keyboard Shortcut event, except for the XF86Audio* shortcuts which are handled by the
 * KMixWindow class. So for 99.9% of all users, this method is never called.
 */
void MDWSlider::increaseVolume()
{
  increaseOrDecreaseVolume(false, Volume::Both);
}

/**
 * This slot is called on a Keyboard Shortcut event, except for the XF86Audio* shortcuts which are handled by the
 * KMixWindow class. So for 99.9% of all users, this method is never called.
 */
void MDWSlider::decreaseVolume()
{
  increaseOrDecreaseVolume(true, Volume::Both);
}

/**
 * Increase or decrease all playback and capture channels of the given control.
 * This method is very similar to Mixer::increaseOrDecreaseVolume(), but it will
 * auto-unmute on increase.
 *
 * @param mixdeviceID The control name
 * @param decrease true for decrease. false for increase
 */
void MDWSlider::increaseOrDecreaseVolume(bool decrease, Volume::VolumeTypeFlag volumeType)
{
	mixDevice()->increaseOrDecreaseVolume(decrease, volumeType);
	// I should possibly not block, as the changes that come back from the Soundcard
	//      will be ignored (e.g. because of capture groups)
// 	qCDebug(KMIX_LOG) << "MDWSlider is blocking signals for " << view()->id();
// 	bool oldViewBlockSignalState = view()->blockSignals(true);
	mixDevice()->mixer()->commitVolumeChange(mixDevice());
// 	qCDebug(KMIX_LOG) << "MDWSlider is unblocking signals for " << view()->id();
// 	view()->blockSignals(oldViewBlockSignalState);
}


/**
 * Must be called by the triggered(bool) signal from a QAction.
 */
void MDWSlider::moveStream(bool checked)
{
	Q_UNUSED(checked);
	QAction *act = qobject_cast<QAction *>(sender());
	Q_ASSERT(act!=nullptr);
	const QString destId = act->data().toString();
	mixDevice()->mixer()->moveStream(mixDevice()->id(), destId);
}


/**
 * This is called whenever there are volume updates pending from the hardware for this MDW.
 */
void MDWSlider::update()
{
	if ( m_slidersPlayback.count() != 0 || mixDevice()->hasMuteSwitch() )
		updateInternal(mixDevice()->playbackVolume(), m_slidersPlayback, mixDevice()->isMuted() );
	if ( m_slidersCapture.count()  != 0 || mixDevice()->captureVolume().hasSwitch() )
		updateInternal(mixDevice()->captureVolume(), m_slidersCapture, mixDevice()->isNotRecSource() );
	if (m_controlLabel!=nullptr)
	{
		QLabel *l;
		VerticalText *v;
		if ((l = dynamic_cast<QLabel*>(m_controlLabel)))
			l->setText(mixDevice()->readableName());
		else if ((v = dynamic_cast<VerticalText*>(m_controlLabel)))
			v->setText(mixDevice()->readableName());
	}
	updateAccesability();
}

/**
 *
 * @param vol
 * @param ref_sliders
 * @param muted Future directions: passing "muted" should not be necessary any longer - due to getVolumeForGUI()
 */
void MDWSlider::updateInternal(Volume& vol, QList<QAbstractSlider *>& ref_sliders, bool muted)
{
	for (int i = 0; i<ref_sliders.count(); ++i)
	{						// for all sliders
		VolumeSlider *slider = qobject_cast<VolumeSlider *>(ref_sliders.at(i));
		if (slider==nullptr) continue;

		Volume::ChannelID chid = slider->channelId();
		long useVolume = muted ? 0 : vol.getVolumeForGUI(chid);

		QSignalBlocker blocker(slider);
		// --- Avoid feedback loops START -----------------
		const int volume_index = volumeValues.indexOf(useVolume);
		if (volume_index>-1 && --m_waitForSoundSetComplete<1)
		{
		    m_waitForSoundSetComplete = 0;
		    volumeValues.removeAt(volume_index);

		    if (!m_sliderInWork) slider->setValue(useVolume);
		}
		else if (!m_sliderInWork && m_waitForSoundSetComplete<1)
		{
			slider->setValue(useVolume);
		}
		// --- Avoid feedback loops END -----------------
	}


	// update mute state
	if (m_muteButton!=nullptr)
	{
		QSignalBlocker blocker(m_muteButton);
		m_muteButton->setActive(!mixDevice()->isMuted());
	}

	// update capture state
	if (m_captureButton!=nullptr)
	{
		QSignalBlocker blocker(m_captureButton);
		m_captureButton->setActive(mixDevice()->isRecSource());
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
                QList<VolumeChannel> vols = mixDevice()->playbackVolume().getVolumes().values();
                for (QAbstractSlider *slider : m_slidersPlayback) {
                        slider->setAccessibleName(slider->toolTip()+ " (" +Volume::channelNameReadable(vols.first().chid)+')');
                        vols.pop_front();
                }
                vols = mixDevice()->captureVolume().getVolumes().values();
                for (QAbstractSlider *slider : m_slidersCapture) {
                        slider->setAccessibleName(slider->toolTip()+ " (" +Volume::channelNameReadable(vols.first().chid)+')');
                        vols.pop_front();
                }
        }
}
#endif


void MDWSlider::createContextMenu(QMenu *menu)
{
	if (m_moveMenu!=nullptr)
	{
		MixSet *ms = mixDevice()->moveDestinationMixSet();
		Q_ASSERT(ms!=nullptr);

		m_moveMenu->setEnabled(ms->count()>1);
		// The "Event Sounds" stream cannot be moved at present.  This is because
		// Mixer_PULSE::moveStream() does not record the stream ID in the
		// output stream list and hence cannot get its PulseAudio stream index.
		// I don't know whether this is a design decision or a PA limitation.
		if (mixDevice()->id().endsWith(QLatin1String(":event"))) m_moveMenu->setEnabled(false);

		menu->addMenu( m_moveMenu );
	}

	if (m_slidersPlayback.count()>1 || m_slidersCapture.count()>1)
	{
		KToggleAction *stereo = qobject_cast<KToggleAction *>(channelActions()->action("stereo"));
		if (stereo!=nullptr) {
			QSignalBlocker blocker(stereo);
			stereo->setChecked(!isStereoLinked());
			menu->addAction( stereo );
		}
	}

	if (mixDevice()->captureVolume().hasSwitch())
	{
		KToggleAction *ta = qobject_cast<KToggleAction *>(channelActions()->action("recsrc"));
		if (ta!=nullptr) {
			QSignalBlocker blocker(ta);
			ta->setChecked( mixDevice()->isRecSource() );
			menu->addAction( ta );
		}
	}

	if (mixDevice()->hasMuteSwitch())
	{
		KToggleAction *ta = qobject_cast<KToggleAction *>(channelActions()->action("mute"));
		if (ta!=nullptr) {
			QSignalBlocker blocker(ta);
			ta->setChecked( mixDevice()->isMuted() );
			menu->addAction( ta );
		}
	}
}


void MDWSlider::showMoveMenu()
{
    const MixSet *ms = mixDevice()->moveDestinationMixSet();
    Q_ASSERT(ms!=nullptr);

    const QString cur = mixDevice()->mixer()->currentStreamDevice(mixDevice()->id());

    // There is no need to keep a record of the actions (in a KActionCollection
    // or otherwise);  QMenu::clear() will delete them as long as they are owned
    // by the menu.
    m_moveMenu->clear();

    // Default action
    QAction *act = new QAction(i18n("Automatic (according to category)"), m_moveMenu);
    act->setData(QString());
    connect(act, &QAction::triggered, this, &MDWSlider::moveStream, Qt::QueuedConnection);
    m_moveMenu->addAction(act);

    m_moveMenu->addSeparator();

    // Device actions
    for (const shared_ptr<MixDevice> md : *ms)
    {
	act = new QAction(QIcon::fromTheme(md->iconName()), md->readableName(), m_moveMenu);
	act->setData(md->id());
	act->setCheckable(true);
	if (md->id()==cur) act->setChecked(true);
	connect(act, &QAction::triggered, this, &MDWSlider::moveStream, Qt::QueuedConnection);
	m_moveMenu->addAction(act);
    }
}


/**
 * An event filter for the various widgets making up this control.
 *
 * Redirect all wheel events to the main slider, so that they will be
 * handled consistently regardless of where the pointer actually is.
 */
bool MDWSlider::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type()!=QEvent::Wheel) return (QWidget::eventFilter(obj, ev));
							// only want wheel events
	if (!ev->spontaneous()) return (false);		// avoid recursion on slider

	QAbstractSlider *slider = qobject_cast<QAbstractSlider *>(obj);
	if (slider!=nullptr)				// event is over a slider
	{
		// Do nothing in this case.  No event filter is installed
		// on a slider, and it will handle the wheel event itself.
		qCWarning(KMIX_LOG) << "unexpected wheel event on slider" << slider;
		return (false);
	}

	// Mouse is not over a slider.  Find the principal slider (the first
	// playback control if there are any, otherwise the first capture
	// control if any) and redirect the event to that.
	if (!m_slidersPlayback.isEmpty()) slider = m_slidersPlayback.first();
	else if (!m_slidersCapture.isEmpty()) slider = m_slidersCapture.first();
	else slider = nullptr;

	if (slider!=nullptr)
	{
		//qCDebug(KMIX_LOG) << "identified for slider" << slider;
		QCoreApplication::sendEvent(slider, ev);
	}

	return (true);					// wheel event handled
}
