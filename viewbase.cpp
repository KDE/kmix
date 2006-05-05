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

#include "viewbase.h"

// QT
#include <qcursor.h>
#include <qlabel.h>
#include <QMouseEvent>

// KDE
#include <kaction.h>
#include <kmenu.h>
#include <klocale.h>
#include <kiconloader.h>

// KMix
#include "dialogviewconfiguration.h"
#include "mixdevicewidget.h"
#include "mixer.h"


ViewBase::ViewBase(QWidget* parent, const char* name, Mixer* mixer, Qt::WFlags f, ViewBase::ViewFlags vflags, GUIProfile *guiprof)
    : QWidget(parent, name, f), _vflags(vflags), _guiprof(guiprof)
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
    KAction *action = new KAction(i18n("&Channels"), _actions, "toggle_channels");
    connect(action, SIGNAL(triggered(bool) ), SLOT(configureView()));
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
    QWidget* label = new QLabel( mdw->name(), this );
    label->setObjectName( mdw->name() );
    label->move(0, _dummyImplPos*12);
    ++_dummyImplPos;
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
    for ( int i=0; i<_mixSet->count(); i++ )
    {
        MixDevice *mixDevice;
        mixDevice = (*_mixSet)[i];
	QWidget* mdw = add(mixDevice);
	_mdws.append(mdw);
    }
    // allow view to "polish" itself
    constructionFinished();
}

// ---------- Popup stuff START ---------------------
void ViewBase::mousePressEvent( QMouseEvent *e )
{
   if ( e->button() == Qt::RightButton )
      showContextMenu();
}

/**
 * Return a popup menu. This contains basic entries.
 * More can be added by the caller.
 */
KMenu* ViewBase::getPopup()
{
   popupReset();
   return _popMenu;
}

void ViewBase::popupReset()
{
    KAction *a;

    _popMenu = new KMenu( this );
    _popMenu->addTitle( SmallIcon( "kmix" ), i18n("Device Settings") );

    a = _actions->action( "toggle_channels" );
    if ( a ) _popMenu->addAction(a);

    a = _actions->action( "options_show_menubar" );
    if ( a ) _popMenu->addAction(a);
}


/**
   This will only get executed, when the user has removed all items from the view.
   Don't remove this method, because then the user cannot get a menu for getting his
   channels back
*/
void ViewBase::showContextMenu()
{
    //kDebug(67100) << "ViewBase::showContextMenu()" << endl;
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
    //kDebug(67100) << "ViewBase::toggleMenuBarSlot() start\n";
    emit toggleMenuBar();
    //kDebug(67100) << "ViewBase::toggleMenuBarSlot() done\n";
}

// ---------- Popup stuff END ---------------------

#include "viewbase.moc"
