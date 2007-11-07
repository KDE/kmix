/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2003-2004 Christian Esken <esken@kde.org>
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

#include "verticaltext.h"
#include <QPainter>
#include <kdebug.h>



VerticalText::VerticalText(QWidget * parent, const QString& text, Qt::WFlags f) : QWidget(parent,f)
{
   m_labelText = text;

   QPainter paint(this);
   QFontMetrics fontMetrics ( paint.font()  );
   m_width = fontMetrics.width(m_labelText);
   m_height = fontMetrics.height();
   
   //resize(20,100 /*parent->height() */ );
   //setMinimumSize(20,10); // necessary for smooth integration into layouts (we only care for the widths).
}

VerticalText::~VerticalText() {
}


void VerticalText::paintEvent ( QPaintEvent * /*event*/ ) {
    QPainter paint(this);
    paint.rotate(270);
    paint.translate(0,-4); // Silly "solution" to make underlengths work
   //kDebug(67100) << "paintEvent(). height()=" <<  height() << "\n";

   // Fix for bug 72520
   //-       paint.drawText(-height()+2,width(),name());
   //+       paint.drawText( -height()+2, width(), QString::fromUtf8(name()) );
   int posX =  -height()+2;
   int posY = width();
   paint.drawText( posX, posY, m_labelText );
}

QSize VerticalText::sizeHint() const {
    kDebug(67100) << "QSize VerticalText::sizeHint() height=" << m_height << "width=" << m_width << "\n";
    return QSize( m_height, m_width+2);
    //return QSize(m_boundingRectangle.width(), m_boundingRectangle.height());
    
}

QSizePolicy VerticalText::sizePolicy () const
{
    return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
}

