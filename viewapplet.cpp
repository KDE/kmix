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

#include "viewapplet.h"

// Qt
#include <qwidget.h>
#include <qlayout.h>

// KDE
#include <kdebug.h>

// KMix
#include "mdwslider.h"
#include "mixer.h"

ViewApplet::ViewApplet(QWidget* parent, const char* name, Mixer* mixer, KPanelApplet::Direction direction)
      : ViewBase(parent, name, mixer, WStyle_Customize|WStyle_NoBorder)
{
    _direction = direction;
    if ( _direction == KPanelApplet::Left || _direction == KPanelApplet::Right )
    {
	_layoutMDW = new QVBoxLayout( this );
    }
    else
    {
	_layoutMDW = new QHBoxLayout( this );
    }
    _layoutMDW->setResizeMode(QLayout::Fixed);
    init();
}

ViewApplet::~ViewApplet() {
}

void ViewApplet::setMixSet(MixSet *mixset)
{
    MixDevice* md;
    for ( md = mixset->first(); md != 0; md = mixset->next() ) {
	//	kdDebug(67100) << "ViewApplet::setMixSet() loop\n";
	if ( ! md->isSwitch() ) {
	    _mixSet->append(md);
	}
	//	kdDebug(67100) << "ViewApplet::setMixSet() loop size is now " << count() <<"\n";
    }
}

int ViewApplet::count()
{
    return ( _mixSet->count() );	
}

int ViewApplet::advice() {
    if (  _mixSet->count() > 0 ) {
        // The standard input and output views are always advised, if there are devices in it
        return 100;
    }
    else {
        return 0;
    }
}



QWidget* ViewApplet::add(MixDevice *md)
{
    //    kdDebug(67100) << "ViewApplet::add()\n";
    MixDeviceWidget *mdw =
	new MDWSlider(
			    _mixer,       // the mixer for this device
			    md,           // MixDevice (parameter)
			    false,        // Show Mute LED
			    false,        // Show Record LED
			    true,         // Small
			    _direction,   // Direction
			    this,         // parent
			    this,         // View widget
			    md->name().latin1()
			    );
    _layoutMDW->add(mdw);
    //QLayout::maximumSize
    return mdw;
}

QSize ViewApplet::sizeHint() {
    //kdDebug(67100) << "ViewApplet::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";
    return( _layoutMDW->sizeHint() );
}

void ViewApplet::constructionFinished() {
    _layoutMDW->activate();
}

void ViewApplet::resizeEvent(QResizeEvent *e)
{
    //kdDebug(67100) << "ViewApplet::resizeEvent(). SHOULD resize _layoutMDW to " << e->size() << endl;
}


void ViewApplet::refreshVolumeLevels() {
    //kdDebug(67100) << "ViewApplet::refreshVolumeLevels()\n";

     QWidget *mdw = _mdws.first();
     MixDevice* md;
     for ( md = _mixSet->first(); md != 0; md = _mixSet->next() ) {
	 if ( mdw == 0 ) {
	     kdError(67100) << "ViewApplet::refreshVolumeLevels(): mdw == 0\n";
	     break; // sanity check (normally the lists are set up correctly)
	 }
	 else {
	     if ( mdw->inherits("MDWSlider")) {
		 //kdDebug(67100) << "ViewApplet::refreshVolumeLevels(): updating\n";
		 // a slider, fine. Lets update its value
		 static_cast<MDWSlider*>(mdw)->update();
	     }
	     else {
		 kdError(67100) << "ViewApplet::refreshVolumeLevels(): mdw is not slider\n";
		 // no slider. Cannot happen in theory => skip it
	     }
	 }
	 mdw = _mdws.next();
    }
}

#include "viewapplet.moc"
