/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
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

#include "kmix.h"

#include <iostream.h>


#include <qtooltip.h>
#include <qpainter.h>
#include <kwm.h>
#include <kapp.h>

#include "kmix-docking.h"


KMixDockWidget::KMixDockWidget(const char *name, 
			       const QString& dockIconName) 
    : KDockWidget((QString)name, dockIconName)
{
  //CT assure that the vars are correctly initialized before the first paint
  setDisplay(0); 

  // Now for the mouse click stuff
  i_b_move_active = false;
  i_i_move_delta  = 5;
}

KMixDockWidget::~KMixDockWidget()
{
}


void KMixDockWidget::doPaint()
{
  QPainter paint( this );
  int h = this->height();
  for (int i = 0; i < h; i=i+2) {
    paint.setPen( i>i_i_height?gray:QColor(255*i/h, 0, 255*(h-i)/h) );
    paint.drawLine(i_i_width / 2, h - i,
		   i_i_width / 2 + i_i_width * i / h, h - i);
  }
}

void KMixDockWidget::paintEvent (QPaintEvent* )
{
  doPaint();
}

void KMixDockWidget::setDisplay(int val_i_percent)
{
  i_i_percent = val_i_percent;
  i_i_height  = ( val_i_percent * this->height() ) / 100;
  i_i_y	      = this->height() - i_i_height;
  i_i_width	= this->width() / 2;

  doPaint();
}

void KMixDockWidget::mousePressLeftEvent ( QMouseEvent *qme )
{
  cerr << "mplEvent()\n";

  i_b_mouse_moved = false;
  i_b_move_active = true;
  i_i_click_x = qme->x();
  i_i_click_y = qme->y();
}


void KMixDockWidget::mouseReleaseEvent ( QMouseEvent* /*e*/ )
{
  if ( i_b_mouse_moved )
    emit quickchange(i_i_diff);
  else
    toggle_window_state();

  i_b_move_active = false;
}

void KMixDockWidget::mouseMoveEvent ( QMouseEvent *qme )
{
  if ( !i_b_move_active ) {
    KDockWidget::mouseMoveEvent(qme);
  }
  else {
    // We come here if the LEFT mouse button was pressed and the mouse was moved
    if ( ! i_b_mouse_moved ) {
      // Verifying whether the mouse was moved far away after the click
      if ( i_b_move_active ) {
	if ( /* abs ( i_i_click_x - qme->x() ) > i_i_move_delta || */
	     abs ( i_i_click_y - qme->y() ) > i_i_move_delta    ) {
	  i_b_mouse_moved = true;
	}
      }
    }

    if ( i_b_mouse_moved ) {
      //cerr << "Distance = " <<  (i_i_click_y - qme->y())/2 << "\n";
      i_i_diff = (i_i_click_y - qme->y())/1 ; // -<- Scaled distance
      i_i_click_y = qme->y();
      emit quickchange(i_i_diff);
    }
  }
}


#include "kmix-docking.moc"

