//-*-C++-*-
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

#ifndef KSMALLSLIDER_H
#define KSMALLSLIDER_H

#include <kpanelapplet.h>

#include <qwidget.h>
#include <qpixmap.h>
#include <qrangecontrol.h>

class KSmallSlider : public QWidget, public QRangeControl
{
      Q_OBJECT

   public:
      KSmallSlider( QWidget *parent, const char *name=0 );
      KSmallSlider( Qt::Orientation, QWidget *parent, const char *name=0 );
      KSmallSlider( int minValue, int maxValue, int pageStep, int value,
		    Qt::Orientation, QWidget *parent, const char *name=0 );

    //virtual void setTracking( bool enable );
    //bool tracking() const;
      QSize sizeHint() const;
      QSizePolicy sizePolicy() const;
      QSize minimumSizeHint() const;

      int minValue() const;
      int maxValue() const;
      void setMinValue( int ); //  Don't use these unless you make versions
      void setMaxValue( int ); //  that work. -esigra
      int lineStep() const;
      int pageStep() const;
      void setLineStep( int );
      void setPageStep( int );
      int  value() const;

    //void paletteChange ( const QPalette & oldPalette );
      bool gray() const;

public slots:
    virtual void setValue( int );
    void addStep();
    void subtractStep();

      void setGray( bool value );
      void setColors( QColor high, QColor low, QColor back );
      void setGrayColors( QColor high, QColor low, QColor back );

      signals:
      void valueChanged( int value );
      void sliderPressed();
      void sliderMoved( int value );
      void sliderReleased();

   protected:
      void resizeEvent( QResizeEvent * );
      void paintEvent( QPaintEvent * );

      void mousePressEvent( QMouseEvent * );
      void mouseReleaseEvent( QMouseEvent * );
      void mouseMoveEvent( QMouseEvent * );
      void wheelEvent( QWheelEvent * );

      void valueChange();
      void rangeChange();

   private:
    //enum State { Idle, Dragging };

      void init();
      int positionFromValue( int ) const;
      int valueFromPosition( int ) const;
      void moveSlider( int );
    //void resetState();

    //      int slideLength() const;
      int available() const;
      int goodPart( const QPoint& ) const;
    //void initTicks();

    //QCOORD sliderPos;
    //int sliderVal;
    //State state;
    //bool track;
      bool grayed;
      Qt::Orientation _orientation;
      QColor colHigh, colLow, colBack;
      QColor grayHigh, grayLow, grayBack;

};

/*
inline bool KSmallSlider::tracking() const
{
    return track;
}
*/
#endif
