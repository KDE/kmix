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
//  * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "viewbase.h"

// QT
#include <qcursor.h>
#include <QMouseEvent>

// KDE
#include <kaction.h>
#include <kmenu.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
// KMix
#include "dialogviewconfiguration.h"
#include "guiprofile.h"
#include "kmixtoolbox.h"
#include "mixdevicewidget.h"
#include "mixer.h"


ViewBase::ViewBase(QWidget* parent, const char* id, Mixer* mixer, Qt::WFlags f, ViewBase::ViewFlags vflags, GUIProfile *guiprof, KActionCollection *actionColletion)
    : QWidget(parent, f), _actions(actionColletion), _vflags(vflags), _guiprof(guiprof)
{
   setObjectName(id);
   m_viewId = id;
   _mixer = mixer;
   _mixSet = new MixSet();

   if ( _actions == 0 ) {
      // We create our own action collection, if the actionColletion was 0.
      // This is currently done for the ViewDockAreaPopup, but only because it has not been converted to use the app-wide
      // actionCollection(). This is a @todo.
      _actions = new KActionCollection( this );
   }
   _localActionColletion = new KActionCollection( this );

   // Plug in the "showMenubar" action, if the caller wants it. Typically this is only necessary for views in the KMix main window.
   if ( vflags & ViewBase::HasMenuBar ) {
      KToggleAction *m = static_cast<KToggleAction*>(  _actions->action( name(KStandardAction::ShowMenubar) ) ) ;

      //static_cast<KToggleAction*>(KStandardAction::showMenubar( this, SLOT(toggleMenuBarSlot()), _actions ));
      //_actions->addAction( m->objectName(), m );
      if ( m != 0 ) {
         if ( vflags & ViewBase::MenuBarVisible ) {
            m->setChecked(true);
         }
         else {
            m->setChecked(false);
         }
      }
   }
   QAction *action = _localActionColletion->addAction("toggle_channels");
   action->setText(i18n("&Channels"));
   connect(action, SIGNAL(triggered(bool) ), SLOT(configureView()));
   connect ( _mixer, SIGNAL(controlChanged()), this, SLOT(refreshVolumeLevels()) );
   connect ( _mixer, SIGNAL(controlsReconfigured(int)), this, SLOT(controlsReconfigured(int)) );
}

ViewBase::~ViewBase() {
    delete _mixSet;
    // A GUI profile can be shared by different views
    // Starting with 5/2009 it is shared by the "tabs" of one card.
    // So we have to make sure to delete it after all users are gone;
    if ( _guiprof != 0 ) {
	_guiprof->decreaseRefcount();
	if ( _guiprof->refcount() == 0 )
	       delete _guiprof;
               _guiprof = 0;
    }
}


void ViewBase::configurationUpdate() {
}

QString ViewBase::id() const {
    return m_viewId;
}

bool ViewBase::isValid() const
{
   return ( _mixSet->count() > 0 || _mixer->dynamic() );
}

void ViewBase::setIcons (bool on) { KMixToolBox::setIcons (_mdws, on ); }
void ViewBase::setLabels(bool on) { KMixToolBox::setLabels(_mdws, on ); }
void ViewBase::setTicks (bool on) { KMixToolBox::setTicks (_mdws, on ); }

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

/*
 * Rebuild the View from the (existing) Profile.
 *
 * Hint: this method signature might be extended in the future by a GUIProfile* paramater.
 */
void ViewBase::rebuildFromProfile()
{
   emit rebuildGUI();
/*
   // Redo everything from scratch, as visibility and the order of the controls might have changed.

   // As the order of the controls is stored in the profile, we need
   // to rebuild the _mixSet 
kDebug() << "rebuild 1";
   _mixSet->clear();
kDebug() << "rebuild 2";
   _mdws.clear();
kDebug() << "rebuild 3";
   setMixSet();
kDebug() << "rebuild 4";
   createDeviceWidgets();
kDebug() << "rebuild 5";
   constructionFinished();
kDebug() << "rebuild 6";
*/
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
    QAction *a;

    _popMenu = new KMenu( this );
    _popMenu->addTitle( KIcon( "kmix" ), i18n("Device Settings") );

    a = _localActionColletion->action( "toggle_channels" );
    if ( a ) _popMenu->addAction(a);

    QAction *b = _actions->action( "options_show_menubar" );
    if ( b ) _popMenu->addAction(b);
}


/**
   This will only get executed, when the user has removed all items from the view.
   Don't remove this method, because then the user cannot get a menu for getting his
   channels back
*/
void ViewBase::showContextMenu()
{
    //kDebug(67100) << "ViewBase::showContextMenu()";
    popupReset();

    QPoint pos = QCursor::pos();
    _popMenu->popup( pos );
}

void ViewBase::controlsReconfigured(int mixerTabIndex)
{
    emit redrawMixer(mixerTabIndex);
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
