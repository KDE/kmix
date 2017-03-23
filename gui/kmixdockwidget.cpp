/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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

#include "gui/kmixdockwidget.h"

#include <kaction.h>
#include <klocale.h>
#include <kmenu.h>
#include <kdebug.h>
#include <kwindowsystem.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDesktopWidget>
#include <QAction>
#include <QApplication>
#include <QTextDocument>

#include "apps/kmix.h"
#include "core/ControlManager.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "gui/dialogselectmaster.h"
#include "gui/mixdevicewidget.h"
#include "gui/viewdockareapopup.h"


//#define FEATURE_UNITY_POPUP true

KMixDockWidget::KMixDockWidget(KMixWindow* parent)
    : KStatusNotifierItem(parent)
    , _oldToolTipValue(-1)
    , _oldPixmapType('-')
    , _kmixMainWindow(parent)
    , _delta(0)
{
    setToolTipIconByName("kmix");
    setTitle(i18n( "Volume Control"));
    setCategory(Hardware);
    setStatus(Active);

    // TODO Unity / Gnome only support one type of activation (left-click == right-click)
    //      So we should show here the ViewDockAreaPopup instead of the menu:
    //bool onlyOneMouseButtonAction = onlyHaveOneMouseButtonAction();

    createMenuActions();

    connect(this, SIGNAL(scrollRequested(int,Qt::Orientation)), this, SLOT(trayWheelEvent(int,Qt::Orientation)));
    connect(this, SIGNAL(secondaryActivateRequested(QPoint)), this, SLOT(dockMute()));

	// For bizarre reasons, we wrap the ViewDockAreaPopup in a KMenu. Must relate to how KStatusNotifierItem works.
   _dockAreaPopupMenuWrapper = new KMenu(parent);
	_volWA = new QWidgetAction(_dockAreaPopupMenuWrapper);
	_dockView = new ViewDockAreaPopup(_dockAreaPopupMenuWrapper, "dockArea", 0, QString("no-guiprofile-yet-in-dock"), parent);
	_volWA->setDefaultWidget(_dockView);
	_dockAreaPopupMenuWrapper->addAction(_volWA);
	connect(contextMenu(), SIGNAL(aboutToShow()), this, SLOT(contextMenuAboutToShow()));

	ControlManager::instance().addListener(
		QString(), // All mixers (as the Global master Mixer might change)
		(ControlChangeType::Type) (ControlChangeType::Volume | ControlChangeType::MasterChanged), this,
		QString("KMixDockWidget"));
	 
	      // Refresh in all cases. When there is no Golbal Master we still need
     // to initialize correctly (e.g. for showin 0% or hiding it)
     refreshVolumeLevels();
}

KMixDockWidget::~KMixDockWidget()
{
	ControlManager::instance().removeListener(this);
	// Note: deleting _volWA also deletes its associated ViewDockAreaPopup (_referenceWidget) and prevents the
	//       action to be left with a dangling pointer.
	//       cesken: I adapted the patch from https://bugs.kde.org/show_bug.cgi?id=220621#c27 to branch /branches/work/kmix
	delete _volWA;
}

void KMixDockWidget::controlsChange(int changeType)
{
  ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
  switch (type )
  {
    case  ControlChangeType::MasterChanged:
      // Notify the main window, as it might need to update the visibiliy of the dock icon.
//      _kmixMainWindow->updateDocking();
//      _kmixMainWindow->saveConfig();
      refreshVolumeLevels();
      {
		  QAction *selectMasterAction = findAction("select_master");
		  if(selectMasterAction)
		  {
			  // Review #120432 : Guard findAction("select_master"), as it is sometimes 0 on the KF5 build
			  //                  This is probably not a final solution, but better than a crash.
			  selectMasterAction->setEnabled(Mixer::getGlobalMasterMixer() != 0);
		  }
		  else
		  {
			  qCWarning(KMIX_LOG) << "select_master action not found. Cannot enable it in the Systray.";
		  }
      }
      break;

    case ControlChangeType::Volume:
      refreshVolumeLevels();
      break;

    default:
      ControlManager::warnUnexpectedChangeType(type, this);
  }
}

QAction* KMixDockWidget::findAction(const char* actionName)
{
	QList<QAction*> actions = actionCollection();
	int size = actions.size();
	for (int i=0; i<size; ++i)
	{
		QAction* action = actions.at(i);
        if (action->data().toString() == QString::fromUtf8(actionName))
			return action;
	}
    qCWarning(KMIX_LOG) << "ACTION" << actionName << "NOT FOUND!";
    return Q_NULLPTR;
}

/**
 * Updates all visual parts of the volume, namely tooltip and pixmap
 */
void KMixDockWidget::refreshVolumeLevels()
{
  setVolumeTip();
  updatePixmap();
}

/**
 * Creates the right-click menu
 */
