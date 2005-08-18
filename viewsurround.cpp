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

#include "viewsurround.h"

// Qt
#include <qlabel.h>
#include <qlayout.h>
#include <qwidget.h>

// KDE
#include <kdebug.h>
#include <kiconloader.h>

// KMix
#include "kmixtoolbox.h"
#include "mdwslider.h"
#include "mixer.h"

/**
 * Demonstration verion of a "surround view"
 * Not really usable right now.
 */
ViewSurround::ViewSurround(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, ViewBase::ViewFlags vflags)
      : ViewBase(parent, name, caption, mixer, WStyle_Customize|WStyle_NoBorder, vflags)
{
    _mdSurroundFront = 0;
    _mdSurroundBack  = 0;
    _layoutMDW = new QHBoxLayout(this);
    _layoutMDW->setMargin(8);
    // Create switch buttonGroup
    if ( _vflags & ViewBase::Vertical ) {
        _layoutSliders = new QVBoxLayout(_layoutMDW);
    }
    else {
        _layoutSliders = new QHBoxLayout(_layoutMDW);
    }
    _layoutSurround = new QGridLayout(_layoutMDW,3,5);
    //    _layoutMDW->setMargin(8);
    init();
}

ViewSurround::~ViewSurround() {
}

void ViewSurround::setMixSet(MixSet *mixset)
{
    MixDevice* md;
    for ( md = mixset->first(); md != 0; md = mixset->next() ) {
	if ( ! md->isSwitch() ) {
	    switch ( md->type() ) {
	    case MixDevice::VOLUME:
	    case MixDevice::SURROUND:
	    case MixDevice::SURROUND_BACK:
	    case MixDevice::SURROUND_LFE:
	    case MixDevice::SURROUND_CENTERFRONT:
	    case MixDevice::SURROUND_CENTERBACK:
	    case MixDevice::AC97:
		_mixSet->append(md);
		break;
	    default:
		// we are not interested in other channels
		break;
	    } // switch(type)
	} // !is_switch()
    } // for
}

int ViewSurround::count()
{
    return ( _mixSet->count() );	
}

int ViewSurround::advice() {
    if (  _mixSet->count() > 0 ) {
        // The standard input and output views are always advised, if there are devices in it
        return 100;
    }
    else {
        return 0;
    }
}

QWidget* ViewSurround::add(MixDevice *md)
{
    bool small = false;
    Qt::Orientation orientation = Qt::Vertical;
    switch ( md->type() ) {
    case MixDevice::VOLUME:
	_mdSurroundFront = md;
	small = true;
	break;	
    case MixDevice::SURROUND_BACK:
	_mdSurroundBack = md;
	small = true;
	break;
    case MixDevice::SURROUND_LFE:
	orientation = Qt::Horizontal;
	small = true;
	break;
    case MixDevice::SURROUND_CENTERFRONT:
	orientation = Qt::Horizontal;
	small = true;
	break;
    case MixDevice::SURROUND_CENTERBACK:
	orientation = Qt::Horizontal;
	small = true;
	break;
	
    default:
	small       = false;
	// these are the sliders on the left side of the surround View
	orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
    } // switch(type)

    MixDeviceWidget *mdw = createMDW(md, small, orientation);

    switch ( md->type() ) {
    case MixDevice::VOLUME:
	_layoutSurround->addWidget(mdw ,0,0, Qt::AlignBottom | Qt::AlignLeft);
	break;
	
    case MixDevice::SURROUND_BACK:
	_layoutSurround->addWidget(mdw ,2,0, Qt::AlignTop | Qt::AlignLeft);
	break;
    case MixDevice::SURROUND_LFE:
	_layoutSurround->addWidget(mdw,1,3,  Qt::AlignVCenter | Qt::AlignRight ); break;
	break;
    case MixDevice::SURROUND_CENTERFRONT:
	_layoutSurround->addWidget(mdw,0,2,  Qt::AlignTop | Qt::AlignHCenter); break;
	break;
    case MixDevice::SURROUND_CENTERBACK:
	_layoutSurround->addWidget(mdw,2,2,  Qt::AlignBottom | Qt::AlignHCenter); break;
	break;

    case MixDevice::SURROUND:
    case MixDevice::AC97:
    default:
	// Add as slider to the layout on the left side
	_layoutSliders->add(mdw);
	break;
    } // switch(type)

    return mdw;
}

QSize ViewSurround::sizeHint() const {
    //    kdDebug(67100) << "ViewSurround::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";
    return( _layoutMDW->sizeHint() );
}

