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

#include <kdebug.h>

#include <qwidget.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qbrush.h>
#include <qstyle.h>

#include "kglobalsettings.h"
#include "ksmallslider.h"

/*
static const QColor mutedHighColor2 = "#FFFFFF";
static const QColor mutedLowColor2 = "#808080";
static const QColor backColor2 = "#000000";
*/

KSmallSlider::KSmallSlider( QWidget *parent, const char *name )
    : QWidget( parent, name ),  _orientation(  Qt::Vertical )
{
    init();
}

KSmallSlider::KSmallSlider(  Qt::Orientation orientation, QWidget *parent, const char *name )
    : QWidget( parent, name ), _orientation( orientation )
{
    init();
}

KSmallSlider::KSmallSlider( int minValue, int maxValue, int pageStep,
                  int value, Qt::Orientation orientation,
                  QWidget *parent, const char *name )
    : QWidget( parent, name ),
      QRangeControl( minValue, maxValue, 1, pageStep, value ),  _orientation( orientation)
{
    init();
    //    sliderVal = value;
}

void KSmallSlider::init()
{
    // !! the following 2 values must be -1, to make sure the values are not the real values.
    // Otherwise some code below could determine that no change has happened and to send
    // no signals or to do no initial paint.
    //    sliderPos = -1;
    //    state = Idle;
    //track = TRUE;
    //setMouseTracking(true);
    grayed = false;
    setFocusPolicy( TabFocus  );

    colHigh = QColor(0,255,0);
    colLow = QColor(255,0,0);
    colBack = QColor(0,0,0);

    grayHigh = QColor(255,255,255);
    grayLow = QColor(128,128,128);
    grayBack = QColor(0,0,0);
}
/*
void KSmallSlider::setTracking( bool enable )
{
    track = enable;
}
*/
int KSmallSlider::positionFromValue( int v ) const
{
    return QRangeControl::positionFromValue( v, available() );
}

int KSmallSlider::valueFromPosition( int p ) const
{
    if ( _orientation == Qt::Vertical ) {
	// Coordiante System starts at TopLeft, but the slider values increase from Bottom to Top
	// Thus "revert" the position
	int avail = available();
	return QRangeControl::valueFromPosition( avail - p, avail );
    }
    else {
	// Horizontal everything is fine. Slider values match with Coordinate System
	return QRangeControl::valueFromPosition( p, available() );
    }
}

void KSmallSlider::rangeChange()
{
    /*
    int newPos = positionFromValue( QRangeControl::value() );
    if ( newPos != sliderPos ) {
	sliderPos = newPos;
    }
    */
    update();
}

void KSmallSlider::valueChange()
{
    //kdDebug(67100) << "KSmallSlider::valueChange() value=" << value() << "\n";
    update();
    emit valueChanged(value());
    /*
	if ( sliderVal != QRangeControl::value() ) {
        //int newPos = positionFromValue( QRangeControl::value() );
	//sliderPos = newPos;
        sliderVal = QRangeControl::value();
	update();
	emit valueChanged(value());
    }
    */
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
    if ( _orientation == Qt::Vertical) {
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

QColor interpolate( QColor low, QColor high, int percent ) {
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
//    kdDebug(67100) << "KSmallSlider::paintEvent: width() = " << width() << ", height() = " << height() << endl;
   QPainter p( this );

   int sliderPos = positionFromValue( QRangeControl::value() );

   // ------------------------ draw 3d border ---------------------------------------------
   style().drawPrimitive ( QStyle::PE_Panel, &p, QRect( 0, 0, width(), height() ), colorGroup(), TRUE );


   // ------------------------ draw lower/left part ----------------------------------------
   if ( width()>2 && height()>2 )
   {
       if (  _orientation == Qt::Horizontal ) {
         QRect outer = QRect( 1, 1, sliderPos, height() - 2 );
//	 kdDebug(67100) << "KSmallSlider::paintEvent: outer = " << outer << endl;

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
	 kdDebug(67100) << "KSmallSlider::paintEvent: sliderPos=" << sliderPos
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
      if ( _orientation == Qt::Vertical ) {
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

   if ( e->button() == RightButton ) {
      return;
   }

   //   state = Dragging;
   //emit sliderPressed();

   int pos = goodPart( e->pos() );
   moveSlider( pos );
}

void KSmallSlider::mouseMoveEvent( QMouseEvent *e )
{
    /*
    if ( state != Dragging )
        return;
    */
    int pos = goodPart( e->pos() );
    moveSlider( pos );
}

void KSmallSlider::wheelEvent( QWheelEvent * e)
{
//    kdDebug(67100) << "KSmallslider::wheelEvent()" << endl;
    /* Unfortunately KSmallSlider is no MixDeviceWidget, so we don't have access to
     * the MixDevice.
     */
    int inc = ( maxValue() - minValue() ) / 20;
    if ( inc < 1)
	inc = 1;

    //kdDebug(67100) << "KSmallslider::wheelEvent() inc=" << inc << "delta=" << e->delta() << endl;
    if ( e->delta() > 0 ) {
	QRangeControl::setValue( QRangeControl::value() + inc );
    }
    else {
	QRangeControl::setValue( QRangeControl::value() - inc );
    }
    e->accept(); // Accept the event

    // Hint: Qt autmatically triggers a valueChange() when we do setValue()
}

void KSmallSlider::mouseReleaseEvent( QMouseEvent * )
{
    //resetState();
}

/*
 * Moves slider to a dedicated position. If the value has changed
 */
void KSmallSlider::moveSlider( int pos )
{
    int  a = available();
    int newPos = QMIN( a, QMAX( 0, pos ) );  // keep it inside the available bounds of the slider
    int newVal = valueFromPosition( newPos );

    if ( newVal != QRangeControl::value() ) {
        //QRangeControl::directSetValue( sliderVal );
	QRangeControl::setValue( newVal );
        emit valueChanged( value() ); //  Only for external use
    }
    update();
}

/*
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
*/

void KSmallSlider::setValue( int value )
{
    QRangeControl::setValue( value );
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
    if ( _orientation == Qt::Vertical ) {
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

    if (  _orientation == Qt::Vertical )
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

    if ( _orientation == Qt::Vertical ) {
	//kdDebug(67100) << "KSmallSlider::sizePolicy() vertical value=(Fixed,MinimumExpanding)\n";
	return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Expanding );
    }
    else {
	//kdDebug(67100) << "KSmallSlider::sizePolicy() horizontal value=(MinimumExpanding,Fixed)\n";
        return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    }
}
/***************** SIZE STUFF END ***************/


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
    return QRangeControl::value();
}

/*
void KSmallSlider::paletteChange ( const QPalette &) {
    if ( grayed ) {
	setColors(mutedLowColor2, mutedHighColor2, backColor2 );
    }
    else {
	// ignore the QPalette and use the values from KGlobalSettings instead
	//const QColorGroup& qcg = palette().active();
	setColors(KGlobalSettings::baseColor(), KGlobalSettings::highlightColor(), backColor2 );
    }
}
*/

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
