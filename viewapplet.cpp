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
 * This program is distributed in the hope that it will be useful, * but WITHOUT ANY WARRANTY; without even the implied warranty of
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
#include <qsize.h>

// KDE
#include <kactioncollection.h>
#include <kdebug.h>
#include <kpanelapplet.h>
#include <kstdaction.h>

// KMix
#include "kmixtoolbox.h"
#include "mdwslider.h"
#include "mixer.h"

ViewApplet::ViewApplet(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, KPanelApplet::Position position )
    : ViewBase(parent, name, QString::null, mixer, WStyle_Customize|WStyle_NoBorder, vflags)
{
    setBackgroundOrigin(AncestorOrigin);
    // remove the menu bar action, that is put by the "ViewBase" constructor in _actions.
    //KToggleAction *m = static_cast<KToggleAction*>(KStdAction::showMenubar( this, SLOT(toggleMenuBarSlot()), _actions ));
    _actions->remove( KStdAction::showMenubar(this, SLOT(toggleMenuBarSlot()), _actions) );


    if ( position == KPanelApplet::pLeft || position == KPanelApplet::pRight ) {
      //kdDebug(67100) << "ViewApplet() isVertical" << "\n";
      _viewOrientation = Qt::Vertical;
    }
     else {
      //kdDebug(67100) << "ViewApplet() isHorizontal" << "\n";
      _viewOrientation = Qt::Horizontal;
    }

    if ( _viewOrientation == Qt::Horizontal ) {
	_layoutMDW = new QHBoxLayout( this );
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    }
    else {
	_layoutMDW = new QVBoxLayout( this );
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
	if ( (! md->isSwitch()) && ( ! md->isEnum() ) ) {
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
    if (_viewOrientation == Qt::Horizontal )
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
    mdw->setBackgroundOrigin(AncestorOrigin);
    
    static_cast<MDWSlider*>(mdw)->setValueStyle(MixDeviceWidget::NNONE);
    static_cast<MDWSlider*>(mdw)->setIcons(shouldShowIcons( size()) ); // !!! This should use the panel size
    _layoutMDW->add(mdw);
    return mdw;
}

void ViewApplet::constructionFinished() {
    _layoutMDW->activate();
    
    KMixToolBox::setIcons     ( _mdws, shouldShowIcons( size()) ); // !!! This should use the panel size
    KMixToolBox::setValueStyle( _mdws, MixDeviceWidget::NNONE);
}


QSize ViewApplet::sizeHint() const {
    // Basically out main layout knows very good what the sizes should be
    QSize qsz = _layoutMDW->sizeHint();
    //kdDebug(67100) << "ViewApplet::sizeHint(): NewSize is " << qsz << "\n";
    return qsz;
}

QSizePolicy ViewApplet::sizePolicy() const {
    if ( _viewOrientation == Qt::Horizontal ) {
	//kdDebug(67100) << "ViewApplet::sizePolicy=(Fixed,Expanding)\n";
	return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    else {
	//kdDebug(67100) << "ViewApplet::sizePolicy=(Expanding,Fixed)\n";
	return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
}

bool ViewApplet::shouldShowIcons(QSize qsz) {
    bool showIcons = false;
    if ( _viewOrientation == Qt::Horizontal ) {
        if ( qsz.height() >= 32 ) {
            //kdDebug(67100) << "ViewApplet::resizeEvent() hor >=32" << qre->size() << "\n";
            showIcons = true;
        }
    }
    else {
       if ( qsz.width() >= 32 ) {
           //kdDebug(67100) << "ViewApplet::resizeEvent() vert >=32" << qre->size() << "\n";
           showIcons = true;
       }
    }
    return showIcons;
}

void ViewApplet::resizeEvent(QResizeEvent *qre)
{
    //kdDebug(67100) << "ViewApplet::resizeEvent() size=" << qre->size() << "\n";
    // decide whether we have to show or hide all icons
    bool showIcons = shouldShowIcons(qre->size());

    for ( QWidget *mdw = _mdws.first(); mdw != 0; mdw = _mdws.next() ) {
	if ( mdw == 0 ) {
	    kdError(67100) << "ViewApplet::resizeEvent(): mdw == 0\n";
	    break; // sanity check (normally the lists are set up correctly)
	}
	else {
	    if ( mdw->inherits("MDWSlider")) {
		static_cast<MDWSlider*>(mdw)->setIcons(showIcons);
		static_cast<MDWSlider*>(mdw)->setValueStyle(MixDeviceWidget::NNONE);
		//static_cast<MDWSlider*>(mdw)->resize(qre->size());
	    }
	}
    }
    //    kdDebug(67100) << "ViewApplet::resizeEvent(). SHOULD resize _layoutMDW to " << qre->size() << endl;
    //QWidget::resizeEvent(qre);

    // resizing changes our own sizeHint(), because we must take the new PanelSize in account.
    // So updateGeometry() is amust for us.
    updateGeometry();
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

void ViewApplet::configurationUpdate() {
    updateGeometry();
    //_layoutMDW->activate();
    constructionFinished(); // contains "_layoutMDW->activate();"
    emit appletContentChanged();
    kdDebug(67100) << "ViewApplet::configurationUpdate()" << endl;
    // the following "emit" is only here to be picked up by KMixApplet, because it has to
    // - make sure the panel is informed about the size change
    // - save the new configuration
    //emit configurationUpdated();
}

#include "viewapplet.moc"
