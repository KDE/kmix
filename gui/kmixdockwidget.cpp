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

// Define this if KStatusNotifierItem::setOverlayIconByName() works.
// As of March 2020 it appears to be not working, the base icon is
// shown but with no overlay.
//#define CAN_USE_ICON_OVERLAY	1

#include <klocalizedstring.h>
#include <kwindowsystem.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDesktopWidget>
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QScreen>
#include <QTextDocument>
#ifndef CAN_USE_ICON_OVERLAY
#include <QPainter>
#endif // CAN_USE_ICON_OVERLAY

#include "apps/kmix.h"
#include "core/ControlManager.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "gui/dialogselectmaster.h"
#include "gui/mixdevicewidget.h"
#include "gui/viewdockareapopup.h"


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

    connect(this, &KStatusNotifierItem::scrollRequested, this, &KMixDockWidget::trayWheelEvent);
    connect(this, &KStatusNotifierItem::secondaryActivateRequested, this, &KMixDockWidget::dockMute);

	// For bizarre reasons, we wrap the ViewDockAreaPopup in a QMenu. Must relate to how KStatusNotifierItem works.
    // TODO: Is this necessary? Deleted and recreated in activate()
    _dockAreaPopupMenuWrapper = new QMenu(parent);
    _volWA = new QWidgetAction(_dockAreaPopupMenuWrapper);
    _dockView = new ViewDockAreaPopup(_dockAreaPopupMenuWrapper, "dockArea", {}, QString("no-guiprofile-yet-in-dock"), parent);
	_volWA->setDefaultWidget(_dockView);
	_dockAreaPopupMenuWrapper->addAction(_volWA);
	connect(contextMenu(), &QMenu::aboutToShow, this, &KMixDockWidget::contextMenuAboutToShow);

	ControlManager::instance().addListener(
		QString(), // All mixers (as the Global master Mixer might change)
		ControlManager::Volume|ControlManager::MasterChanged, this,
		QString("KMixDockWidget"));
	 
	      // Refresh in all cases. When there is no Global Master we still need
     // to initialize correctly (e.g. for showing 0% or hiding it)
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

void KMixDockWidget::controlsChange(ControlManager::ChangeType changeType)
{
  switch (changeType)
  {
    case  ControlManager::MasterChanged:
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

    case ControlManager::Volume:
      refreshVolumeLevels();
      break;

    default:
      ControlManager::warnUnexpectedChangeType(changeType, this);
  }
}

QAction *KMixDockWidget::findAction(const char *actionName)
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
    return (nullptr);
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
    if (menu==nullptr) return;				// We do not use a menu

    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if (md.get()!=nullptr && md->hasMuteSwitch()) {
        // Put "Mute" selector in context menu
        KToggleAction *action = new KToggleAction(i18n("M&ute"), this);
        action->setData("dock_mute");
        addAction("dock_mute", action);
    	updateDockMuteAction(action);
        connect(action, &QAction::triggered, this, &KMixDockWidget::dockMute);
        menu->addAction(action);
    }

    // Put "Select Master Channel" dialog in context menu
    QAction *action = _kmixMainWindow->actionCollection()->action("select_master");
    action->setEnabled(Mixer::getGlobalMasterMixer() != 0);
    menu->addAction(action);

    // Context menu entry to access KMix settings
    // action name from tier3/kconfigwidgets/src/kstandardaction_p.h
    menu->addAction(_kmixMainWindow->actionCollection()->action("options_configure"));
}

