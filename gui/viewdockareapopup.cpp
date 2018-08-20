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
#include <QIcon>
#include <QLayoutItem>
#include <QPushButton>
#include <QSizePolicy>

// KDE
#include <klocalizedstring.h>
#include <kwindowsystem.h>

// KMix
#include "apps/kmix.h"
#include "core/mixer.h"
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"
#include "gui/dialogchoosebackends.h"
#include "gui/guiprofile.h"
#include "gui/kmixprefdlg.h"
#include "gui/mdwslider.h"
#include "dialogbase.h"

// Restore volume button feature is incomplete => disabling for KDE 4.10
#undef RESTORE_VOLUME_BUTTON

QString ViewDockAreaPopup::InternedString_Star = QString("*");
QString ViewDockAreaPopup::InternedString_Subcontrols = QString("pvolume,cvolume,pswitch,cswitch");
ProfControl* ViewDockAreaPopup::MatchAllForSoundMenu = 0;
//ProfControl(ViewDockAreaPopup::InternedString_Star, ViewDockAreaPopup::InternedString_Subcontrols);

ViewDockAreaPopup::ViewDockAreaPopup(QWidget* parent, QString id, ViewBase::ViewFlags vflags, QString guiProfileId,
	KMixWindow *dockW) :
	ViewBase(parent, id, 0, vflags, guiProfileId), _kmixMainWindow(dockW)
{
	resetRefs();
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	/*
	 * Bug 206724:
	 * Following are excerpts of trying to receive key events while this popup is open.
	 * The best I could do with lots of hacks is to get the keyboard events after a mouse-click in the popup.
	 * But such a solution is neither intuitive nor helpful - if one clicks, then usage of keyboard makes not much sense any longer. 
	 * I finally gave up on fixing Bug 206724.
	 */
	 /*
	releaseKeyboard();
	setFocusPolicy(Qt::StrongFocus);
	setFocus(Qt::TabFocusReason);
	releaseKeyboard();
	mainWindowButton->setFocusPolicy(Qt::StrongFocus);
	
	Also implemented the following "event handlers", but they do not show any signs of key presses:
    keyPressEvent()
    x11Event()
    eventFilter()
    */
	foreach ( Mixer* mixer, Mixer::mixers() )
	{
		// Adding all mixers, as we potentially want to show all master controls
		addMixer(mixer);
		// The list will be redone in _setMixSet() with the actual Mixer instances to use
	}

	restoreVolumeIcon = QIcon::fromTheme(QLatin1String("quickopen-file"));
	createDeviceWidgets();

	// Register listeners for all mixers
	ControlManager::instance().addListener(
		QString(), // all mixers
		(ControlChangeType::Type) (ControlChangeType::GUI | ControlChangeType::ControlList | ControlChangeType::Volume
			| ControlChangeType::MasterChanged), this, QString("ViewDockAreaPopup"));
}


ViewDockAreaPopup::~ViewDockAreaPopup()
{
  ControlManager::instance().removeListener(this);
  delete _layoutMDW;
  // Hint: optionsLayout and "everything else" is deleted when "delete _layoutMDW" cascades down
}


void ViewDockAreaPopup::controlsChange(int changeType)
{
  ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
  switch (type )
  {
    case  ControlChangeType::ControlList:
    case  ControlChangeType::MasterChanged:
      createDeviceWidgets();
      break;
    case ControlChangeType::GUI:
    	updateGuiOptions();
      break;

    case ControlChangeType::Volume:
      refreshVolumeLevels();
      break;

    default:
      ControlManager::warnUnexpectedChangeType(type, this);
      break;
  }
    
}


void ViewDockAreaPopup::wheelEvent ( QWheelEvent * e )
{
  if ( _mdws.isEmpty() )
    return;
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
	seperatorBetweenMastersAndStreams = 0;
	separatorBetweenMastersAndStreamsInserted = false;
	separatorBetweenMastersAndStreamsRequired = false;
	configureViewButton = 0;
	restoreVolumeButton1 = 0;
	restoreVolumeButton2 = 0;
	restoreVolumeButton3 = 0;
	restoreVolumeButton4 = 0;
	mainWindowButton = 0;
	optionsLayout = 0;
	_layoutMDW = 0;
}

