/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef KMIXDOCKWIDGET_H
#define KMIXDOCKWIDGET_H

#include <qwidget.h>
#include <ksystemtray.h>

class Mixer;
class QFrame;

class KMixDockWidget : public KSystemTray  {
   Q_OBJECT

   friend class KMixWindow;

 public: 
   KMixDockWidget(Mixer *, QWidget *parent=0, const char *name=0);
   ~KMixDockWidget();

 protected slots:  
   void setVolumeTip(int, Volume);

 protected:
   void createMasterVolWidget();
   void mousePressEvent(QMouseEvent *);
   void mouseReleaseEvent(QMouseEvent *);
   void mouseDoubleClickEvent(QMouseEvent *);
   void contextMenuAboutToShow( KPopupMenu* menu );

 private:
   Mixer *m_mixer;
   QFrame *masterVol;   
};

#endif
