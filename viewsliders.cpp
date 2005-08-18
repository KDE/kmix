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

#include "viewsliders.h"

// Qt
#include <qlayout.h>
#include <qwidget.h>

// KDE
#include <kdebug.h>

// KMix
#include "mdwslider.h"
#include "mixer.h"

/**
 * Don't instanciate objects of this class directly. It won't work
 * correctly because init() does not get called.
 * See ViewInput and ViewOutput for "real" implementations.
 */
ViewSliders::ViewSliders(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, ViewBase::ViewFlags vflags)
      : ViewBase(parent, name, caption, mixer, WStyle_Customize|WStyle_NoBorder, vflags)
{
    if ( _vflags & ViewBase::Vertical ) {
        _layoutMDW = new QVBoxLayout(this);
    }
    else {
        _layoutMDW = new QHBoxLayout(this);
    }
    /*
     * Do not call init(). Call init() only for "end usage" classes.
     * Otherwise setMixSet() will be called multiple times.
     * Yes, this is rotten ... I will think of something smart later !!
     * Perhaps I can have a boolean "init-has-run" instance variable.
     */
    //init();
}

ViewSliders::~ViewSliders() {
}

void ViewSliders::setMixSet(MixSet *mixset)
{
    MixDevice* md;
    for ( md = mixset->first(); md != 0; md = mixset->next() ) {
	if ( (! md->isSwitch()) && ( ! md->isEnum() ) ) {
	    _mixSet->append(md);
	}
    }
}

int ViewSliders::count()
{
    return ( _mixSet->count() );	
}

int ViewSliders::advice() {
    if (  _mixSet->count() > 0 ) {
        // The standard input and output views are always advised, if there are devices in it
        return 100;
    }
    else {
        return 0;
    }
}

QWidget* ViewSliders::add(MixDevice *md)
{
    Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
    MixDeviceWidget *mdw =
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
    _layoutMDW->add(mdw);
    return mdw;
}

QSize ViewSliders::sizeHint() const {
    //    kdDebug(67100) << "ViewSliders::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";
    return( _layoutMDW->sizeHint() );
}

void ViewSliders::constructionFinished() {
    _layoutMDW->activate();
}

void ViewSliders::refreshVolumeLevels() {
    //     kdDebug(67100) << "ViewSliders::refreshVolumeLevels()\n";

     QWidget *mdw = _mdws.first();
     MixDevice* md;
     for ( md = _mixSet->first(); md != 0; md = _mixSet->next() ) {
	 if ( mdw == 0 ) {
	     kdError(67100) << "ViewSliders::refreshVolumeLevels(): mdw == 0\n";
	     break; // sanity check (normally the lists are set up correctly)
	 }
	 else {
	     if ( mdw->inherits("MDWSlider")) {
		 //kdDebug(67100) << "ViewSliders::refreshVolumeLevels(): updating\n";
		 // a slider, fine. Lets update its value
		 static_cast<MDWSlider*>(mdw)->update();
	     }
	     else {
		 kdError(67100) << "ViewSliders::refreshVolumeLevels(): mdw is not slider\n";
		 // no slider. Cannot happen in theory => skip it
	     }
	 }
	 mdw = _mdws.next();
    }
}


#include "viewsliders.moc"