void ViewDockAreaPopup::_setMixSet()
{
	resetMdws();

	if (optionsLayout != 0)
	{
		QLayoutItem *li2;
		while ((li2 = optionsLayout->takeAt(0)))
			delete li2;
	}
// Hint : optionsLayout itself is deleted when "delete _layoutMDW" cascades down

	if (_layoutMDW != 0)
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
#7  0x00007f9c9d4286b0 in ViewDockAreaPopup::_setMixSet (this=0x1272b60) at /home/chris/workspace/kmix-git-trunk/gui/viewdockareapopup.cpp:164
#8  0x00007f9c9d425700 in ViewBase::createDeviceWidgets (this=0x1272b60) at /home/chris/workspace/kmix-git-trunk/gui/viewbase.cpp:137
#9  0x00007f9c9d42845b in ViewDockAreaPopup::controlsChange (this=0x1272b60, changeType=2) at /home/chris/workspace/kmix-git-trunk/gui/viewdockareapopup.cpp:91
    */
//   if ( optionsLayout != 0 )
//   {
//     QLayoutItem *li2;
//     while ( ( li2 = optionsLayout->takeAt(0) ) ) // strangely enough, it crashes here
// 	    delete li2;
//   }

   // --- Due to the strange crash, delete everything manually : START ---------------
   // I am a bit confused why this doesn't crash. I moved the "optionsLayout->takeAt(0) delete" loop at the beginning,
   // so the obejcts should already be deleted. ...
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
	delete layout(); // BKO 299754
	_layoutMDW = new QGridLayout(this);
	_layoutMDW->setSpacing(DialogBase::verticalSpacing());
	// Review #121166: Add some space over device icons, otherwise they may hit window border
	_layoutMDW->setContentsMargins(0,5,0,0);
	//_layoutMDW->setSizeConstraint(QLayout::SetMinimumSize);
	_layoutMDW->setSizeConstraint(QLayout::SetMaximumSize);
	_layoutMDW->setObjectName(QLatin1String("KmixPopupLayout"));
    setLayout(_layoutMDW);

	// Adding all mixers, as we potentially want to show all master controls. Due to hotplugging
	// we have to redo the list on each _setMixSet() (instead of setting it once in the Constructor)
	_mixers.clear();

	QSet<QString> preferredMixersForSoundmenu = GlobalConfig::instance().getMixersForSoundmenu();
//	qCDebug(KMIX_LOG) << "Launch with " << preferredMixersForSoundmenu;
	foreach ( Mixer* mixer, Mixer::mixers() )
	{
		bool useMixer = preferredMixersForSoundmenu.isEmpty() || preferredMixersForSoundmenu.contains(mixer->id());
		if (useMixer)
			addMixer(mixer);
	}

	// The following loop is for the case when everything gets filtered out. We "reset" to show everything then.
	// Hint: Filtering everything out can only be an "accident", e.g. when restarting KMix with changed hardware or
	// backends.
	if ( _mixers.isEmpty() )
	{
		foreach ( Mixer* mixer, Mixer::mixers() )
		{
			addMixer(mixer);
		}
	}


	// A loop that adds the Master control of each card
	foreach ( Mixer* mixer, _mixers )
	{
//		qCDebug(KMIX_LOG) << "ADD? mixerId=" << mixer->id();
		shared_ptr<MixDevice>dockMD = mixer->getLocalMasterMD();
		if ( !dockMD && mixer->size() > 0 )
		{
			// If we have no dock device yet, we will take the first available mixer device.
			dockMD = (*mixer)[0];
		}
		if ( dockMD )
		{
//			qCDebug(KMIX_LOG) << "ADD? mixerId=" << mixer->id() << ", md=" << dockMD->id();
			if ( !dockMD->isApplicationStream() && dockMD->playbackVolume().hasVolume())
			{
//				qCDebug(KMIX_LOG) << "ADD? mixerId=" << mixer->id() << ", md=" << dockMD->id() << ": YES";
				// don't add application streams here. They are handled below, so
				// we make sure to not add them twice
				_mixSet.append(dockMD);
			}
		}
	} // loop over all cards

	// Add all application streams
	foreach ( Mixer* mixer2 , _mixers )
	{
		foreach ( shared_ptr<MixDevice> md, mixer2->getMixSet() )
		{
			if (md->isApplicationStream())
			{
				_mixSet.append(md);
			}
		}
	}

}


