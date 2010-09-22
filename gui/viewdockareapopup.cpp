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

#include "gui/viewdockareapopup.h"

// Qt
#include <qevent.h>
#include <qframe.h>
#include <QPushButton>

// KDE
#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kwindowsystem.h>

// KMix
#include "gui/guiprofile.h"
#include "mdwslider.h"
#include "core/mixer.h"
#include "apps/kmix.h"


ViewDockAreaPopup::ViewDockAreaPopup(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof, KMixWindow *dockW )
    : ViewBase(parent, name, mixer, /*Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint*/0, vflags, guiprof), _dock(dockW)
{
    _layoutMDW = new QGridLayout( this );
    _layoutMDW->setSpacing( KDialog::spacingHint() );
    _layoutMDW->setMargin(0);
    _layoutMDW->setObjectName( QLatin1String( "KmixPopupLayout" ) );
    setMixSet();
}

ViewDockAreaPopup::~ViewDockAreaPopup() {
}


void ViewDockAreaPopup::wheelEvent ( QWheelEvent * e ) {
   // Pass wheel event from "border widget" to child
   QWidget* mdw = 0;
   if ( !_mdws.isEmpty() )
      mdw = _mdws.first();

   if ( mdw != 0 )
      QApplication::sendEvent( mdw, e);
}

MixDevice* ViewDockAreaPopup::dockDevice()
{
   MixDeviceWidget* mdw = 0;
   if ( !_mdws.isEmpty() )
      mdw = (MixDeviceWidget*)_mdws.first();

   if ( mdw != 0 )
      return mdw->mixDevice();
   return (MixDevice*)(0);
}


void ViewDockAreaPopup::showContextMenu()
{
    // no right-button-menu on "dock area popup"
    return;
}


void ViewDockAreaPopup::_setMixSet()
{
   // kDebug(67100) << "ViewDockAreaPopup::setMixSet()\n";

   if ( _mixer->dynamic() ) {
      // Our _layoutMDW now should only contain spacer widgets from the QSpacerItems's in add() below.
      // We need to trash those too otherwise all sliders gradually migrate away from the edge :p
      QLayoutItem *li;
      while ( ( li = _layoutMDW->takeAt(0) ) )
         delete li;
   }

   MixDevice *dockMD = Mixer::getGlobalMasterMD();
   if ( dockMD == 0 ) {
      // If we have no dock device yet, we will take the first available mixer device
      if ( _mixer->size() > 0) {
         dockMD = (*_mixer)[0];
      }
   }
   if ( dockMD != 0 ) {
      _mixSet->append(dockMD);
   }
}

QWidget* ViewDockAreaPopup::add(MixDevice *md)
{
    QString dummyMatchAll("*");
    QString matchAllPlaybackAndTheCswitch("pvolume,pswitch,cswitch");
    ProfControl *pctl = new ProfControl( dummyMatchAll, matchAllPlaybackAndTheCswitch);
    MixDeviceWidget *mdw = new MDWSlider(
      md,		  // only 1 device. This is actually _dockDevice
      true,         // Show Mute LED
      false,         // Show Record LED
      false,        // Small
      Qt::Vertical, // Direction: only 1 device, so doesn't matter
      this,         // parent
      0             // Is "NULL", so that there is no RMB-popup
      , pctl
   );
   _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, 2 );
   _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, 0 );
   _layoutMDW->addWidget( mdw, 0, 1 );

   // Add button to show main panel
   _showPanelBox = new QPushButton( i18n("Mixer"), this );
   _showPanelBox->setObjectName( QLatin1String("MixerPanel" ));
   connect ( _showPanelBox, SIGNAL( clicked() ), SLOT( showPanelSlot() ) );
   _layoutMDW->addWidget( _showPanelBox, 1, 0, 1, 3 );

   return mdw;
}

void ViewDockAreaPopup::constructionFinished() {
   //    kDebug(67100) << "ViewDockAreaPopup::constructionFinished()\n";
   QWidget* mdw = 0;
   if ( !_mdws.isEmpty() )
      mdw = _mdws.first();

   if ( mdw != 0 ) {
      mdw->move(0,0);
      mdw->show();
   }
}


void ViewDockAreaPopup::refreshVolumeLevels() {
   //    kDebug(67100) << "ViewDockAreaPopup::refreshVolumeLevels()\n";
   QWidget* mdw = 0;
   if ( !_mdws.isEmpty() )
      mdw = _mdws.first();

   if ( mdw == 0 ) {
      kError(67100) << "ViewDockAreaPopup::refreshVolumeLevels(): mdw == 0\n";
      // sanity check (normally the lists are set up correctly)
   }
   else {
      if ( mdw->inherits("MDWSlider")) { // sanity check
            static_cast<MDWSlider*>(mdw)->update();
      }
      else {
         kError(67100) << "ViewDockAreaPopup::refreshVolumeLevels(): mdw is not slider\n";
         // no slider. Cannot happen in theory => skip it
      }
   }
}

void ViewDockAreaPopup::showPanelSlot() {
    kDebug() << "Check when this is called";
    _dock->setVisible(true);
    KWindowSystem::setOnDesktop(_dock->winId(), KWindowSystem::currentDesktop());
    KWindowSystem::activateWindow(_dock->winId());
    // This is only needed when the window is already visible.
    static_cast<QWidget*>(parent())->hide();
}

#include "viewdockareapopup.moc"
