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
}

VerticalText::~VerticalText() {
}

void VerticalText::setText(const QString& text) {
    if (m_labelText != text) {
        m_labelText = text;
        updateGeometry();
    }
}


void VerticalText::paintEvent ( QPaintEvent * /*event*/ ) {
    QPainter paint(this);
    paint.rotate(270);
//    paint.translate(0,-4); // Silly "solution" to make underlengths work

    // Fix for bug 72520
   //-       paint.drawText(-height()+2,width(),name());
   //+       paint.drawText( -height()+2, width(), QString::fromUtf8(name()) );
   int posX =  -height();
   int posY = width();
   paint.drawText( posX, posY, m_labelText );
}

QSize VerticalText::sizeHint() const {
    const QFontMetrics& fontMetr = fontMetrics();
    QSize textSize(fontMetr.width(m_labelText), fontMetr.height());
    textSize.transpose();
    return textSize;
}

QSize VerticalText::minimumSizeHint() const
{
    const QFontMetrics& fontMetr = fontMetrics();
    QSize textSize(fontMetr.width("MMMM"), fontMetr.height());
    textSize.transpose();
    return textSize;
}

QSizePolicy VerticalText::sizePolicy () const
{
    return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
}

