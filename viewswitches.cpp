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

#include "viewswitches.h"

#include <qlayout.h>
#include <qwidget.h>

#include <kdebug.h>

#include "mdwswitch.h"
#include "mixer.h"

ViewSwitches::ViewSwitches(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags)
      : ViewBase(parent, name, mixer, 0, vflags)
{
    // Create switch buttonGroup
    _layoutMDW = new QHBoxLayout(this);
    init();
}

ViewSwitches::~ViewSwitches() {
}

void ViewSwitches::setMixSet(MixSet *mixset)
{
    MixDevice* md;
    for ( md = mixset->first(); md != 0; md = mixset->next() ) {
	if ( md->isSwitch()) {
	    _mixSet->append(md);
	}
	else {
	}
    }
}


int ViewSwitches::count()
{
    return ( _mixSet->count() );	
}

int ViewSwitches::advice() {
    if (  _mixSet->count() > 0 ) {
        // The Switch Views is always advised, if there are devices in it
        return 100;
    }
    else {
        return 0;
    }
}

QWidget* ViewSwitches::add(MixDevice *md)
{
    MDWSwitch *mdw =
	new MDWSwitch(
		      _mixer,       // the mixer for this device
		      md,           // MixDevice (parameter)
		      false,        // Small
		      Qt::Vertical, // Direction
		      this,         // parent
		      this,         // View widget
		      md->name().latin1()
		      );

    _layoutMDW->add(mdw);
    // !! later: allow the other direction as well
    return mdw;
}

QSize ViewSwitches::sizeHint() const {
    //kdDebug(67100) << "ViewSwitches::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";
    return( _layoutMDW->sizeHint() );
}

void ViewSwitches::constructionFinished() {
    configurationUpdate();  // also does _layoutMDW->activate();
}

void ViewSwitches::refreshVolumeLevels() {
    //kdDebug(67100) << "ViewSwitches::refreshVolumeLevels()\n";
     QWidget *mdw = _mdws.first();
     MixDevice* md;
     for ( md = _mixSet->first(); md != 0; md = _mixSet->next() ) {
	 if ( mdw == 0 ) {
	     kdError(67100) << "ViewSwitches::refreshVolumeLevels(): mdw == 0\n";
	     break; // sanity check (normally the lists are set up correctly)
	 }
	 else {
	     if ( mdw->inherits("MDWSwitch")) {
		 //kdDebug(67100) << "ViewSwitches::refreshVolumeLevels(): updating\n";
		 // a slider, fine. Lets update its value
		 static_cast<MDWSwitch*>(mdw)->update();
	     }
	     else {
		 kdError(67100) << "ViewSwitches::refreshVolumeLevels(): mdw is not slider\n";
		 // no switch. Cannot happen in theory => skip it
		 // If I start putting enums in the switch tab, I will get a nice warning.
	     }
	 }
	 mdw = _mdws.next();
    }
}


/**
   This implementation makes sure the BackgroundMode's are properly updated
   after hiding/showing channels.
*/
void ViewSwitches::configurationUpdate() {
    bool backGoundModeToggler = true;
    for (QWidget *qw = _mdws.first(); qw !=0; qw = _mdws.next() ) {
	if ( qw->inherits("MDWSwitch")) {
	    MixDeviceWidget* mdw = static_cast<MDWSwitch*>(qw);
	    if ( ! mdw->isDisabled() ) {
		if ( backGoundModeToggler ) {
		    mdw->setBackgroundMode(PaletteBackground);
		}
		else {
		    mdw->setBackgroundMode( PaletteBase );
		}
		backGoundModeToggler = !backGoundModeToggler;
	    } // ! isDisabled()
	    else {
		//kdDebug(67100) << "ViewSwitches::configurationUpdate() ignoring diabled switch\n";
	    }
	} // inherits("MDWSwitch")
    }
    _layoutMDW->activate();
}


#include "viewswitches.moc"

