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
#include <kstatusnotifieritem.h>

class KMixWindow;
class Mixer;
#include "core/mixdevice.h"
class ViewDockAreaPopup;
class Volume;

/**
 * @brief The MetaMixer class provides a solid wrapper around a possible changing mixer.
 *
 * The primay use of this class is to provide one instance to connect slots to
 * while the underlying object that triggers the signals can be changing.
 * Additionally it nicely hides the rewiring logic that is going on in the back.
 */
class MetaMixer : public QObject
{
    Q_OBJECT
public:
    MetaMixer() : m_mixer(0) {}

    /**
     * @brief rewires against current master mixer
     * @note this also triggers all signals to ensure UI updates are done accordingly
     */
    void reset();

    Mixer *mixer() const { return m_mixer; }
    bool hasMixer() const { return m_mixer; }

private:
    Mixer *m_mixer;
};

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
   void createActions();
   void toggleMinimizeRestore();

 private:
   ViewDockAreaPopup *_referenceWidget2;
   KMenu *_referenceWidget;
   QWidgetAction *_volWA;
   bool _ignoreNextEvent;
   int  _oldToolTipValue;
   char _oldPixmapType;
   bool _volumePopup;
   KMixWindow* _kmixMainWindow;
   MetaMixer m_metaMixer;

   bool _contextMenuWasOpen;
   void refreshVolumeLevels();

 private slots:
   void dockMute();
   void trayWheelEvent(int delta,Qt::Orientation);
   void selectMaster();
   void contextMenuAboutToShow();
   int getUserfriendlyVolumeLevel(const shared_ptr<MixDevice>& md);
};

#endif