QWidget* ViewDockAreaPopup::add(shared_ptr<MixDevice> md)
{
  bool vertical = (GlobalConfig::instance().data.getTraypopupOrientation() == Qt::Vertical); // I am wondering whether using vflags for this would still make sense
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
    
    if (MatchAllForSoundMenu == 0)
    {
    	// Lazy init of static member on first use
    	MatchAllForSoundMenu = new ProfControl(ViewDockAreaPopup::InternedString_Star, ViewDockAreaPopup::InternedString_Subcontrols);
    }
    ProfControl *pctl = MatchAllForSoundMenu;

    MixDeviceWidget *mdw = new MDWSlider(
      md,           // only 1 device.
      true,         // Show Mute LE
      true,        // Show Record LED
      true,        // Include Mixer Name
      false,        // Small
      vertical ? Qt::Vertical : Qt::Horizontal,
      this,         // parent
      this             // NOT ANYMORE!!! -> Is "NULL", so that there is no RMB-popup
      , pctl
   );
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
//   qCDebug(KMIX_LOG) << "ViewDockAreaPopup::constructionFinished()\n";

	mainWindowButton = new QPushButton(QIcon::fromTheme("show-mixer"), "" , this);
	mainWindowButton->setObjectName(QLatin1String("MixerPanel"));
	mainWindowButton->setToolTip(i18n("Show the full mixer window"));
	connect(mainWindowButton, SIGNAL(clicked()), SLOT(showPanelSlot()));

	configureViewButton = createConfigureViewButton();

	optionsLayout = new QHBoxLayout();
	optionsLayout->addWidget(mainWindowButton);
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

	updateGuiOptions();

	_layoutMDW->update();
	_layoutMDW->activate();

    //bool fnc = focusNextChild();
    //qCWarning(KMIX_LOG) << "fnc=" <<fnc;

//	qCDebug(KMIX_LOG) << "F layout()=" << layout() << ", _layoutMDW=" << _layoutMDW;
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
  foreach ( QWidget* qw, _mdws )
  {
    //qCDebug(KMIX_LOG) << "rvl: " << qw;
    MixDeviceWidget* mdw = qobject_cast<MixDeviceWidget*>(qw);
    if ( mdw != 0 ) mdw->update();
  }
}

void ViewDockAreaPopup::configureView()
{
//    Q_ASSERT( !pulseaudioPresent() );

//    QSet<QString> currentlyActiveMixersInDockArea;
//	foreach ( Mixer* mixer, _mixers )
//	{
//		currentlyActiveMixersInDockArea.insert(mixer->id());
//	}

	KMixPrefDlg* prefDlg = KMixPrefDlg::getInstance();
	//prefDlg->setActiveMixersInDock(currentlyActiveMixersInDockArea);
	prefDlg->switchToPage(KMixPrefDlg::PrefSoundMenu);
}

/**
 * This gets activated whne a user clicks the "Mixer" PushButton in this popup.
 */
void ViewDockAreaPopup::showPanelSlot()
{
    _kmixMainWindow->setVisible(true);
    KWindowSystem::setOnDesktop(_kmixMainWindow->winId(), KWindowSystem::currentDesktop());
    KWindowSystem::activateWindow(_kmixMainWindow->winId());
    // This is only needed when the window is already visible.
    static_cast<QWidget*>(parent())->hide();
}

