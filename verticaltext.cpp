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



VerticalText::VerticalText(QWidget * parent, const QString& text, Qt::WFlags f) : QWidget(parent,f),
                           m_height(8), m_width(20), cachedSizeValid(false)
{
   m_labelText = text;
}

VerticalText::~VerticalText() {
}


void VerticalText::paintEvent ( QPaintEvent * /*event*/ ) {
    QPainter paint(this);
    paint.rotate(270);
    paint.translate(0,-4); // Silly "solution" to make underlengths work
    
    
    if ( ! cachedSizeValid ) {
        QFontMetrics fontMetrics ( paint.font()  );
        m_width = fontMetrics.width(m_labelText);
        m_height = fontMetrics.height();
        resize(m_height, m_width+2);
        setFixedWidth(m_height);  // horizontal size policy fixed doesn't work for whatever reason => using setFixedWidth() fixes it
        updateGeometry();
        cachedSizeValid = true;
    }
    
   // Fix for bug 72520
   //-       paint.drawText(-height()+2,width(),name());
   //+       paint.drawText( -height()+2, width(), QString::fromUtf8(name()) );
   int posX =  -height();
   int posY = width();
   paint.drawText( posX, posY, m_labelText );
}

QSize VerticalText::sizeHint() const {
    return QSize( m_height, m_width+2);
    
}

QSizePolicy VerticalText::sizePolicy () const
{
    return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
}