void KMixDockWidget::createMenuActions()
{
    QMenu *menu = contextMenu();
    if (!menu)
        return; // We do not use a menu

    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if ( md.get() != 0 && md->hasMuteSwitch() ) {
        // Put "Mute" selector in context menu
        KToggleAction *action = new KToggleAction(i18n("M&ute"), this);
        action->setData("dock_mute");
        addAction("dock_mute", action);
    	updateDockMuteAction(action);
        connect(action, SIGNAL(triggered(bool)), SLOT(dockMute()));
        menu->addAction( action );
    }

    // Put "Select Master Channel" dialog in context menu
    QAction *action = new QAction(i18n("Select Master Channel..."), this);
    action->setData("select_master");
    addAction("select_master", action);
    action->setEnabled(Mixer::getGlobalMasterMixer() != 0);
    connect(action, SIGNAL(triggered(bool)), _kmixMainWindow, SLOT(slotSelectMaster()));
    menu->addAction( action );

    //Context menu entry to access phonon settings
    menu->addAction(_kmixMainWindow->actionCollection()->action("launch_kdesoundsetup"));
}

void
KMixDockWidget::setVolumeTip()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    QString tip;
    QString subTip;
    int virtualToolTipValue = 0;

    if ( md.get() == 0 )
    {
        tip = i18n("Mixer cannot be found"); // !! text could be reworked
        virtualToolTipValue = -2;
    }
    else
    {
        // Playback volume will be used for the DockIcon if available.
        // This heuristic is "good enough" for the DockIcon for now.
        int val = md->getUserfriendlyVolumeLevel();
        tip += "<font size=\"+1\">" + i18n( "Volume at %1%", val ) + "</font>";
        if ( md->isMuted() )
            tip += i18n( " (Muted)" );
        subTip = QString( "%1<br/>%2" )
                 .arg( Qt::escape(md->mixer()->readableName()) ).arg( Qt::escape(md->readableName()) );

        // create a new "virtual" value. With that we see "volume changes" as well as "muted changes"
        virtualToolTipValue = val;
        if ( md->isMuted() )
        	virtualToolTipValue += 10000;
    }

    // The actual updating is only done when the "toolTipValue" was changed (to avoid flicker)
    if ( virtualToolTipValue != _oldToolTipValue )
    {
        // changed (or completely new tooltip)
        setToolTipTitle(tip);
        setToolTipSubTitle(subTip);
    }
    _oldToolTipValue = virtualToolTipValue;
}


void KMixDockWidget::updatePixmap()
{
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();

    char newPixmapType;
    if ( !md )
    {
    	// no such control => error
        newPixmapType = 'e';
    }
    else
    {
    	int percentage = md->getUserfriendlyVolumeLevel();
		if      ( percentage <= 0 ) newPixmapType = '0';  // Hint: also muted, and also negative-values
		else if ( percentage < 25 ) newPixmapType = '1';
		else if ( percentage < 75 ) newPixmapType = '2';
		else                        newPixmapType = '3';
    }

   if ( newPixmapType != _oldPixmapType ) {
      // Pixmap must be changed => do so
      switch ( newPixmapType ) {
         case 'e': setIconByName( "kmixdocked_error" ); break;
         case 'm': 
         case '0': setIconByName( "audio-volume-muted"  ); break;
         case '1': setIconByName( "audio-volume-low"  ); break;
         case '2': setIconByName( "audio-volume-medium" ); break;
         case '3': setIconByName( "audio-volume-high" ); break;
      }
   }

   _oldPixmapType = newPixmapType;
}

/**
 * Called whenever the icon gets "activated". Usually when its clicked.
 * @overload
 * @param pos
 */
