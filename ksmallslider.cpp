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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qwidget.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qbrush.h>

#include "ksmallslider.h"


KSmallSlider::KSmallSlider( QWidget *parent, const char *name )
    : QWidget( parent, name ), direction( KPanelApplet::Up )
{
    init();
}

KSmallSlider::KSmallSlider( KPanelApplet::Direction dir, QWidget *parent, const char *name )
    : QWidget( parent, name ), direction( dir )
{
    init();
}

KSmallSlider::KSmallSlider( int minValue, int maxValue, int pageStep,
                  int value, KPanelApplet::Direction dir,
                  QWidget *parent, const char *name )
    : QWidget( parent, name ),
      QRangeControl( minValue, maxValue, 1, pageStep, value ), direction( dir )
{
    init();
    sliderVal = value;
}

void KSmallSlider::init()
{
    sliderPos = 0;
    sliderVal = 0;
    state = Idle;
    track = TRUE;
    setFocusPolicy( TabFocus  );

    colHigh = QColor(0,255,0);
    colLow = QColor(255,0,0);
    colBack = QColor(0,0,0);

    grayHigh = QColor(255,255,255);
    grayLow = QColor(128,128,128);
    grayBack = QColor(0,0,0);
}

void KSmallSlider::setTracking( bool enable )
{
    track = enable;
}

int KSmallSlider::positionFromValue( int v ) const
{
    return QRangeControl::positionFromValue( v, available() );
}

int KSmallSlider::valueFromPosition( int p ) const
{
    return QRangeControl::valueFromPosition( p, available() );
}

void KSmallSlider::rangeChange()
{
    int newPos = positionFromValue( QRangeControl::value() );
    if ( newPos != sliderPos ) {
        reallyMoveSlider( newPos );
    }
}

void KSmallSlider::valueChange()
{
    if ( sliderVal != QRangeControl::value() ) {
        int newPos = positionFromValue( QRangeControl::value() );
        sliderVal = QRangeControl::value();
        reallyMoveSlider( newPos );
    }
    emit valueChanged(value());
}

void KSmallSlider::resizeEvent( QResizeEvent * )
{
    rangeChange();
    static int w, h;
    if ((w != width()) || (h != height())) {
        w = width(), h = height();
    }
}

//  usually returns a positive integer
int KSmallSlider::available() const
{
   return ((direction == KPanelApplet::Up) || (direction == KPanelApplet::Down)) ?
     height() - 2 : width() - 2;
}

void KSmallSlider::reallyMoveSlider( int newPos )
{
    sliderPos = newPos;
    repaint( FALSE );
}

void gradient( QPainter &p, bool hor, const QRect &rect, const QColor &ca, const QColor &cb, int /*ncols*/)
{
   int rDiff, gDiff, bDiff;
   int rca, gca, bca, rcb, gcb, bcb;

   register int x, y;

   if ((rect.width()<=0) || (rect.height()<=0)) return;

   rDiff = (rcb = cb.red())   - (rca = ca.red());
   gDiff = (gcb = cb.green()) - (gca = ca.green());
   bDiff = (bcb = cb.blue())  - (bca = ca.blue());

   register int rl = rca << 16;
   register int gl = gca << 16;
   register int bl = bca << 16;

   int rcdelta = ((1<<16) / ((!hor) ? rect.height() : rect.width())) * rDiff;
   int gcdelta = ((1<<16) / ((!hor) ? rect.height() : rect.width())) * gDiff;
   int bcdelta = ((1<<16) / ((!hor) ? rect.height() : rect.width())) * bDiff;

   // these for-loops could be merged, but the if's in the inner loop
   // would make it slow
   if (!hor)
   {
      for ( y = rect.top(); y <= rect.bottom(); y++ ) {
         rl += rcdelta;
         gl += gcdelta;
         bl += bcdelta;

         p.setPen(QColor(rl>>16, gl>>16, bl>>16));
         p.drawLine(rect.left(), y, rect.right(), y);
      }
   } else
   {
      for( x = rect.left(); x <= rect.right(); x++) {
         rl += rcdelta;
         gl += gcdelta;
         bl += bcdelta;

         p.setPen(QColor(rl>>16, gl>>16, bl>>16));
         p.drawLine(x, rect.top(), x, rect.bottom());
      }
   }
}

