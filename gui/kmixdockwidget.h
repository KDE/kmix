/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2003 Sven Leiber <s.leiber@web.de>
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

#ifndef KMIXDOCKWIDGET_H
#define KMIXDOCKWIDGET_H

class QString;
class QWidgetAction;

class KToggleAction;
#include <kstatusnotifieritem.h>

class KMixWindow;
class Mixer;
#include "core/mixdevice.h"
class ViewDockAreaPopup;
class Volume;

class KMixDockWidget : public KStatusNotifierItem
{
   Q_OBJECT

   friend class KMixWindow;

 public:
   explicit KMixDockWidget(KMixWindow *parent,bool volumePopup);
   virtual ~KMixDockWidget();

   void setErrorPixmap();
   void ignoreNextEvent();
   void update();

 public slots:
   void setVolumeTip();
   void updatePixmap();
   void activate(const QPoint &pos);
   void controlsChange(int changeType);

 protected:
   void createMenuActions();
   void toggleMinimizeRestore();

 private:
   ViewDockAreaPopup *_dockAreaPopup;
   KMenu *_dockAreaPopupMenuWrapper;
   QWidgetAction *_volWA;
   int  _oldToolTipValue;
   char _oldPixmapType;
   KMixWindow* _kmixMainWindow;

   bool _contextMenuWasOpen;
   bool onlyHaveOneMouseButtonAction();
   void refreshVolumeLevels();
   void updateDockMuteAction ( KToggleAction* dockMuteAction );

 private slots:
   void dockMute();
   void trayWheelEvent(int delta,Qt::Orientation);
   void selectMaster();
   void contextMenuAboutToShow();
};

#endif
