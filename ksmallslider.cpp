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

#include "ksmallslider.h"


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
    sliderVal = value;
}

void KSmallSlider::init()
{
    sliderPos = 0;
    sliderVal = 0;
    state = Idle;
    track = TRUE;
    grayed = false;
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
    //kdDebug(67100) << "KSmallSlider::valueChange() value=" << value() << "\n";
    emit valueChanged(value());
}

void KSmallSlider::resizeEvent( QResizeEvent * )
{
    //    rangeChange();
    static int w, h;
    if ((w != width()) || (h != height())) {
        w = width(), h = height();
/*
        kdDebug(67100)
            << "KSmallSlider::resizeEvent: width() = " << width()
            << ", height() = " << height() << endl;
*/
    }
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

void KSmallSlider::reallyMoveSlider( int newPos )
{
    sliderPos = newPos;
    update();
    //repaint( FALSE );
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

   // ------------------------ draw 3d border ---------------------------------------------
   style().drawPrimitive ( QStyle::PE_Panel, &p, QRect( 0, 0, width(), height() ), colorGroup(), TRUE );


   // ------------------------ draw lower/left part ----------------------------------------
   if ( width()>2 && height()>2 )
   {
       /*
       if ( _orientation == Qt::Vertical ) // !! was: KPanelApplet::Up
       {
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
       else
       */
       
       /*
       if ( _orientation == Qt::Vertical ) { // !! was: direction == KPanelApplet::Down ) {
         QRect outer = QRect( 1, 1, width() - 2, sliderPos );

         if ( grayed )
             gradient( p, false, outer, grayLow,
                       interpolate( grayLow, grayHigh, 100*sliderPos/(height()-2) ),
                       32 );
         else
             gradient( p, false, outer, colLow,
                       interpolate( colLow, colHigh, 100*sliderPos/(height()-2) ),
                       32 );
       }
       else
       */
       
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
      /*
      if ( direction == KPanelApplet::Up )
         inner = QRect( 1, 1, width() - 2, sliderPos );
      else if ( direction == KPanelApplet::Down )
         inner = QRect( 1, sliderPos + 1, width() - 2, height() - 2 - sliderPos );
      else if ( direction == KPanelApplet::Right )
         inner = QRect( sliderPos + 1, 1, width() - 2 - sliderPos, height() - 2 );
      else
         inner = QRect( 1, 1, sliderPos, height() - 2 );
      */
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
    return;


    /* the solution below is quite "interesting". If we did not "use up"
     * all "wheel delta" due to rounding,
     * we save some up in the "factor" variable for the next wheel event.
     * Unfortunately this means, that the increase or decrease is unregular.
     * There's a bugreport in bugs.kde.org about that.
     * So we throw that away and use trivial code now (see above).
     */
   static float offset = 0;
   static KSmallSlider* offset_owner = 0;
   if (offset_owner != this){
      offset_owner = this;
      offset = 0;
   }
   offset += -e->delta()*QMAX(pageStep(),lineStep())/120;
   if (QABS(offset)<1)
      return;

   /*
   // !!! is this good for?
   int factor = 1;
   if ( _orientation == Qt::Vertical ) {
      factor = -1;
   }
   */

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
    QRangeControl::setValue( value );
    /* !!! unclear semantics
    if ( (direction == KPanelApplet::Right) || (direction == KPanelApplet::Down) )
        QRangeControl::setValue( value );
    else
        QRangeControl::setValue( QRangeControl::maxValue() - value );
    */
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
	return p.x() -1;
    }
}

QSize KSmallSlider::sizeHint() const
{ // FIXME Här har vi roten till det onda (89)!
    //constPolish();
    const int length = 25;
    const int thick  = 10;

    if (  _orientation == Qt::Vertical )
        return QSize( thick, length );
    else
        return QSize( length, thick );
}


/***************** SIZE STUFF START ***************/
QSize KSmallSlider::minimumSizeHint() const
{
    QSize s(8,8);
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
    /** !!! @todo correct value
    if ( (direction == KPanelApplet::Right) || (direction == KPanelApplet::Down) )
        return QRangeControl::value();
    else
        return QRangeControl::maxValue() - QRangeControl::value();
    */
}

void KSmallSlider::paletteChange ( const QPalette &) {
    if ( grayed ) {
	const QColorGroup& qcg = palette().disabled();
	setColors(qcg.highlight(), qcg.dark(), qcg.background() );
    }
    else {
	const QColorGroup& qcg = palette().active();
	setColors(qcg.highlight(), qcg.dark(), qcg.background() );
    }
}

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
