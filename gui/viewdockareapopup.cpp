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

#include "gui/viewdockareapopup.h"

// Qt
#include <qevent.h>
#include <qframe.h>
#include <QGridLayout>
#include <QLayoutItem>
#include <QSizePolicy>

// KDE
#include <klocalizedstring.h>
#include <kglobalaccel.h>
#include <kxmlguiwindow.h>
#include <kx11extras.h>

// KMix
#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "gui/guiprofile.h"
#include "gui/kmixprefdlg.h"
#include "gui/mdwslider.h"
#include "dialogbase.h"
#include "settings.h"

// Restore volume button feature is incomplete => disabling for KDE 4.10
#undef RESTORE_VOLUME_BUTTON


ViewDockAreaPopup::ViewDockAreaPopup(QWidget* parent, const QString &id, ViewBase::ViewFlags vflags,
				     const QString &guiProfileId, KXmlGuiWindow *dockW)
	: ViewBase(parent, id, {}, vflags, guiProfileId),
	  _kmixMainWindow(dockW)
{
	resetRefs();
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	// Original comments from cesken on commit 2bd1c4cb 12 Aug 2014:
	//
	//   Bug 206724:
	//   Following are excerpts of trying to receive key events while
	//   this popup is open.  The best I could do with lots of hacks is
	//   to get the keyboard events after a mouse-click in the popup.
	//   But such a solution is neither intuitive nor helpful - if one
	//   clicks, then usage of keyboard makes not much sense any longer.
	//   I finally gave up on fixing Bug 206724.
	//
	//   releaseKeyboard();
	//   setFocusPolicy(Qt::StrongFocus);
	//   setFocus(Qt::TabFocusReason);
	//   releaseKeyboard();
	//   mainWindowButton->setFocusPolicy(Qt::StrongFocus);
	//
	//   Also implemented the following "event handlers", but they
	//   do not show any signs of key presses:
	//
	//   keyPressEvent()
	//   x11Event()
	//   eventFilter()
	//
	// However, that applied to KDE4/Qt4.  it does now seem to be possible
	// to receive key events on the popup, by doing setFocusPolicy() and
	// setFocus() below.  setFocus() needs to be called on an idle timer,
	// if it is not called or is called directly there is the very strange
	// effect that the popup does not receive key events until one of the
	// up/down arrow keys is first pressed;  the first key disappears but
	// then all subsequent events for all keys are received.
	//
	setFocusPolicy(Qt::StrongFocus);
	QTimer::singleShot(0, this, [this]() { setFocus(Qt::OtherFocusReason); });

	// Adding all mixers, as we potentially want to show all master controls.
	// The list will be redone in initLayout() with the actual Mixer instances to use.
	//
	// TODO: is this necessary?  As the comment says, initLayout() does a
	// clearMixers() which clears the mixers that addMixer() adds here.
	for (Mixer *mixer : qAsConst(MixerToolBox::mixers()))
	{
		addMixer(mixer);
	}

	restoreVolumeIcon = QIcon::fromTheme(QLatin1String("quickopen-file"));
	createDeviceWidgets();

	// Register listeners for all mixers
	ControlManager::instance().addListener(
		QString(), // all mixers
		ControlManager::GUI|ControlManager::ControlList|ControlManager::Volume|ControlManager::MasterChanged,
		this, QString("ViewDockAreaPopup"));
}


ViewDockAreaPopup::~ViewDockAreaPopup()
{
  ControlManager::instance().removeListener(this);
  delete _layoutMDW;
  // Hint: optionsLayout and "everything else" is deleted when "delete _layoutMDW" cascades down
}


