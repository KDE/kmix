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
 * Software Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QMouseEvent>
#include <qsizepolicy.h>

#include "kledbutton.h"


KLedButton::KLedButton(const QColor &col, QWidget *parent, const char *name)
   : KLed( col, parent, name )
{	
    // KLed and thus KLedButtung does not do proper positioning in QLayout's.
    // Thus I will do a dirty trick here
    installEventFilter(parent);
}

KLedButton::KLedButton(const QColor& col, KLed::State st, KLed::Look look,
		       KLed::Shape shape, QWidget *parent, const char *name)
   : KLed( col, st, look, shape, parent, name )
{
}

KLedButton::~KLedButton()
{
}

void KLedButton::mousePressEvent( QMouseEvent *e )
{
   if (e->button() == Qt::LeftButton)
   {
      toggle();
      emit stateChanged( state() );
   }
}


bool KLedButton::eventFilter( QObject* /*obj*/ , QEvent* /*ev*/ ) { // !! Obsolete
    return false;
}	

QSize KLedButton::sizeHint() const
{
    return size();
}

QSizePolicy KLedButton::sizePolicy () const
{
    return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

#include "kledbutton.moc"