QColor interpolate( QColor low, QColor high, int percent ) {
    if ( percent<=0 ) return low; else
    if ( percent>=100 ) return high; else
    return QColor(
        low.red() + (high.red()-low.red()) * percent/100,
        low.green() + (high.green()-low.green()) * percent/100,
        low.blue() + (high.blue()-low.blue()) * percent/100 );
}

void KSmallSlider::paintEvent( QPaintEvent * )
{
   //  FIXME: The problem is that height() and width() respectively have wrong
   //         values when the panel is dragged between the top and the bottom
   //         of the screen or between the left and the right. It gets right
   //         again when when it is dragged in another way. The correct value
   //         is 43 (for the largest panel) and 31 (for the size below the
   //         largest). The wrong values are 19 and 89 respectively (for the
   //         largest panel). Note that these 4 numbers are prime numbers.
   QPainter p( this );

   // draw 3d border
   style().drawPanel ( &p, 0, 0, width(), height(), colorGroup(), TRUE );

   // drow lower/left part
   if ( width()>2 && height()>2 )
   {
      if ( direction == KPanelApplet::Up) {
         QRect outer = QRect( 1, sliderPos + 1, width() - 2, height() - 2 - sliderPos );

         if ( grayed )
             gradient( p, false, outer,
                       interpolate( grayHigh, grayLow, 100*sliderPos/(height()-2) ),
                       grayLow, 32 );
         else
             gradient( p, false, outer,
                       interpolate( colHigh, colLow, 100*sliderPos/(height()-2) ),
                       colLow, 32 );
      }
      else if (direction == KPanelApplet::Down ) {
         QRect outer = QRect( 1, 1, width() - 2, sliderPos - 1 );

         if ( grayed )
             gradient( p, false, outer,
                       interpolate( grayHigh, grayLow, 100*sliderPos/(height()-2) ),
                       grayHigh, 32 );
         else
             gradient( p, false, outer,
                       interpolate( colHigh, colLow, 100*sliderPos/(height()-2) ),
                       colHigh, 32 );
      }
      else if (direction == KPanelApplet::Right ) {
         QRect outer = QRect( 1, 1, sliderPos - 1, height() - 2 );

         if ( grayed )
             gradient( p, true, outer,
                       interpolate( grayHigh, grayLow, 100*sliderPos/(width()-2) ),
                       grayHigh, 32 );
         else
             gradient( p, true, outer,
                       interpolate( colHigh, colLow, 100*sliderPos/(width()-2) ),
                       colHigh, 32 );
      }
      else {
         QRect outer = QRect( sliderPos + 1, 1, width() - 2 - sliderPos, height() - 2 );

         if ( grayed )
             gradient( p, true, outer,
                       interpolate( grayHigh, grayLow, 100*sliderPos/(width()-2) ),
                       grayLow, 32 );
         else
             gradient( p, true, outer,
                       interpolate( colHigh, colLow, 100*sliderPos/(width()-2) ),
                       colLow, 32 );
      }

      // drow upper/right part
      QRect inner;
      if ( direction == KPanelApplet::Up )
         inner = QRect( 1, 1, width() - 2, sliderPos - 1 );
      else if ( direction == KPanelApplet::Down )
         inner = QRect( 1, sliderPos + 1, width() - 2, height() - 2 - sliderPos );
      else if ( direction == KPanelApplet::Right )
         inner = QRect( sliderPos + 1, 1, width() - 2 - sliderPos, height() - 2 );
      else
         inner = QRect( 1, 1, sliderPos - 1, height() - 2 );

      if ( grayed ) {
          p.setBrush( grayBack );
          p.setPen( grayBack );
      } else {
          p.setBrush( colBack );
          p.setPen( colBack );
      }
      p.drawRect( inner );
   }
}

void KSmallSlider::mousePressEvent( QMouseEvent *e )
{
   resetState();

   if ( e->button() == RightButton ) {
      return;
   }

   state = Dragging;
   emit sliderPressed();

   int pos = goodPart( e->pos() );
   moveSlider( pos );
}

void KSmallSlider::mouseMoveEvent( QMouseEvent *e )
{
    if ( state != Dragging )
        return;

    int pos = goodPart( e->pos() );
    moveSlider( pos );
}