void KMixDockWidget::activate(const QPoint &pos)
{
	QWidget* dockAreaPopup = _dockAreaPopupMenuWrapper; // TODO Refactor to use _dockAreaPopupMenuWrapper directly
	if (dockAreaPopup->isVisible())
	{
		dockAreaPopup->hide();
		return;
	}

	_dockAreaPopupMenuWrapper->removeAction(_volWA);
	delete _volWA;
	_volWA = new QWidgetAction(_dockAreaPopupMenuWrapper);
	_dockView = new ViewDockAreaPopup(_dockAreaPopupMenuWrapper, "dockArea", 0, QString("no-guiprofile-yet-in-dock"),
		_kmixMainWindow);
	_volWA->setDefaultWidget(_dockView);
	_dockAreaPopupMenuWrapper->addAction(_volWA);

	//_dockView->show(); // TODO cesken check: this should be automatic
	// Showing, to hopefully get the geometry manager started. We need width and height below. Also
	// vdesktop->availableGeometry(dockAreaPopup) needs to know on which screen the widget will be shown.
//	dockAreaPopup->show();
	_dockView->adjustSize();
	dockAreaPopup->adjustSize();

	int x = pos.x() - dockAreaPopup->width() / 2;
	if (x < 0)
		x = pos.x();
	int y = pos.y() - dockAreaPopup->height() / 2;
	if (y < 0)
		y = pos.y();

	// Now handle Multihead displays. And also make sure that the dialog is not
	// moved out-of-the screen on the right (see Bug 101742).
	const QDesktopWidget* vdesktop = QApplication::desktop();
	int screenNumber = vdesktop->screenNumber(pos);
	const QRect& vScreenSize = vdesktop->availableGeometry(screenNumber);

	if ((x + dockAreaPopup->width()) > (vScreenSize.width() + vScreenSize.x()))
	{
		// move horizontally, so that it is completely visible
		x = vScreenSize.width() + vScreenSize.x() - dockAreaPopup->width() - 1;
		qCDebug(KMIX_LOG) << "Multihead: (case 1) moving to" << x << "," << y;
	}
	else if (x < vScreenSize.x())
	{
		// horizontally out-of bound
		x = vScreenSize.x();
		qCDebug(KMIX_LOG) << "Multihead: (case 2) moving to" << x << "," << y;
	}

	if ((y + dockAreaPopup->height()) > (vScreenSize.height() + vScreenSize.y()))
	{
		// move horizontally, so that it is completely visible
		y = vScreenSize.height() + vScreenSize.y() - dockAreaPopup->height() - 1;
		qCDebug(KMIX_LOG) << "Multihead: (case 3) moving to" << x << "," << y;
	}
	else if (y < vScreenSize.y())
	{
		// horizontally out-of bound
		y = vScreenSize.y();
		qCDebug(KMIX_LOG) << "Multihead: (case 4) moving to" << x << "," << y;
	}


	KWindowSystem::setType(dockAreaPopup->winId(), NET::Dock);
	KWindowSystem::setState(dockAreaPopup->winId(), NET::StaysOnTop | NET::SkipTaskbar | NET::SkipPager);
	dockAreaPopup->show();
	dockAreaPopup->move(x, y);
}


void
KMixDockWidget::trayWheelEvent(int delta,Qt::Orientation wheelOrientation)
{
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
	if ( md.get() == 0 )
		return;


	Volume &vol = ( md->playbackVolume().hasVolume() ) ?  md->playbackVolume() : md->captureVolume();
//	qCDebug(KMIX_LOG) << "I am seeing a wheel event with delta=" << delta << " and orientation=" <<  wheelOrientation;
	if (wheelOrientation == Qt::Horizontal) // Reverse horizontal scroll: bko228780
	{
		delta = -delta;
	}
	// bko313579, bko341536, Review #121725 - Use delta and round it by 120.
	_delta += delta;
	bool decrease = delta < 0;
	unsigned long inc = 0;
	while (_delta >= 120) {
		_delta -= 120;
		inc++;
	}
	while (_delta <= -120) {
		_delta += 120;
		inc++;
	}

	if (inc == 0) {
		return;
	}
	long cv = vol.volumeStep(decrease) * inc;

    bool isInactive =  vol.isCapture() ? !md->isRecSource() : md->isMuted();
//    qCDebug(KMIX_LOG) << "Operating on capture=" << vol.isCapture() << ", isInactive=" << isInactive;
	if ( cv > 0 && isInactive)
	{
		// increasing from muted state: unmute and start with a low volume level
		if ( vol.isCapture())
			md->setRecSource(true);
		else
			md->setMuted(false);
		vol.setAllVolumes(cv);
	}
	else
	    vol.changeAllVolumes(cv);

    md->mixer()->commitVolumeChange(md);
    refreshVolumeLevels();
}


void
KMixDockWidget::dockMute()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if ( md )
    {
        md->toggleMute();
        md->mixer()->commitVolumeChange( md );
        refreshVolumeLevels();
    }
}

/**
 * Returns whether the running Desktop only supports one Mouse Button
 * Hint: Unity / Gnome only support one type of activation (left-click == right-click).
 */
bool KMixDockWidget::onlyHaveOneMouseButtonAction()
{
	QDBusConnection connection = QDBusConnection::sessionBus();
    bool unityIsRunning = (connection.interface()->isServiceRegistered("com.canonical.Unity.Panel.Service"));
    // Possibly implement other detectors, like for Gnome 3 or Gnome 2
    return unityIsRunning;

}

void KMixDockWidget::contextMenuAboutToShow()
{
    // Enable/Disable "Muted" menu item
    KToggleAction *dockMuteAction = static_cast<KToggleAction*>(findAction("dock_mute"));
    qDebug() << "DOCK MUTE" << dockMuteAction;
    if (dockMuteAction)
        updateDockMuteAction(dockMuteAction);
}

void KMixDockWidget::updateDockMuteAction ( KToggleAction* dockMuteAction )
{  
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if ( md && dockMuteAction != 0 )
    {
    	Volume& vol = md->playbackVolume().hasVolume() ? md->playbackVolume() : md->captureVolume();
    	bool isInactive =  vol.isCapture() ? !md->isRecSource() : md->isMuted();
        bool hasSwitch = vol.isCapture() ? vol.hasSwitch() : md->hasMuteSwitch();
        dockMuteAction->setEnabled( hasSwitch );
        dockMuteAction->setChecked( isInactive );
    }
}