void ViewSurround::constructionFinished() {
    QLabel* personLabel = new QLabel("Listener", this);
    QPixmap icon = UserIcon( "Listener" );
    if ( ! icon.isNull()) personLabel->setPixmap(icon);
    personLabel->setLineWidth( 4 );
    personLabel->setMidLineWidth( 3 );
    personLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    int rowOfSpeaker = 0;
    if ( _mdSurroundBack != 0 ) {
       // let the speaker "sit" in the rear of the room, if there is
       // rear speaker support in this sound card
       rowOfSpeaker = 1;
    }
    _layoutSurround->addWidget(personLabel ,rowOfSpeaker, 2, Qt::AlignHCenter | Qt::AlignVCenter);

    if ( _mdSurroundFront != 0 ) {
	MixDeviceWidget *mdw = createMDW(_mdSurroundFront, true, Qt::Vertical);
	_layoutSurround->addWidget(mdw,0,4, Qt::AlignBottom | Qt::AlignRight);
	_mdws.append(mdw);

	QLabel* speakerIcon = new QLabel("Speaker", this);
        icon = UserIcon( "SpeakerFrontLeft" );
	if ( ! icon.isNull()) speakerIcon->setPixmap(icon);
        _layoutSurround->addWidget(speakerIcon,0,1, Qt::AlignTop | Qt::AlignLeft);

        speakerIcon = new QLabel("Speaker", this);
        icon = UserIcon( "SpeakerFrontRight" );
        if ( ! icon.isNull()) speakerIcon->setPixmap(icon);
	_layoutSurround->addWidget(speakerIcon,0,3, Qt::AlignTop | Qt::AlignRight);

    }

    if ( _mdSurroundBack != 0 ) {
	MixDeviceWidget *mdw = createMDW(_mdSurroundBack, true, Qt::Vertical);
	_layoutSurround->addWidget(mdw,2,4, Qt::AlignTop | Qt::AlignRight);
	_mdws.append(mdw);

        QLabel* speakerIcon = new QLabel("Speaker", this);
        icon = UserIcon( "SpeakerRearLeft" );
        if ( ! icon.isNull()) speakerIcon->setPixmap(icon);
        _layoutSurround->addWidget(speakerIcon,2,1, Qt::AlignBottom | Qt::AlignLeft);

        speakerIcon = new QLabel("Speaker", this);
        icon = UserIcon( "SpeakerRearRight" );
        if ( ! icon.isNull()) speakerIcon->setPixmap(icon);
        _layoutSurround->addWidget(speakerIcon,2,3, Qt::AlignBottom | Qt::AlignRight);


    }

    // !! just for the demo version
    KMixToolBox::setIcons (_mdws, true);
    KMixToolBox::setLabels(_mdws, true);
    KMixToolBox::setTicks (_mdws, true);

    _layoutMDW->activate();
}

void ViewSurround::refreshVolumeLevels() {
    //     kdDebug(67100) << "ViewSurround::refreshVolumeLevels()\n";

     QWidget *mdw = _mdws.first();
     MixDevice* md;
     for ( md = _mixSet->first(); md != 0; md = _mixSet->next() ) {
	 if ( mdw == 0 ) {
	     kdError(67100) << "ViewSurround::refreshVolumeLevels(): mdw == 0\n";
	     break; // sanity check (normally the lists are set up correctly)
	 }
	 else {
	     if ( mdw->inherits("MDWSlider")) {
		 //kdDebug(67100) << "ViewSurround::refreshVolumeLevels(): updating\n";
		 // a slider, fine. Lets update its value
		 static_cast<MDWSlider*>(mdw)->update();
	     }
	     else {
		 kdError(67100) << "ViewSurround::refreshVolumeLevels(): mdw is not slider\n";
		 // no slider. Cannot happen in theory => skip it
	     }
	 }
	 mdw = _mdws.next();
    }
}


MixDeviceWidget* ViewSurround::createMDW(MixDevice *md, bool small, Qt::Orientation orientation)
{
    MixDeviceWidget* mdw = new MDWSlider(
			    _mixer,       // the mixer for this device
			    md,           // MixDevice (parameter)
			    false,         // Show Mute LED
			    false,        // Show Record LED
			    small,        // Small
			    orientation,  // Orientation
			    this,         // parent
			    this,         // View widget
			    md->name().latin1()
			    );
    return mdw;
}

#include "viewsurround.moc"
