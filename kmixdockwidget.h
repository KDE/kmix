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

#include <qwidget.h>
#include <qvbox.h>
#include <ksystemtray.h>

class Mixer;
class QFrame;
class KAudioPlayer;
class KMixMasterVolume;
class MixDeviceWidget;

class KMixDockWidget : public KSystemTray  {
   Q_OBJECT

   friend class KMixWindow;

 public:
   KMixDockWidget(Mixer *, QWidget *parent=0, const char *name=0);
   ~KMixDockWidget();

   void updatePixmap();
   void setErrorPixmap();

 public slots:
   void setVolumeTip(int, Volume);

 protected:
   void createMasterVolWidget();
   void mousePressEvent(QMouseEvent *);
   void mouseReleaseEvent(QMouseEvent *);
   void wheelEvent(QWheelEvent *);
   void contextMenuAboutToShow( KPopupMenu* menu );
   void toggleMinimizeRestore();

 private:
   Mixer *m_mixer;
   KMixMasterVolume *masterVol;
   KAudioPlayer *audioPlayer;
   bool m_playBeepOnVolumeChange;
   bool m_mixerVisible;
};

class KMixMasterVolume : public QVBox
{
    public:
        KMixMasterVolume(QWidget* parent, const char* name, Mixer* mixer);
        ~KMixMasterVolume() {}
        MixDeviceWidget* mixerWidget() { return mdw; }

    protected:
        MixDeviceWidget *mdw;
};

#endif
