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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "viewswitches.h"

#include <QLayout>
#include <QWidget>

#include <kdebug.h>
#include <klocale.h>

#include "mdwswitch.h"
#include "mdwenum.h"
#include "mixer.h"

ViewSwitches::ViewSwitches(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof)
      : ViewBase(parent, name, mixer, 0, vflags, guiprof)
{
    // Create switch buttonGroup
    if ( _vflags & ViewBase::Vertical ) {
        _layoutMDW = new QVBoxLayout(this);
        _layoutSwitch = new QVBoxLayout();
        _layoutMDW->addItem( _layoutSwitch );
	_layoutEnum = new QVBoxLayout(); // always vertical!
        _layoutMDW->addItem( _layoutEnum );
    }
    else {
        _layoutMDW = new QHBoxLayout(this);
	_layoutSwitch = new QHBoxLayout();
        _layoutMDW->addItem( _layoutSwitch );
	// Place enums right from the switches: This is done, so that there will be no
	// ugly space on the left side, when no Switch is shown.
	// Actually it is not really clear yet, why there is empty space at all: There are 0 items in
	// the _layoutEnum, so it might be a sizeHint() or some other subtle layout issue.
	_layoutEnum = new QVBoxLayout();
        _layoutMDW->addItem( _layoutEnum );
    }
    init();
}

ViewSwitches::~ViewSwitches() {
}

void ViewSwitches::setMixSet(MixSet *mixset)
{
    for ( int i=0; i<mixset->count(); i++ )
    {
        MixDevice *md = (*mixset)[i];
        if (    md->captureVolume().hasSwitch()  && ! md->captureVolume().hasVolume()
             || md->playbackVolume().hasSwitch() && ! md->playbackVolume().hasVolume()
             || md->isEnum()
           )
        {
            _mixSet->append(md);
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
  MixDeviceWidget *mdw;

  if ( md->isEnum() ) {
     Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
     mdw = new MDWEnum(
		      _mixer,       // the mixer for this device
                      md,           // MixDevice (parameter)
                      orientation,  // Orientation
                      this,         // parent
                      this          // View widget
                      );
     _layoutEnum->addWidget(mdw);
  } // an enum
  else {
    // must be a switch
    Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
    mdw =
	new MDWSwitch(
		      _mixer,       // the mixer for this device
		      md,           // MixDevice (parameter)
		      false,        // Small
		      orientation,  // Orientation
		      this,         // parent
		      this          // View widget
		      );
        _layoutSwitch->addWidget(mdw);
    } // a switch

    return mdw;
}

QSize ViewSwitches::sizeHint() const {
    //kDebug(67100) << "ViewSwitches::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";
    return( _layoutMDW->sizeHint() );
}

void ViewSwitches::constructionFinished() {
    configurationUpdate();  // also does _layoutMDW->activate();
}

void ViewSwitches::refreshVolumeLevels() {
    //kDebug(67100) << "ViewSwitches::refreshVolumeLevels()\n";

    for ( int i=0; i<_mdws.count(); i++ ) {
        QWidget *mdw = _mdws[i];
	 if ( mdw == 0 ) {
	     kError(67100) << "ViewSwitches::refreshVolumeLevels(): mdw == 0\n";
	     break; // sanity check (normally the lists are set up correctly)
	 }
	 else {
	     if ( mdw->inherits("MDWSwitch")) {
		 //kDebug(67100) << "ViewSwitches::refreshVolumeLevels(): updating\n";
		 // a slider, fine. Lets update its value
		 static_cast<MDWSwitch*>(mdw)->update();
	     }
	     else if ( mdw->inherits("MDWEnum")) {
		static_cast<MDWEnum*>(mdw)->update();
             }
	     else {
		 kError(67100) << "ViewSwitches::refreshVolumeLevels(): mdw is not slider\n";
		 // no switch. Cannot happen in theory => skip it
		 // If I start putting other stuff in the switch tab, I will get a nice warning.
	     }
        }
    }
}


/**
   This implementation makes sure the BackgroundMode's are properly updated
   with their alternating colors after hiding/showing channels.
*/
void ViewSwitches::configurationUpdate() {
    bool backGoundModeToggler = true;
    for ( int i=0; i<_mdws.count(); i++ ) {
        QWidget *qw = _mdws[i];
	if ( qw->inherits("MDWSwitch")) {
	    MixDeviceWidget* mdw = static_cast<MDWSwitch*>(qw);
	    if ( ! mdw->isDisabled() ) {
		if ( backGoundModeToggler ) {
		    mdw->setBackgroundRole(QPalette::Background);
		}
		else {
		    // !! Should use KGlobalSettings::alternateBackgroundColor()
		    // or KGlobalSettings::calculateAlternateBackgroundColor() instead.
		    mdw->setBackgroundRole( QPalette::Base );
		}
		backGoundModeToggler = !backGoundModeToggler;
	    } // ! isDisabled()
	    else {
		//kDebug(67100) << "ViewSwitches::configurationUpdate() ignoring disabled switch\n";
	    }
	} // inherits("MDWSwitch")
    }
    _layoutMDW->activate();
}


#include "viewswitches.moc"