void ViewDockAreaPopup::keyPressEvent(QKeyEvent *ev)
{
	// Key events are received here, after the widget has been set up for
	// focus in the constructor above.  However, this event handler receives
	// all key events, including global ones - this is presumably because
	// KMixDockWidget wraps the popup volume control in a menu which grabs
	// the keyboard while shown.  Releasing the grab closes the menu and
	// hence the popup, so the grab has to be maintained.
	//
	// Handle the small subset of key events that we are interested in:
	// the up/down arrow keys for a volume step, Page Up/Down for a
	// double step, and Space or M to toggle mute.  Since we also receive
	// global shortcuts, check for the configured KMix global shortcuts
	// and act on them.  Unfortunately, as is the case with any menu when
	// it is active, all other keys are grabbed and other global shortcuts
	// will not work.  The standard QWidget::keyPressEvent() handler will
	// close the popup on Escape.

	KActionCollection *ac = _kmixMainWindow->actionCollection();
	QAction *actUp = ac->action("increase_volume");
	QAction *actDown = ac->action("decrease_volume");
	QAction *actMute = ac->action("mute");

	int key = ev->key();
	const QKeySequence seq(key);
	// KGlobalAccel::shortcut() is safe if called with a null pointer.
	if (KGlobalAccel::self()->shortcut(actUp).contains(seq)) key = Qt::Key_Up;
	if (KGlobalAccel::self()->shortcut(actDown).contains(seq)) key = Qt::Key_Down;
	if (KGlobalAccel::self()->shortcut(actMute).contains(seq)) key = Qt::Key_Space;

	switch (key)
	{
case Qt::Key_PageUp:
            if (actUp!=nullptr) actUp->trigger();	// double step up
	    Q_FALLTHROUGH();

case Qt::Key_Up:
            if (actUp!=nullptr) actUp->trigger();	// single step up
            return;

case Qt::Key_PageDown:
            if (actDown!=nullptr) actDown->trigger();	// double step down
	    Q_FALLTHROUGH();

case Qt::Key_Down:
            if (actDown!=nullptr) actDown->trigger();	// single step down
            return;

case Qt::Key_M:
case Qt::Key_Space:
            if (actMute!=nullptr) actMute->trigger();	// toggle mute
            return;
	}

	QWidget::keyPressEvent(ev);			// handle others, including ESC
}


void ViewDockAreaPopup::controlsChange(ControlManager::ChangeType changeType)
{
  switch (changeType)
  {
    case  ControlManager::ControlList:
    case  ControlManager::MasterChanged:
      createDeviceWidgets();
      break;
    case ControlManager::GUI:
    	updateGuiOptions();
      break;

    case ControlManager::Volume:
      refreshVolumeLevels();
      break;

    default:
      ControlManager::warnUnexpectedChangeType(changeType, this);
      break;
  }
    
}


void ViewDockAreaPopup::configurationUpdate()
{
	// TODO Do we still need configurationUpdate(). It was never implemented for ViewDockAreaPopup
}

// TODO Currently no right-click, as we have problems to get the ViewDockAreaPopup resized
 void ViewDockAreaPopup::showContextMenu()
 {
     // no right-button-menu on "dock area popup"
     return;
 }

void ViewDockAreaPopup::resetRefs()
{
	seperatorBetweenMastersAndStreams = nullptr;
	separatorBetweenMastersAndStreamsInserted = false;
	separatorBetweenMastersAndStreamsRequired = false;
	configureViewButton = nullptr;
	restoreVolumeButton1 = nullptr;
	restoreVolumeButton2 = nullptr;
	restoreVolumeButton3 = nullptr;
	restoreVolumeButton4 = nullptr;
	mainWindowButton = nullptr;
	optionsLayout = nullptr;
	_layoutMDW = nullptr;
}


void ViewDockAreaPopup::addDevice(shared_ptr<MixDevice> md, const Mixer *mixer)
{
	shared_ptr<MixDevice> dockMD = md;
	if (dockMD==nullptr && mixer->numDevices()>0)
	{
		// If the mixer has no local master device defined,
		// then take its first available device.  first() is
		// safe here because size() has been checked above.
		dockMD = mixer->mixDevices().first();
	}

	if (dockMD==nullptr) return;			// still no master device

	// Do not add application streams here, they are handled elsewhere.
	if (dockMD->isApplicationStream()) return;

	// Add the device if it has a volume control.
	if (dockMD->playbackVolume().hasVolume() || dockMD->captureVolume().hasVolume())
	{
		qDebug() << "adding" << dockMD->id() << dockMD->readableName();
		//qCDebug(KMIX_LOG) << "ADD? mixerId=" << mixer->id() << ", md=" << dockMD->id() << ": YES";
		addToMixSet(dockMD);
	}
}


