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

#ifndef KLEDBUTTON_H
#define KLEDBUTTON_H

#include <qwidget.h>

#include <kled.h>

/**
  *@author Stefan Schimanski
  */

class KLedButton : public KLed  {
   Q_OBJECT
  public: 
   KLedButton(const QColor &col=Qt::green, QWidget *parent=0, const char *name=0);
   KLedButton(const QColor& col, KLed::State st, KLed::Look look, KLed::Shape shape,
	      QWidget *parent=0, const char *name=0);
   ~KLedButton();	

   QSize sizeHint () const;
   QSizePolicy sizePolicy () const;
  signals:
   void stateChanged( bool newState );

  protected:	
   void mousePressEvent ( QMouseEvent *e );

 private:
   bool eventFilter( QObject*, QEvent* );
};

#endif
