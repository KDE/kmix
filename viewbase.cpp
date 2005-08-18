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

#include "viewbase.h"

// QT
#include <qlabel.h>
#include <qcursor.h>

// KDE
#include <kaction.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kiconloader.h>

// KMix
#include "dialogviewconfiguration.h"
#include "mixdevicewidget.h"
#include "mixer.h"


ViewBase::ViewBase(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, WFlags f, ViewBase::ViewFlags vflags)
     : QWidget(parent, name, f), _vflags(vflags), _caption(caption)
{
    _mixer = mixer;
    _mixSet = new MixSet();

    /* Can't use the following construct:
       setMixSet( & mixer->getMixSet());
       C++ does not use overloaded methods like getMixSet() as long as the constructor has not completed :-(((
    */
    _actions = new KActionCollection( this );

    // Plug in the "showMenubar" action, if the caller wants it. Typically this is only neccesary for views in the KMix main window.
    if ( vflags & ViewBase::HasMenuBar ) {
	KToggleAction *m = static_cast<KToggleAction*>(KStdAction::showMenubar( this, SLOT(toggleMenuBarSlot()), _actions ));
	if ( vflags & ViewBase::MenuBarVisible ) {
	    m->setChecked(true);
	}
	else {
	    m->setChecked(false);
	}
    }
    new KAction(i18n("&Channels"), 0, this, SLOT(configureView()), _actions, "toggle_channels");
    connect ( _mixer, SIGNAL(newVolumeLevels()), this, SLOT(refreshVolumeLevels()) );
}

ViewBase::~ViewBase() {
    delete _mixSet;
}

void ViewBase::init() {
    const MixSet& mixset = _mixer->getMixSet();
    setMixSet( const_cast<MixSet*>(&mixset)); // const_cast<>
}

void ViewBase::setMixSet(MixSet *)
{
   // do nothing. Subclasses can do something if they feel like it
}

/**
 * Dummy implementation for add().
 */
QWidget* ViewBase::add(MixDevice* mdw) {
    QWidget* label = new QLabel( mdw->name(), this, mdw->name().latin1());
    label->move(0, mdw->num()*12);
    return label;
}

void ViewBase::configurationUpdate() {
}

/**
 * Create all widgets.
 * This is a loop over all supported devices of the corresponding view.
 * On each device add() is called - the derived class must implement add() for creating and placing
 * the real MixDeviceWidget.
 * The added MixDeviceWidget is appended to the _mdws list.
 */
void ViewBase::createDeviceWidgets()
{
    // create devices
    MixDevice *mixDevice;
    for ( mixDevice = _mixSet->first(); mixDevice != 0; mixDevice = _mixSet->next())
    {
	QWidget* mdw = add(mixDevice);
	_mdws.append(mdw);
    }
    // allow view to "polish" itself
    constructionFinished();
}

// ---------- Popup stuff START ---------------------
void ViewBase::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      showContextMenu();
}

/**
 * Return a popup menu. This contains basic entries.
 * More can be added by the caller.
 */
KPopupMenu* ViewBase::getPopup()
{
   popupReset();
   return _popMenu;
}

void ViewBase::popupReset()
{
    KAction *a;

    _popMenu = new KPopupMenu( this );
    _popMenu->insertTitle( SmallIcon( "kmix" ), i18n("Device Settings") );

    a = _actions->action( "toggle_channels" );
    if ( a ) a->plug( _popMenu );

    a = _actions->action( "options_show_menubar" );
    if ( a ) a->plug( _popMenu );
}


/**
   This will only get executed, when the user has removed all items from the view.
   Don't remove this method, because then the user cannot get a menu for getting his
   channels back
*/
void ViewBase::showContextMenu()
{
    //kdDebug(67100) << "ViewBase::showContextMenu()" << endl;
    popupReset();

    QPoint pos = QCursor::pos();
    _popMenu->popup( pos );
}


void ViewBase::refreshVolumeLevels()
{
    // is virtual
}

Mixer* ViewBase::getMixer() {
    return _mixer;
}

/**
 * Open the View configuration dialog. The user can select which channels he wants
 * to see and which not.
 */
void ViewBase::configureView() {

    DialogViewConfiguration* dvc = new DialogViewConfiguration(0, *this);
    dvc->show();
    // !! The dialog is modal. Does it delete itself?
}

void ViewBase::toggleMenuBarSlot() {
    //kdDebug(67100) << "ViewBase::toggleMenuBarSlot() start\n";
    emit toggleMenuBar();
    //kdDebug(67100) << "ViewBase::toggleMenuBarSlot() done\n";
}

// ---------- Popup stuff END ---------------------

#include "viewbase.moc"