static bool shouldUseMixer(const QStringList &filter, const QString &id)
{
	return (filter.isEmpty() || filter.contains(id));
}


void ViewDockAreaPopup::initLayout()
{
	resetMdws();

	if (optionsLayout!=nullptr)
	{
		QLayoutItem *li2;
		while ((li2 = optionsLayout->takeAt(0)))
			delete li2;
	}
// Hint : optionsLayout itself is deleted when "delete _layoutMDW" cascades down

	if (_layoutMDW!=nullptr)
	{
		QLayoutItem *li;
		while ((li = _layoutMDW->takeAt(0)))
			delete li;
	}

   /*
    * Strangely enough, I cannot delete optionsLayout in a loop. I get a strange stacktrace:
    *
Application: KMix (kmix), signal: Segmentation fault
[...]
#6  0x00007f9c9a282900 in QString::shared_null () from /usr/lib/x86_64-linux-gnu/libQtCore.so.4
#7  0x00007f9c9d4286b0 in ViewDockAreaPopup::initLayout (this=0x1272b60) at /home/chris/workspace/kmix-git-trunk/gui/viewdockareapopup.cpp:164
#8  0x00007f9c9d425700 in ViewBase::createDeviceWidgets (this=0x1272b60) at /home/chris/workspace/kmix-git-trunk/gui/viewbase.cpp:137
#9  0x00007f9c9d42845b in ViewDockAreaPopup::controlsChange (this=0x1272b60, changeType=2) at /home/chris/workspace/kmix-git-trunk/gui/viewdockareapopup.cpp:91
    */
//   if ( optionsLayout != nullptr )
//   {
//     QLayoutItem *li2;
//     while ( ( li2 = optionsLayout->takeAt(0) ) ) // strangely enough, it crashes here
// 	    delete li2;
//   }

   // --- Due to the strange crash, delete everything manually : START ---------------
   // I am a bit confused why this doesn't crash. I moved the "optionsLayout->takeAt(0) delete" loop at the beginning,
   // so the objects should already be deleted. ...
	delete configureViewButton;
	delete restoreVolumeButton1;
	delete restoreVolumeButton2;
	delete restoreVolumeButton3;
	delete restoreVolumeButton4;

	delete mainWindowButton;
	delete seperatorBetweenMastersAndStreams;
	// --- Due to the strange crash, delete everything manually : END ---------------

	resetRefs();
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	/*
	 * BKO 299754: Looks like I need to explicitly delete layout(). I have no idea why
	 *             "delete _layoutMDW" is not enough, as that is supposed to be the layout
	 *             of this  ViewDockAreaPopup
	 *             (Hint: it might have been 0 already. Nowadays it is definitely, see #resetRefs())
	 */
	delete layout(); // Bug 299754
	_layoutMDW = new QGridLayout(this);
	_layoutMDW->setSpacing(DialogBase::verticalSpacing());
	// Review #121166: Add some space over device icons, otherwise they may hit window border
	_layoutMDW->setContentsMargins(0,5,0,0);
	//_layoutMDW->setSizeConstraint(QLayout::SetMinimumSize);
	_layoutMDW->setSizeConstraint(QLayout::SetMaximumSize);
	_layoutMDW->setObjectName(QLatin1String("KmixPopupLayout"));
	setLayout(_layoutMDW);

	// Adding all mixers, as we potentially want to show all master controls.
	// Due to hotplugging we have to redo the list on each initLayout() instead of
	// setting it once in the constructor.
	clearMixers();

	// This is the "Mixers to show in the popup volume control" list
	// as set in the "Preferences - Volume Control" dialogue.
	// For ALSA this will select from the available cards and also
	// MPRIS2 "Playback Streams".  For PulseAudio this will select from
	// the four preset "PlayBack/Capture Devices/Streams", the loop
	// below adds invividual PulseAudio devices.
	const QStringList &preferredMixersForSoundmenu = Settings::mixersForSoundMenu();
	//qCDebug(KMIX_LOG) << "Launch with " << preferredMixersForSoundmenu;
	for (Mixer *mixer : std::as_const(MixerToolBox::mixers()))
	{
		const bool useMixer = shouldUseMixer(preferredMixersForSoundmenu, mixer->id());

		// addMixer() here is ViewBase::addMixer() which appends
		// the 'mixer' to the list which will be returned
		// by getMixers() below.
		if (useMixer) addMixer(mixer);
	}

	// The following loop is for the case where no devices were added
	// above, for example when the 'preferredMixersForSoundmenu' list was
	// not empty but contained only mixers which have now disappeared due
	// to hotplugging.  We "reset" the list to show everything.
	if (getMixers().isEmpty())
	{
		for (Mixer *mixer : std::as_const(MixerToolBox::mixers()))
		{
			addMixer(mixer);
		}
	}

	// A loop that adds the Master control of each card
	const auto &mixers = getMixers();
	qDebug() << "adding" << mixers.count() << "mixers";

	for (const Mixer *mixer : std::as_const(mixers))
	{
		qDebug() << "mixer" << mixer->id() << mixer->readableName();
		if (mixer->getDriverName()=="PulseAudio")
		{
			// For PulseAudio, there is the fixed set of four mixers
			// "Playback Devices" etc.  All playback devices, for example,
			// appear under that.  Following the original logic as for
			// ALSA, a volume control is shown only for the configured
			// master playback device.  Under ALSA, this still shows a
			// control for all devices because each device is a separate
			// mixer.
			//
			// The same also applies to "Capture Devices".
			//
			// In order to achieve the same user-visible behaviour as with
			// ALSA, for PulseAudio there is a second loop below which adds
			// all devices of the mixer, not just the configured master.
			// Therefore a volume control is shown for all playback
			// devices.
			const MixSet &ms = mixer->mixDevices();
			qDebug() << "PulseAudio adding" << ms.count() << "devices";
			for (shared_ptr<MixDevice> md : std::as_const(ms))
			{
				const bool useMixer = shouldUseMixer(preferredMixersForSoundmenu, md->id());
				if (useMixer) addDevice(md, mixer);
			}
		}
		else					// not PulseAudio
		{
			shared_ptr<MixDevice> md = mixer->getLocalMasterMD();
			addDevice(md, mixer);
		}
	} // loop over all cards

	// Finally add all application streams
	for (const Mixer *mixer : std::as_const(getMixers()))
	{
		for (shared_ptr<MixDevice> md : std::as_const(mixer->mixDevices()))
		{
			if (md->isApplicationStream()) addToMixSet(md);
		}
	}
}


