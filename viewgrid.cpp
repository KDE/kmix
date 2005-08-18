/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include "viewgrid.h"

// Qt
#include <qwidget.h>

// KDE
#include <kdebug.h>

// KMix
#include "mdwenum.h"
#include "mdwslider.h"
#include "mdwswitch.h"
#include "mixer.h"

/**
 */
ViewGrid::ViewGrid(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, ViewBase::ViewFlags vflags)
      : ViewBase(parent, name, caption, mixer, WStyle_Customize|WStyle_NoBorder, vflags)
{
   m_spacingHorizontal = 5;
   m_spacingVertical = 5;
   
   if ( _vflags & ViewBase::Vertical ) {
        //_layoutMDW = new QVBoxLayout(this);
   }
   else {
        //_layoutMDW = new QHBoxLayout(this);
   }
   init();
}

ViewGrid::~ViewGrid() {
}

void ViewGrid::setMixSet(MixSet *mixset)
{
    MixDevice* md;
    int testCounter = 0;
    for ( md = mixset->first(); md != 0; md = mixset->next() ) {
       if (testCounter<8) {
	    _mixSet->append(md);
	}
	testCounter++;
    }
}

int ViewGrid::count()
{
    return ( _mixSet->count() );	
}

int ViewGrid::advice() {
    if (  _mixSet->count() > 0 ) {
        // The standard input and output views are always advised, if there are devices in it
        return 100;
    }
    else {
        return 0;
    }
}

QWidget* ViewGrid::add(MixDevice *md)
{
  MixDeviceWidget *mdw = 0;
  if ( md->isEnum() ) {
    Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
    mdw = new MDWEnum(
	_mixer,       // the mixer for this device
    md,           // MixDevice (parameter)
    orientation,  // Orientation
    this,         // parent
    this,         // View widget
    md->name().latin1()
		     );
  } // an enum
  else if (md->isSwitch()) {
    Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
    mdw =
	new MDWSwitch(
	_mixer,       // the mixer for this device
    md,           // MixDevice (parameter)
    false,        // Small
    orientation,  // Orientation
    this,         // parent
    this,         // View widget
    md->name().latin1()
		     );
  } // a switch

  else { // must be a slider
    Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
    mdw =
	new MDWSlider(
			    _mixer,       // the mixer for this device
			    md,           // MixDevice (parameter)
			    true,         // Show Mute LED
			    true,         // Show Record LED
			    false,        // Small
			    orientation,  // Orientation
			    this,         // parent
			    this,         // View widget
			    md->name().latin1()
			    );
  }
  return mdw;
}

QSize ViewGrid::sizeHint() const {
    //    kdDebug(67100) << "ViewGrid::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";
    return( m_sizeHint);
}

void ViewGrid::constructionFinished() {
    //_layoutMDW->activate();

    // do a manual layout
    configurationUpdate();
}

void ViewGrid::refreshVolumeLevels() {
    //     kdDebug(67100) << "ViewGrid::refreshVolumeLevels()\n";

   m_sizeHint.setWidth (0);
   m_sizeHint.setHeight(0);

   m_testingX = 0;
   m_testingY = 0;

     QWidget *mdw = _mdws.first();
     MixDevice* md;
     for ( md = _mixSet->first(); md != 0; md = _mixSet->next() ) {
	 if ( mdw == 0 ) {
	     kdError(67100) << "ViewGrid::refreshVolumeLevels(): mdw == 0\n";
	     break; // sanity check (normally the lists are set up correctly)
	 }
	 else {
	     if ( mdw->inherits("MDWSlider")) {
		 //kdDebug(67100) << "ViewGrid::refreshVolumeLevels(): updating\n";
		 // a slider, fine. Lets update its value
		 static_cast<MDWSlider*>(mdw)->update();
	     }
	     else if ( mdw->inherits("MDWSwitch")) {
                 //kdDebug(67100) << "ViewGrid::refreshVolumeLevels(): updating\n";
                 // a slider, fine. Lets update its value
	       static_cast<MDWSwitch*>(mdw)->update();
	     }
	     else if ( mdw->inherits("MDWEnum")) {
	       static_cast<MDWEnum*>(mdw)->update();
	     }
	     else {
	       kdError(67100) << "ViewGrid::refreshVolumeLevels(): mdw is unknown/unsupported type\n";
		 // no slider. Cannot happen in theory => skip it
	     }
	 }
	 mdw = _mdws.next();
    }
}

/**
   This implementation makes sure the Grid's geometry is updated
   after hiding/showing channels.
*/
void ViewGrid::configurationUpdate() {
    m_sizeHint.setWidth (0);
    m_sizeHint.setHeight(0);

    m_testingX = 0;
    m_testingY = 0;

    for (QWidget *qw = _mdws.first(); qw !=0; qw = _mdws.next() ) {

    if ( qw->inherits("MixDeviceWidget")) {
      MixDeviceWidget* mdw = static_cast<MixDeviceWidget*>(qw);
      int xPos = m_testingX * m_spacingHorizontal;
      int yPos = m_testingY * m_spacingVertical ;
      mdw->move( xPos, yPos );
      mdw->resize( mdw->sizeHint() );
      int xMax = xPos + mdw->width() ; if ( xMax > m_sizeHint.width()  ) m_sizeHint.setWidth(xMax);
      int yMax = yPos + mdw->height(); if ( yMax > m_sizeHint.height() ) m_sizeHint.setHeight(yMax);

      m_testingX += 5;
      if ( m_testingX > 50 ) {
        m_testingY += 10;
        m_testingX = 0;
      }
    } // inherits MixDeviceWidget
  } // for all MDW's
}


#include "viewgrid.moc"
