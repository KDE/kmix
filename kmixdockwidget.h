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
#include <QWidget>

#include <knotificationitem-1/knotificationitem.h>

class KMixWindow;
class Mixer;
//class ViewDockAreaPopup;
class Volume;

namespace Phonon
{
    class MediaObject;
}

using namespace Experimental;

class KMixDockWidget : public KNotificationItem
{
   Q_OBJECT

   friend class KMixWindow;

 public:
   explicit KMixDockWidget(KMixWindow *parent, QWidget *referenceWidget, bool volumePopup);
   ~KMixDockWidget();

   void setErrorPixmap();
   void ignoreNextEvent();
   //ViewDockAreaPopup* getDockAreaPopup();

 signals:
   void newMasterSelected();

 public slots:
   void setVolumeTip();
   void updatePixmap();
   void activate(const QPoint &pos);

 protected:
   void createMasterVolWidget();
   void createActions();
//    void trayToolTipEvent(QHelpEvent *e );
   void toggleMinimizeRestore();

 private:
   QWidget *_referenceWidget;
   Phonon::MediaObject *_audioPlayer;
   bool _playBeepOnVolumeChange;
   bool _ignoreNextEvent;
   int  _oldToolTipValue;
   char _oldPixmapType;
   bool _volumePopup;
   KMixWindow* _kmixMainWindow;
   Mixer *m_mixer;

 private slots:
   void dockMute();
   void trayWheelEvent(int delta);
   //void kmixSystrayAction(QSystemTrayIcon::ActivationReason);
   void selectMaster();
   void handleNewMaster(QString& soundcard_id, QString& channel_id);
   void contextMenuAboutToShow();
};

#endif