QWidget* ViewDockAreaPopup::add(shared_ptr<MixDevice> md)
{
  const bool vertical = (orientation()==Qt::Vertical);

  /*
    QString dummyMatchAll("*");
    QString matchAllPlaybackAndTheCswitch("pvolume,cvolume,pswitch,cswitch");
    // Leak | relevant | pctl Each time a stream is added, a new ProfControl gets created.
    //      It cannot be deleted in ~MixDeviceWidget, as ProfControl* ownership is not consistent.
    //      here a new pctl is created (could be deleted), but in ViewSliders the ProcControl is taken from the
    //      MixDevice, which in turn uses it from the GUIProfile.
    //  Summarizing: ProfControl* is either owned by the GUIProfile or created new (ownership unclear).
    //  Hint: dummyMatchAll and matchAllPlaybackAndTheCswitch leak together with pctl
    ProfControl *pctl = new ProfControl( dummyMatchAll, matchAllPlaybackAndTheCswitch);
 */
    
    if ( !md->isApplicationStream() )
    {
      separatorBetweenMastersAndStreamsRequired = true;
    }
    if ( !separatorBetweenMastersAndStreamsInserted && separatorBetweenMastersAndStreamsRequired && md->isApplicationStream() )
    {
      // First application stream => add separator
      separatorBetweenMastersAndStreamsInserted = true;

   int sliderColumn = vertical ? _layoutMDW->columnCount() : _layoutMDW->rowCount();
   int row = vertical ? 0 : sliderColumn;
   int col = vertical ? sliderColumn : 0;
   seperatorBetweenMastersAndStreams = new QFrame(this);
   if (vertical)
     seperatorBetweenMastersAndStreams->setFrameStyle(QFrame::VLine);
   else
     seperatorBetweenMastersAndStreams->setFrameStyle(QFrame::HLine);
_layoutMDW->addWidget( seperatorBetweenMastersAndStreams, row, col );
//_layoutMDW->addItem( new QSpacerItem( 5, 5 ), row, col );
    }
    
    static ProfControl *MatchAllForSoundMenu = nullptr;
    if (MatchAllForSoundMenu==nullptr)
    {
        // Lazy init of static member on first use
        MatchAllForSoundMenu = new ProfControl("*", "pvolume,cvolume,pswitch,cswitch");
    }

    MixDeviceWidget *mdw = new MDWSlider(md,
					 MixDeviceWidget::ShowMute|MixDeviceWidget::ShowCapture|MixDeviceWidget::ShowMixerName,
					 this,
					 MatchAllForSoundMenu);
    mdw->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
   int sliderColumn = vertical ? _layoutMDW->columnCount() : _layoutMDW->rowCount();
   //if (sliderColumn == 1 ) sliderColumn =0;
   int row = vertical ? 0 : sliderColumn;
   int col = vertical ? sliderColumn : 0;
   
   _layoutMDW->addWidget( mdw, row, col );

   //qCDebug(KMIX_LOG) << "ADDED " << md->id() << " at column " << sliderColumn;
   return mdw;
}


