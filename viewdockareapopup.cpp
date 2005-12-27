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

#include "viewdockareapopup.h"

// Qt
#include <qwidget.h>
#include <qevent.h>
#include <qlayout.h>
#include <qframe.h>
#include <qpushbutton.h>
#include <qdatetime.h>

// KDE
#include <kdebug.h>
#include <kaction.h>
#include <kapplication.h>
#include <klocale.h>

// KMix
#include "mdwslider.h"
#include "mixer.h"
#include "kmixdockwidget.h"

// !! Do NOT remove or mask out "WType_Popup"
//    Users will not be able to close the Popup without opening the KMix main window then.
//    See Bug #93443, #96332 and #96404 for further details. -- esken
ViewDockAreaPopup::ViewDockAreaPopup(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, KMixDockWidget *dockW )
      : ViewBase(parent, name, QString::null, mixer, WStyle_Customize | WType_Popup | Qt::WStyle_DialogBorder, vflags), _mdw(0), _dock(dockW)
{
    QBoxLayout *layout = new QHBoxLayout( this );
    _frame = new QFrame( this );
    layout->addWidget( _frame );

    _frame->setFrameStyle( QFrame::PopupPanel | QFrame::Raised );
    _frame->setLineWidth( 1 );

    _layoutMDW = new QGridLayout( _frame, 1, 1, 2, 1, "KmixPopupLayout" );
    _hideTimer = new QTime();
    init();
}

ViewDockAreaPopup::~ViewDockAreaPopup() {
}



void ViewDockAreaPopup::mousePressEvent(QMouseEvent *)
{
//	kdDebug() << "Teste pres mouse" << endl;
    /**
       Hide the popup:
       This should work automatically, when the user clicks outside the bounds of this popup:
       Alas - it does not work.
       Why it does not work, I do not know: this->isPopup() returns "true", so Qt should
       properly take care of it in QWidget.
    */
    if ( ! this->hasMouse() ) {
        _hideTimer->start();
        hide(); // needed!
    }
    return;
}

bool ViewDockAreaPopup::justHidden()
{
    return _hideTimer->elapsed() < 300;
}

void ViewDockAreaPopup::wheelEvent ( QWheelEvent * e ) {
   // Pass wheel event from "border widget" to child
   if ( _mdw != 0 ) {
      QApplication::sendEvent( _mdw, e);
   }
}

MixDevice* ViewDockAreaPopup::dockDevice()
{
    return _dockDevice;
}


void ViewDockAreaPopup::showContextMenu()
{
    // no right-button-menu on "dock area popup"
    return;
}
	

void ViewDockAreaPopup::setMixSet(MixSet *)
{
    //    kdDebug(67100) << "ViewDockAreaPopup::setMixSet()\n";
    // This implementation of setMixSet() is a bit "exotic". But I will leave it like this, until I implement
    // a configuration option for "what device to show on the dock area"
    _dockDevice = _mixer->masterDevice();
    if ( _dockDevice == 0 ) {
        // If we have no mixer device, we will take the first available mixer device
        _dockDevice = (*_mixer)[0];
    }
    _mixSet->append(_dockDevice);
}

QWidget* ViewDockAreaPopup::add(MixDevice *md)
{
    _mdw =
	new MDWSlider(
			    _mixer,       // the mixer for this device
			    md,		  // only 1 device. This is actually _dockDevice
			    true,         // Show Mute LED
			    false,        // Show Record LED
                            false,        // Small
			    Qt::Vertical, // Direction: only 1 device, so doesn't matter
			    _frame,       // parent
			    0,            // Is "NULL", so that there is no RMB-popup
			    _dockDevice->name().latin1() );
	 _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, 2 );
	 _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, 0 );
    _layoutMDW->addWidget( _mdw, 0, 1 );

	 // Add button to show main panel
	 _showPanelBox = new QPushButton( i18n("Mixer"), _frame, "MixerPanel" );
	 connect ( _showPanelBox, SIGNAL( clicked() ), SLOT( showPanelSlot() ) );
    _layoutMDW->addMultiCellWidget( _showPanelBox, 1, 1, 0, 2 );

    return _mdw;
}

int ViewDockAreaPopup::count()
{
    return ( _mixSet->count() );	
}

int ViewDockAreaPopup::advice() {
    if ( _dockDevice != 0 ) {
        // I could also evaluate whether we have a "sensible" device available.
        // For example
        // 100 : "master volume"
        // 100 : "PCM"
	// 50  : "CD"
        // 0   : all other devices
        return 100;
    }
    else {
        return 0;
    }
}

QSize ViewDockAreaPopup::sizeHint() const {
    //    kdDebug(67100) << "ViewDockAreaPopup::sizeHint(): NewSize is " << _mdw->sizeHint() << "\n";
    return( _mdw->sizeHint() );
}

void ViewDockAreaPopup::constructionFinished() {
    //    kdDebug(67100) << "ViewDockAreaPopup::constructionFinished()\n";

    _mdw->move(0,0);
    _mdw->show();
    _mdw->resize(_mdw->sizeHint() );
    resize(sizeHint());
	 
}


void ViewDockAreaPopup::refreshVolumeLevels() {
    //    kdDebug(67100) << "ViewDockAreaPopup::refreshVolumeLevels()\n";
    QWidget* mdw = _mdws.first();
    if ( mdw == 0 ) {
	kdError(67100) << "ViewDockAreaPopup::refreshVolumeLevels(): mdw == 0\n";
	// sanity check (normally the lists are set up correctly)
    }
    else {
	if ( mdw->inherits("MDWSlider")) {
	    static_cast<MDWSlider*>(mdw)->update();
	}
	else {
	    kdError(67100) << "ViewDockAreaPopup::refreshVolumeLevels(): mdw is not slider\n";
	    // no slider. Cannot happen in theory => skip it
	}
    }
}

void ViewDockAreaPopup::showPanelSlot() {
	_dock->toggleActive();
	_dock->_dockAreaPopup->hide();
}

#include "viewdockareapopup.moc"

