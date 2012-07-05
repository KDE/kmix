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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KSMALLSLIDER_H
#define KSMALLSLIDER_H

#include <qabstractslider.h>
#include <qpixmap.h>

#include "core/volume.h"
#include "volumesliderextradata.h"

class KSmallSlider : public QAbstractSlider
{
      Q_OBJECT

   public:
      KSmallSlider( int minValue, int maxValue, int pageStep, int value,
        Qt::Orientation, QWidget *parent, const char *name=0 );

/*      void setChid(Volume::ChannelID chid) { this->chid = chid; };
      Volume::ChannelID getChid() { return chid; };*/
      
      QSize sizeHint() const;
      QSizePolicy sizePolicy() const;
      QSize minimumSizeHint() const;

      bool gray() const;

      VolumeSliderExtraData extraData;
      
public slots:
      void setGray( bool value );
      void setColors( QColor high, QColor low, QColor back );
      void setGrayColors( QColor high, QColor low, QColor back );

      signals:
      void valueChanged( int value );

   protected:
      void resizeEvent( QResizeEvent * );
      void paintEvent( QPaintEvent * );

      void mousePressEvent( QMouseEvent * );
      void mouseMoveEvent( QMouseEvent * );
      void wheelEvent( QWheelEvent * );

      void valueChange();

   private:

      void init();
      int positionFromValue( int ) const;
      int valueFromPosition( int ) const;
      int positionFromValue( int logical_val, int span ) const;
      int valueFromPosition( int pos, int span ) const;
      void moveSlider( int );

      int available() const;
      int goodPart( const QPoint& ) const;

      bool grayed;
      QColor colHigh, colLow, colBack;
      QColor grayHigh, grayLow, grayBack;

//      Volume::ChannelID chid;
};

#endif