void ViewDockAreaPopup::constructionFinished()
{
	//qCDebug(KMIX_LOG);

	mainWindowButton = new QPushButton(QIcon::fromTheme("show-mixer"), "" , this);
	mainWindowButton->setObjectName(QLatin1String("MixerPanel"));
	mainWindowButton->setToolTip(i18n("Show the full mixer window"));
	connect(mainWindowButton, SIGNAL(clicked()), SLOT(showPanelSlot()));

	configureViewButton = createConfigureViewButton();

	optionsLayout = new QHBoxLayout();
	optionsLayout->addWidget(mainWindowButton);
	optionsLayout->addStretch(1);
	optionsLayout->addWidget(configureViewButton);

#ifdef RESTORE_VOLUME_BUTTON
	restoreVolumeButton1 = createRestoreVolumeButton(1);
	optionsLayout->addWidget( restoreVolumeButton1 ); // TODO enable only if user has saved a volume profile

//    optionsLayout->addWidget( createRestoreVolumeButton(2) );
//    optionsLayout->addWidget( createRestoreVolumeButton(3) );
//    optionsLayout->addWidget( createRestoreVolumeButton(4) );
#endif

	int sliderRow = _layoutMDW->rowCount();
	_layoutMDW->addLayout(optionsLayout, sliderRow, 0, 1, _layoutMDW->columnCount());

	// The controls layout needs to be adjusted after the popup has been shown.
	QTimer::singleShot(0, this, [this](){ adjustControlsLayout(); });
	updateGuiOptions();
}


QPushButton* ViewDockAreaPopup::createRestoreVolumeButton ( int storageSlot )
{
	QString buttonText = QString("%1").arg(storageSlot);
	QPushButton* profileButton = new QPushButton(restoreVolumeIcon, buttonText, this);
	profileButton->setToolTip(i18n("Load volume profile %1", storageSlot));
	profileButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	return profileButton;
}


void ViewDockAreaPopup::refreshVolumeLevels()
{
	const int num = mixDeviceCount();
	for (int i = 0; i<num; ++i)
	{
		MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(mixDeviceAt(i));
		if (mdw!=nullptr) mdw->update();
	}
}


void ViewDockAreaPopup::configureView()
{
	KMixPrefDlg::instance()->showAtPage(KMixPrefDlg::PageVolumeControl);
}


/**
 * This gets activated when a user clicks the "Mixer" PushButton in this popup.
 */
void ViewDockAreaPopup::showPanelSlot()
{
    _kmixMainWindow->setVisible(true);
    KX11Extras::setOnDesktop(_kmixMainWindow->winId(), KX11Extras::currentDesktop());
    KX11Extras::activateWindow(_kmixMainWindow->winId());
    // This is only needed when the window is already visible.
    static_cast<QWidget *>(parent())->hide();
}


Qt::Orientation ViewDockAreaPopup::orientationSetting() const
{
	return (static_cast<Qt::Orientation>(Settings::orientationTrayPopup()));
}