void KSmallSlider::wheelEvent( QWheelEvent * e)
{
   static float offset = 0;
   static KSmallSlider* offset_owner = 0;
   if (offset_owner != this){
      offset_owner = this;
      offset = 0;
   }
   offset += -e->delta()*QMAX(pageStep(),lineStep())/120;
   if (QABS(offset)<1)
      return;
   QRangeControl::setValue( QRangeControl::value() + int(offset) );
   offset -= int(offset);
}

void KSmallSlider::mouseReleaseEvent( QMouseEvent * )
{
   resetState();
}

void KSmallSlider::moveSlider( int pos )
{
    int  a = available();
    int newPos = QMIN( a, QMAX( 0, pos ) );
    int newVal = valueFromPosition( newPos );
    if ( sliderVal != newVal ) {
        sliderVal = newVal;
        emit sliderMoved( sliderVal );
    }
    if ( tracking() && sliderVal != QRangeControl::value() ) {
        QRangeControl::directSetValue( sliderVal );
        emit valueChanged( value() ); //  Only for external use
    }

    if ( sliderPos != newPos )
        reallyMoveSlider( newPos );
}

void KSmallSlider::resetState()
{
    switch ( state ) {
    case Dragging: {
        QRangeControl::setValue( valueFromPosition( sliderPos ) );
        emit sliderReleased();
        break;
    }
    case Idle:
       break;

    default:
        qWarning("KSmallSlider: (%s) in wrong state", name( "unnamed" ) );
    }
    state = Idle;
}

void KSmallSlider::setValue( int value )
{
    if ( (direction == KPanelApplet::Right) || (direction == KPanelApplet::Down) )
        QRangeControl::setValue( value );
    else
        QRangeControl::setValue( QRangeControl::maxValue() - value );
}

void KSmallSlider::addStep()
{
    addPage();
}

void KSmallSlider::subtractStep()
{
    subtractPage();
}

int KSmallSlider::goodPart( const QPoint &p ) const
{
    return ((direction == KPanelApplet::Up) || (direction == KPanelApplet::Down)) ?
        p.y() - 1 : p.x() - 1;
}

QSize KSmallSlider::sizeHint() const
{ // FIXME Här har vi roten till det onda (89)!
    constPolish();
    const int length = 84;
    const int thick = 10;

    if ( (direction == KPanelApplet::Up) || (direction == KPanelApplet::Down) )
        return QSize( thick, length );
    else
        return QSize( length, thick );
}

QSize KSmallSlider::minimumSizeHint() const
{
    QSize s = sizeHint();
    s.setHeight( 8 );
    s.setWidth( 8 );
    return s;
}

QSizePolicy KSmallSlider::sizePolicy() const
{
    if ( (direction == KPanelApplet::Up) || (direction == KPanelApplet::Down) )
        return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Expanding );
    else
        return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}

int KSmallSlider::minValue() const
{
    return QRangeControl::minValue();
}

int KSmallSlider::maxValue() const
{
    return QRangeControl::maxValue();
}

int KSmallSlider::lineStep() const
{
    return QRangeControl::lineStep();
}

int KSmallSlider::pageStep() const
{
    return QRangeControl::pageStep();
}

void KSmallSlider::setLineStep( int i )
{
    setSteps( i, pageStep() );
}

void KSmallSlider::setPageStep( int i )
{
    setSteps( lineStep(), i );
}

//  Only for external acces. You MUST use QRangeControl::value() internally.
int KSmallSlider::value() const
{
    if ( (direction == KPanelApplet::Right) || (direction == KPanelApplet::Down) )
        return QRangeControl::value();
    else
        return QRangeControl::maxValue() - QRangeControl::value();
}

void KSmallSlider::setGray( bool value )
{
   if ( grayed!=value )
   {
      grayed = value;
      repaint();
   }
}

bool  KSmallSlider::gray() const
{
   return grayed;
}

void KSmallSlider::setColors( QColor high, QColor low, QColor back )
{
    colHigh = high;
    colLow = low;
    colBack = back;
    repaint();
}

void KSmallSlider::setGrayColors( QColor high, QColor low, QColor back )
{
    grayHigh = high;
    grayLow = low;
    grayBack = back;
    repaint();
}

#include "ksmallslider.moc"
