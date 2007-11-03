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

#include <ksystemtrayicon.h>

class Mixer;
class ViewDockAreaPopup;
class Volume;

namespace Phonon
{
       class MediaObject;
}

class KMixDockWidget : public KSystemTrayIcon  {
   Q_OBJECT

   friend class KMixWindow;

 public:
   explicit KMixDockWidget(QWidget *parent=0, const char *name=0, bool volumePopup=true);
   ~KMixDockWidget();

   void setErrorPixmap();
   void ignoreNextEvent();
   ViewDockAreaPopup* getDockAreaPopup();

   Mixer *m_mixer;
   ViewDockAreaPopup *_dockAreaPopup;
   Phonon::MediaObject *_audioPlayer;

 public slots:
   void setVolumeTip();
   void updatePixmap();

 protected:
   void createMasterVolWidget();
   void createActions();
   void mousePressEvent(QMouseEvent *);
   void mouseReleaseEvent(QMouseEvent *);
   void wheelEvent(QWheelEvent *);
   void contextMenuAboutToShow( KMenu* menu );
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
   void handleNewMaster(QString& soundcard_id, QString& channel_id);
};

#endif
