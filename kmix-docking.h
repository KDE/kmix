/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-98 Christian Esken
 *                        esken@kde.org
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


#ifndef KMIX_DOCKING_H
#define KMIX_DOCKING_H

#include "docking.h"
#include <qrect.h>

class KMixDockWidget : public KDockWidget {

  Q_OBJECT

public:
  KMixDockWidget(const char *name=0, const QString& dockIconName=0);
  ~KMixDockWidget();

  void paintEvent (QPaintEvent* );
  void setDisplay(int i_volume);


  void mouseReleaseEvent ( QMouseEvent *e );
  void mousePressLeftEvent ( QMouseEvent * );
  void mouseMoveEvent ( QMouseEvent *qme );

signals:
  void quickchange(int diff);

private:
  void doPaint();

  int i_i_percent;
  int i_i_height;
  int i_i_y;
  int i_i_width;

  bool i_b_mouse_moved;
  bool i_b_move_active;
  int  i_i_diff;
  int  i_i_click_x;
  int  i_i_click_y;
  int  i_i_move_delta;
};

#endif

