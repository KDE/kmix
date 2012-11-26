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

#include "gui/ksmallslider.h"

// For INT_MAX
#include <limits.h>

#include <kdebug.h>

#include <QWidget>
#include <qpainter.h>
#include <QColor>
#include <qbrush.h>
#include <QMouseEvent>
#include <qstyle.h>
#include <QStyleOptionSlider>

#include "kglobalsettings.h"
#include "core/mixer.h"

KSmallSlider::KSmallSlider( int minValue, int maxValue, int pageStep,
                  int value, Qt::Orientation orientation,
                  QWidget *parent, const char * /*name*/ )
    : QAbstractSlider( parent )
{
    init();
    setOrientation(orientation);
    setRange(minValue, maxValue);
    setSingleStep(1);
    setPageStep(pageStep);
    setValue(value);
	 setTracking(true);
}

void KSmallSlider::init()
{
    grayed = false;
    setFocusPolicy( Qt::TabFocus  );

    colHigh = QColor(0,255,0);
    colLow = QColor(255,0,0);
    colBack = QColor(0,0,0);

    grayHigh = QColor(255,255,255);
    grayLow = QColor(128,128,128);
    grayBack = QColor(0,0,0);
}

int KSmallSlider::positionFromValue( int v ) const
{
    return positionFromValue( v, available() );
}

int KSmallSlider::valueFromPosition( int p ) const
{
    if ( orientation() == Qt::Vertical ) {
	// Coordiante System starts at TopLeft, but the slider values increase from Bottom to Top
	// Thus "revert" the position
	int avail = available();
	return valueFromPosition( avail - p, avail );
    }
    else {
	// Horizontal everything is fine. Slider values match with Coordinate System
	return valueFromPosition( p, available() );
    }
}

/*  postionFromValue() discontinued in in Qt4 => taken from Qt3 */
int KSmallSlider::positionFromValue( int logical_val, int span ) const
{
    if ( span <= 0 || logical_val < minimum() || maximum() <= minimum() )
        return 0;
    if ( logical_val > maximum() )
        return span;

    uint range = maximum() - minimum();
    uint p = logical_val - minimum();

    if ( range > (uint)INT_MAX/4096 ) {
        const int scale = 4096*2;
        return ( (p/scale) * span ) / (range/scale);
        // ### the above line is probably not 100% correct
        // ### but fixing it isn't worth the extreme pain...
    } else if ( range > (uint)span ) {
        return (2*p*span + range) / (2*range);
    } else {
        uint div = span / range;
        uint mod = span % range;
        return p*div + (2*p*mod + range) / (2*range);
    }
    //equiv. to (p*span)/range + 0.5
    // no overflow because of this implicit assumption:
    // span <= 4096
}

/* valueFromPositon() discontinued in in Qt4 => taken from Qt3 */
int KSmallSlider::valueFromPosition( int pos, int span ) const
{
    if ( span <= 0 || pos <= 0 )
        return minimum();
    if ( pos >= span )
        return maximum();

    uint range = maximum() - minimum();

    if ( (uint)span > range )
        return  minimum() + (2*pos*range + span) / (2*span);
    else {
        uint div = range / span;
        uint mod = range % span;
        return  minimum() + pos*div + (2*pos*mod + span) / (2*span);
    }
    // equiv. to minimum() + (pos*range)/span + 0.5
    // no overflow because of this implicit assumption:
    // pos <= span < sqrt(INT_MAX+0.0625)+0.25 ~ sqrt(INT_MAX)
}


void KSmallSlider::resizeEvent( QResizeEvent * )
{
    update();
    //QWidget::resizeEvent( ev );
}

//  Returns the really available space for the slider. If there is no space, 0 is returned;
int KSmallSlider::available() const
{
    int available = 0;
    if ( orientation() == Qt::Vertical) {
	available = height();
    }
    else {
	available = width();
    }
    if ( available > 1 ) {
	available -= 2;
    }
    else {
	available = 0;
    }
    return available;
}



namespace
{

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

QColor interpolate( const QColor& low, const QColor& high, int percent ) {
    if ( percent<=0 ) return low; else
    if ( percent>=100 ) return high; else
    return QColor(
        low.red() + (high.red()-low.red()) * percent/100,
        low.green() + (high.green()-low.green()) * percent/100,
        low.blue() + (high.blue()-low.blue()) * percent/100 );
}

}