void KMixDockWidget::setVolumeTip()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    QString tip;
    QString subTip;
    int virtualToolTipValue = 0;

    if (md.get()==nullptr)
    {
        tip = i18n("No mixer devices available");
        virtualToolTipValue = -2;
    }
    else
    {
        // Playback volume will be used for the DockIcon if available.
        // This heuristic is "good enough" for the DockIcon for now.
        int val = md->userVolumeLevel();
        if (md->isMuted()) tip = i18n("Volume muted");
        else tip = i18n("Volume at %1%", val);

        subTip = i18n("%1 - %2",
		      md->mixer()->readableName().toHtmlEscaped(),
		      md->readableName().toHtmlEscaped());

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

    char newPixmapType = 'e';				// default = error indication
    if (md.get()!=nullptr)				// a control is available
    {
	const int percentage = md->userVolumeLevel();
	if      (percentage<=0) newPixmapType = '0';	// also muted or negative value
	else if (percentage<25) newPixmapType = '1';
	else if (percentage<75) newPixmapType = '2';
	else                    newPixmapType = '3';
    }

    if (newPixmapType!=_oldPixmapType)			// pixmap must be changed => do so
    {
        switch (newPixmapType)
        {
case 'e':   {
#ifdef CAN_USE_ICON_OVERLAY
                setIconByName("audio-volume-medium");
                setOverlayIconByName("error");
#else // CAN_USE_ICON_OVERLAY
		// Using QIcon/QPixmap here because dependency on KIconThemes was removed
		// by commit 6b59a9a8.  This notification code path will only be used if
		// no sound devices are available or if there is a serious error, so any
		// inefficiency or HiDPI display problem is not serious.
		//
		// Based on KStatusNotifierItem::setOverlayIconByName()
		// in tier2/knotifications/src/kstatusnotifieritem.cpp
		// Pixmap size 24 from s_legacyTrayIconSize in the same source file

                QPixmap iconPix = QIcon::fromTheme("audio-volume-medium").pixmap(24, 24, QIcon::Disabled);
                QPixmap overlayPix = QIcon::fromTheme("error").pixmap(12, 12, QIcon::Normal);

                QPainter p(&iconPix);
                p.drawPixmap(iconPix.width()-overlayPix.width(), iconPix.height()-overlayPix.height(), overlayPix);
                p.end();

                setIconByPixmap(QIcon(iconPix));
#endif // CAN_USE_ICON_OVERLAY
            }
            break;

case 'm':
case '0':   setIconByName("audio-volume-muted");
            break;

case '1':   setIconByName("audio-volume-low");
            break;

case '2':   setIconByName("audio-volume-medium");
            break;

case '3':   setIconByName("audio-volume-high");
            break;
        }
    }

    _oldPixmapType = newPixmapType;
}

/**
 * Called when the system tray icon gets "activated".
 *
 * This can happen in two ways, either by a left-click on the icon or
 * by "Restore" from the context menu.  In the first case the sender()
 * is a null pointer and 'pos' is the popup position.  In the second
 * case the sender() is the KStatusNotifierItem's QAction and 'pos' is
 * a null QPoint.
 *
 * Left-clicking on the system tray icon should pop up the volume control,
 * as expected.  However, it is not useful to do the same for "Restore" as
 * there is no 'pos' to open at (and no way to find it), so the popup volume
 * control appears at the top left of the screen.  It is more useful, and
 * follows the action label more accurately, to open the main KMix window.
 */
void KMixDockWidget::activate(const QPoint &pos)
{
	if (pos.isNull())				// "Restore" from the menu
	{
		_dockView->showPanelSlot();
		return;
	}

	QWidget* dockAreaPopup = _dockAreaPopupMenuWrapper; // TODO Refactor to use _dockAreaPopupMenuWrapper directly
	if (dockAreaPopup->isVisible())
	{
		dockAreaPopup->hide();
		return;
	}

	_dockAreaPopupMenuWrapper->removeAction(_volWA);
	delete _volWA;
	_volWA = new QWidgetAction(_dockAreaPopupMenuWrapper);
	_dockView = new ViewDockAreaPopup(_dockAreaPopupMenuWrapper, "dockArea", {}, QString("no-guiprofile-yet-in-dock"),
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
	const QScreen *screen = QGuiApplication::screenAt(pos);
	if (screen==nullptr) return;
	const QRect vScreenSize = screen->availableGeometry();

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
	KWindowSystem::setState(dockAreaPopup->winId(), NET::KeepAbove | NET::SkipTaskbar | NET::SkipPager);
	dockAreaPopup->show();
	dockAreaPopup->move(x, y);
}


void KMixDockWidget::trayWheelEvent(int delta,Qt::Orientation wheelOrientation)
{
	shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
	if (md.get()==nullptr) return;

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


void KMixDockWidget::dockMute()
{
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if (md.get()!=nullptr)
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
    qCDebug(KMIX_LOG) << "DOCK MUTE" << dockMuteAction;
    if (dockMuteAction!=nullptr) updateDockMuteAction(dockMuteAction);
}

void KMixDockWidget::updateDockMuteAction ( KToggleAction* dockMuteAction )
{  
    shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
    if (md.get()!=nullptr && dockMuteAction!=nullptr)
    {
    	Volume& vol = md->playbackVolume().hasVolume() ? md->playbackVolume() : md->captureVolume();
    	bool isInactive =  vol.isCapture() ? !md->isRecSource() : md->isMuted();
        bool hasSwitch = vol.isCapture() ? vol.hasSwitch() : md->hasMuteSwitch();
        dockMuteAction->setEnabled( hasSwitch );
        dockMuteAction->setChecked( isInactive );
    }
}
