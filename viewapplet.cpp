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
#include <kactioncollection.h>
#include <kdebug.h>
#include <kpanelapplet.h>
#include <kstdaction.h>

// KMix
#include "mdwslider.h"
#include "mixer.h"

ViewApplet::ViewApplet(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, KPanelApplet::Position position )
    : ViewBase(parent, name, mixer, WStyle_Customize|WStyle_NoBorder, vflags) , _position(position)
{
    // remove the menu bar action, that is put by the "ViewBase" constructor in _actions.
    //KToggleAction *m = static_cast<KToggleAction*>(KStdAction::showMenubar( this, SLOT(toggleMenuBarSlot()), _actions ));
    _actions->remove( KStdAction::showMenubar(this, SLOT(toggleMenuBarSlot()), _actions) );

    if ( position == KPanelApplet::pLeft || position == KPanelApplet::pRight ) {
	_orientation = Qt::Vertical;
    }
    else {
	_orientation = Qt::Horizontal;
    }

    if ( _orientation == Qt::Horizontal ) {
	_layoutMDW = new QHBoxLayout( this );
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    }
    else {
	_layoutMDW = new QVBoxLayout( this );
	setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    }


    //_layoutMDW->setResizeMode(QLayout::Fixed);
    init();
}

ViewApplet::~ViewApplet() {
}

void ViewApplet::setMixSet(MixSet *mixset)
{
    MixDevice* md;
    for ( md = mixset->first(); md != 0; md = mixset->next() ) {
	if ( ! md->isSwitch() ) {
	    _mixSet->append(md);
	}
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
    /**
       Slider orientation is exactly the other way round. If the applet stretches horzontally,
       the sliders must be vertical
    */
    Qt::Orientation sliderOrientation;
    if (_orientation == Qt::Horizontal )
	sliderOrientation = Qt::Vertical;
    else
	sliderOrientation = Qt::Horizontal;
	
    //    kdDebug(67100) << "ViewApplet::add()\n";
    MixDeviceWidget *mdw =
	new MDWSlider(
			    _mixer,       // the mixer for this device
			    md,           // MixDevice (parameter)
			    false,        // Show Mute LED
			    false,        // Show Record LED
			    true,         // Small
			    sliderOrientation, // Orientation
			    this,         // parent
			    this,         // View widget
			    md->name().latin1()
			    );
    _layoutMDW->add(mdw);
    return mdw;
}

QSize ViewApplet::sizeHint() {
    // kdDebug(67100) << "ViewApplet::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";

    // Basically out main layout knows very good what the sizes should be
    QSize qsz = _layoutMDW->sizeHint();
    // But the panel is limited - thus constrain the size by the height() or width() of the panel
    if ( _orientation == Qt::Horizontal ) {
	qsz.setHeight( parentWidget()->height() );
    }
    else {
	qsz.setWidth ( parentWidget()->width() );
    }
    return qsz;
}

void ViewApplet::constructionFinished() {
    _layoutMDW->activate();
}

void ViewApplet::resizeEvent(QResizeEvent *qre)
{
    //    kdDebug(67100) << "ViewApplet::resizeEvent() size=" << qre->size() << "\n";
    // decide whether we have to show or hide all icons
    bool showIcons = false;
    if ( _orientation == Qt::Horizontal ) {
	if ( qre->size().height() > 50 ) {
	    showIcons = true;
	}
    }
    else {
       if ( qre->size().width() > 50 ) {
           showIcons = true;
       }
    }
    for ( QWidget *mdw = _mdws.first(); mdw != 0; mdw = _mdws.next() ) {
	if ( mdw == 0 ) {
	    kdError(67100) << "ViewApplet::resizeEvent(): mdw == 0\n";
	    break; // sanity check (normally the lists are set up correctly)
	}
	else {
	    if ( mdw->inherits("MDWSlider")) {
		static_cast<MDWSlider*>(mdw)->setIcons(showIcons);
		//static_cast<MDWSlider*>(mdw)->resize(qre->size());
	    }
	}
    }
    //    kdDebug(67100) << "ViewApplet::resizeEvent(). SHOULD resize _layoutMDW to " << qre->size() << endl;
    //QWidget::resizeEvent(qre);
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
