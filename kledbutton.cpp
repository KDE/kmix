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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

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
   if (e->button() == LeftButton)
   {
      toggle();
      emit stateChanged( state() );
   }
}

bool KLedButton::eventFilter( QObject* /*obj*/ , QEvent* /*ev*/ ) {
    // KLed and thus KLedButtung does not do proper positioning in QLayout's.
    // Thus I listen to my parents resize events and do it here ... OUCH, that's ugly
    /* No, this cannot work !
    if ( ev->type() == QEvent::Resize ) {
	QResizeEvent* qre = (QResizeEvent*)ev;
	this->move( qre->size().width()  - width()/2,
		    qre->size().height() - height()/2 );
    }
    */
    return false;
    //    KLed::eventFilter(obj, ev);

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