void KSmallSlider::paintEvent( QPaintEvent * )
{
//    kDebug(67100) << "KSmallSlider::paintEvent: width() = " << width() << ", height() = " << height();
   QPainter p( this );

   int sliderPos = positionFromValue( QAbstractSlider::value() );

   // ------------------------ draw 3d border ---------------------------------------------
   QStyleOptionSlider option;
   option.init(this);
   style()->drawPrimitive ( QStyle::PE_Frame, &option, &p );


   // ------------------------ draw lower/left part ----------------------------------------
   if ( width()>2 && height()>2 )
   {
       if (  orientation() == Qt::Horizontal ) {
         QRect outer = QRect( 1, 1, sliderPos, height() - 2 );
//	 kDebug(67100) << "KSmallSlider::paintEvent: outer = " << outer;

         if ( grayed )
             gradient( p, true, outer, grayLow,
                       interpolate( grayLow, grayHigh, 100*sliderPos/(width()-2) ),
                       32 );
         else
             gradient( p, true, outer, colLow,
                       interpolate( colLow, colHigh, 100*sliderPos/(width()-2) ),
                       32 );
      }
      else {
         QRect outer = QRect( 1, height()-sliderPos-1, width() - 2, sliderPos-1 );
/*
	 kDebug(67100) << "KSmallSlider::paintEvent: sliderPos=" << sliderPos
			<< "height()=" << height()
			<< "width()=" << width()
			<< "outer = " << outer << endl;
*/
         if ( grayed )
             gradient( p, false, outer,
                       interpolate( grayLow, grayHigh, 100*sliderPos/(height()-2) ),
                       grayLow, 32 );
         else
             gradient( p, false, outer,
                       interpolate( colLow, colHigh, 100*sliderPos/(height()-2) ),
                       colLow, 32 );
      }

      // -------- draw upper/right part --------------------------------------------------
      QRect inner;
      if ( orientation() == Qt::Vertical ) {
	  inner = QRect( 1, 1, width() - 2, height() - 2 -sliderPos );
      }
      else {
	  inner = QRect( sliderPos + 1, 1, width() - 2 - sliderPos, height() - 2 );
      }
	
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
    //resetState();

   if ( e->button() == Qt::RightButton ) {
      return;
   }

   int pos = goodPart( e->pos() );
   moveSlider( pos );
}

void KSmallSlider::mouseMoveEvent( QMouseEvent *e )
{
    int pos = goodPart( e->pos() );
    moveSlider( pos );
}


void KSmallSlider::wheelEvent( QWheelEvent * qwe)
{
//    kDebug(67100) << "KSmallslider::wheelEvent()";
    int inc = ( maximum() - minimum() ) / Mixer::VOLUME_STEP_DIVISOR;
    if ( inc < 1)
	inc = 1;

    //kDebug(67100) << "KSmallslider::wheelEvent() inc=" << inc << "delta=" << e->delta();
	int newVal;

	bool increase = (qwe->delta() > 0);
	if (qwe->orientation() == Qt::Horizontal) // Reverse horizontal scroll: bko228780 
		increase = !increase;

    if ( increase ) {
       newVal = QAbstractSlider::value() + inc;
    }
    else {
       newVal = QAbstractSlider::value() - inc;
    }
    setValue( newVal );
    emit valueChanged(newVal);
    qwe->accept(); // Accept the event

    // Hint: Qt autmatically triggers a valueChange() when we do setValue()
}



/*
 * Moves slider to a dedicated position. If the value has changed
 */
void KSmallSlider::moveSlider( int pos )
{
    int  a = available();
    int newPos = qMin( a, qMax( 0, pos ) );  // keep it inside the available bounds of the slider
    int newVal = valueFromPosition( newPos );

    if ( newVal != value() ) {
        setValue( newVal );
	emit valueChanged(newVal);
        // probably done by Qt: emit valueChanged( value() ); //  Only for external use
        // probably we need update() here
    }
    update();
}


int KSmallSlider::goodPart( const QPoint &p ) const
{
    if ( orientation() == Qt::Vertical ) {
	return p.y() - 1;
    }
    else {
	return p.x() - 1;
    }
}

/***************** SIZE STUFF START ***************/
QSize KSmallSlider::sizeHint() const
{
    //constPolish();
    const int length = 25;
    const int thick  = 10;

    if (  orientation() == Qt::Vertical )
        return QSize( thick, length );
    else
        return QSize( length, thick );
}


QSize KSmallSlider::minimumSizeHint() const
{
    QSize s(10,10);
    return s;
}


QSizePolicy KSmallSlider::sizePolicy() const
{

    if ( orientation() == Qt::Vertical ) {
	//kDebug(67100) << "KSmallSlider::sizePolicy() vertical value=(Fixed,MinimumExpanding)\n";
	return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Expanding );
    }
    else {
	//kDebug(67100) << "KSmallSlider::sizePolicy() horizontal value=(MinimumExpanding,Fixed)\n";
        return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    }
}
/***************** SIZE STUFF END ***************/



void KSmallSlider::setGray( bool value )
{
   if ( grayed!=value )
   {
      grayed = value;
      update();
      //repaint();
   }
}

bool KSmallSlider::gray() const
{
   return grayed;
}

void KSmallSlider::setColors( QColor high, QColor low, QColor back )
{
    colHigh = high;
    colLow = low;
    colBack = back;
    update();
    //repaint();
}

void KSmallSlider::setGrayColors( QColor high, QColor low, QColor back )
{
    grayHigh = high;
    grayLow = low;
    grayBack = back;
    update();
    //repaint();
}

#include "ksmallslider.moc"
