/*
 *              kmix: KDE's small mixer
 *
 *              Copyright (C) 1997 Christian Esken
 *                       esken@kde.org
 * 
 * This file is based on a contribution by Harri Porten <porten@tu-harburg.de>
 *
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


#ifndef _DOCKING_H_
#define _DOCKING_H_

#include <stdio.h>
#include <qapp.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qpopmenu.h>
#include <qpoint.h>


class DockWidget : public QWidget {

  Q_OBJECT

public:
  DockWidget(const char *name=0);
  ~DockWidget();

protected:
  void paintEvent(QPaintEvent *e);

private slots:
  void timeclick();
  void toggle_window_state();
  void mousePressEvent(QMouseEvent *e);

public slots:
  void dock();
  void undock();
  void paintIcon();

public:
  const bool isDocked();

private:
  bool docked;
  int toggleID;
  int pos_x;
  int pos_y;
  QPopupMenu *popup_m;

  QPixmap cdsmall_pixmap;

};

#endif

