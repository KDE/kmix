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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KMIXDOCKWIDGET_H
#define KMIXDOCKWIDGET_H

class QFrame;
class QString;
#include <qwidget.h>
#include <qvbox.h>

#include <ksystemtray.h>

class Mixer;
class KAudioPlayer;
class MixDeviceWidget;
class Mixer;
class ViewDockAreaPopup;
class Volume;

class KMixDockWidget : public KSystemTray  {
   Q_OBJECT

   friend class KMixWindow;

 public:
   KMixDockWidget(Mixer *, QWidget *parent=0, const char *name=0, bool volumePopup=true);
   ~KMixDockWidget();

   void setErrorPixmap();
   void ignoreNextEvent();
   ViewDockAreaPopup* getDockAreaPopup();

   Mixer *m_mixer;
   ViewDockAreaPopup *_dockAreaPopup;
   KAudioPlayer *_audioPlayer;

 public slots:
   void setVolumeTip();
   void updatePixmap();

 protected:
   void createMasterVolWidget();
   void createActions();
   void mousePressEvent(QMouseEvent *);
   void mouseReleaseEvent(QMouseEvent *);
   void wheelEvent(QWheelEvent *);
   void contextMenuAboutToShow( KPopupMenu* menu );
   void toggleMinimizeRestore();

 private:
   bool _playBeepOnVolumeChange;
   bool _ignoreNextEvent;
   int  _oldToolTipValue;
   char _oldPixmapType;
   bool _volumePopup;
 private slots:
   void dockMute();
   void selectMaster();
   void handleNewMaster(int soundcard_id, QString& channel_id);
};

#endif
