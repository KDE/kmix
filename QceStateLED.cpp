/* This file is part of the KDE libraries
    Copyright (C) 1997 Richard Moore (moorer@cs.man.ac.uk)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
// CDE style LED lamp widget for Qt
// Richard J. Moore 23.11.96
// Email: moorer@cs.man.ac.uk

#include <stdio.h>
#include <QceStateLED.h>
#include <qpainter.h>
#include <qbrush.h>
#include <qpen.h>
#include <qcolor.h>

QceStateLED::QceStateLED(QWidget *parent) : QFrame(parent), dx( 4 )
{
  // Make sure we're in a sane state
  s= Off;

  // Set the frame style
  setFrameStyle(Sunken | Box);
  setGeometry(0,0,28,7);
}

void QceStateLED::setColor(QColor val_QColor_state)
{
  // Set new color internally and post a redraw request
  i_QColor_base = val_QColor_state;
  update();
}

void QceStateLED::drawContents(QPainter *painter)
{
  QBrush lightBrush(i_QColor_base);
  QBrush darkBrush(QColor(60,60,0));
  QPen pen(QColor(40,40,0));
  switch(s) {
  case On:
    painter->setBrush(i_QColor_base);
    painter->drawRect(1,1,QFrame::width()-2, QFrame::height()-2);
    break;
  case Off:
    painter->setBrush(i_QColor_base.dark(300));
    painter->drawRect(1,1,QFrame::width()-2, QFrame::height()-2);
    painter->setPen(i_QColor_base.dark(400));
    painter->drawLine(2,2,QFrame::width()-2, 2);
    painter->drawLine(2,QFrame::height()-2,QFrame::width()-2,QFrame::height()-2);
    // Draw verticals
    int i;
    for (i= 2; i < QFrame::width()-1; i+= dx)
      painter->drawLine(i,2,i,QFrame::height()-2);
    break;
  }
}

#include "QceStateLED.moc"

