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
#include <QLayoutItem>
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


ViewDockAreaPopup::ViewDockAreaPopup(QWidget* parent, const char* name, ViewBase::ViewFlags vflags, QString guiProfileId, KMixWindow *dockW )
    : ViewBase(parent, name, /*Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint*/0, vflags, guiProfileId), _dock(dockW)
{
  foreach ( Mixer* mixer, Mixer::mixers() )
  {
    // Adding all mixers, as we potentially want to show all master controls
    addMixer(mixer);
    connect( mixer, SIGNAL(controlsReconfigured(QString)), this, SLOT(controlsReconfigured(QString)) );

  }
    //_layoutControls = new QHBoxLayout(this);
    _layoutMDW = new QGridLayout( this );
    _layoutMDW->setSpacing( KDialog::spacingHint() );
    _layoutMDW->setMargin(0);
    _layoutMDW->setObjectName( QLatin1String( "KmixPopupLayout" ) );
    createDeviceWidgets();

}

// void ViewDockAreaPopup::controlsReconfigured(QString mixerId)
// {
//   //	    connect( &m_metaMixer, SIGNAL(controlsReconfigured(QString)), this, SLOT(controlsReconfigured(QString)) );
// 
//   kDebug() << "jiha";
//   createDeviceWidgets();
//   constructionFinished();
// }



ViewDockAreaPopup::~ViewDockAreaPopup()
{
  delete _layoutMDW;
}


void ViewDockAreaPopup::wheelEvent ( QWheelEvent * e )
{
  if ( _mdws.isEmpty() )
    return;
  
   // Pass wheel event from "border widget" to child
   QApplication::sendEvent( _mdws.first(), e);
}


// void ViewDockAreaPopup::showContextMenu()
// {
//     // no right-button-menu on "dock area popup"
//     return;
// }


void ViewDockAreaPopup::_setMixSet()
{
  resetMdws();

// TODO code somewhat similar to ViewSliders => refactor  
	// -- remove controls
	if ( isDynamic() ) {
		// Our _layoutMDW now should only contain spacer widgets from the QSpacerItem's in add() below.
		// We need to trash those too otherwise all sliders gradually migrate away from the edge :p
		QLayoutItem *li;
		while ( ( li = _layoutMDW->takeAt(0) ) )
			delete li;
	}

	foreach ( Mixer* mixer, _mixers )
	{
	shared_ptr<MixDevice>dockMD = mixer->getLocalMasterMD();
	if ( dockMD == 0 && mixer->size() > 0 )
	{
		// If we have no dock device yet, we will take the first available mixer device
		dockMD = (*mixer)[0];
	}
	if ( dockMD != 0 )
	{
	  if ( !dockMD->isApplicationStream() && dockMD->playbackVolume().hasVolume()) 
	{
		// don't add application streams here. They are handled below, so
		// we make sure to not add them twice
		_mixSet.append(dockMD);
	}
	}
	} // loop over all cards

	foreach ( Mixer* mixer2 , Mixer::mixers() )
	{
		foreach ( shared_ptr<MixDevice> md, mixer2->getMixSet() )
		{
			if (md->isApplicationStream())
			{
				_mixSet.append(md);
				kDebug(67100) << "Add to tray popup: " << md->id();
			}
		}
	}

}


// void ViewDockAreaPopup::controlsReconfigured( const QString& mixer_ID )
// {
// 	kDebug(67100) << "RECONFIGURE AND RECREATE DOCK";
// 	ViewBase::controlsReconfigured(mixer_ID);
// }


QWidget* ViewDockAreaPopup::add(shared_ptr<MixDevice> md)
{
  bool vertical = (_dock->toplevelOrientation() == Qt::Vertical); // TODO use vflags instead (and set them when Constructing the object)
  
    QString dummyMatchAll("*");
    QString matchAllPlaybackAndTheCswitch("pvolume,cvolume,pswitch,cswitch");
    ProfControl *pctl = new ProfControl( dummyMatchAll, matchAllPlaybackAndTheCswitch);
    MixDeviceWidget *mdw = new MDWSlider(
      md,           // only 1 device.
      true,         // Show Mute LE
      true,        // Show Record LED
      false,        // Small
      _dock->toplevelOrientation(), // TODO: Why don't we use vflags ??? Direction: only 1 device, so doesn't matter
      this,         // parent
      this             // NOT ANYMORE!!! -> Is "NULL", so that there is no RMB-popup
      , pctl
   );
   int sliderColumn = vertical ? _layoutMDW->columnCount() : _layoutMDW->rowCount();
   //if (sliderColumn == 1 ) sliderColumn =0;
   int row = vertical ? 0 : sliderColumn;
   int col = vertical ? sliderColumn : 0;
   
   //_layoutMDW->addItem( new QSpacerItem( 5, 20 ), sliderColumn,0 );
   _layoutMDW->addWidget( mdw, row, col );

   //kDebug(67100) << "ADDED " << md->id() << " at column " << sliderColumn;
   return mdw;
}

void ViewDockAreaPopup::constructionFinished() {
   //    kDebug(67100) << "ViewDockAreaPopup::constructionFinished()\n";

   Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
   bool vertical = (_vflags & ViewBase::Vertical);
   
   int sliderRow = _layoutMDW->rowCount();
  _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, sliderRow ); // TODO add this on "polish()"
   QPushButton *pb = new QPushButton( i18n("Mixer"), this );
   pb->setObjectName( QLatin1String("MixerPanel" ));
   connect ( pb, SIGNAL(clicked()), SLOT(showPanelSlot()) );
   //_layoutMDW->addWidget( pb, sliderColumn+1, 0, 1, 1 );
   
       const KIcon& icon = KIcon( QLatin1String( "configure" ));
    QPushButton* configureViewButton = new QPushButton(icon, "", this);
    configureViewButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

   QBoxLayout* optionsLayout;
   if ( vertical )
     optionsLayout = new QVBoxLayout(this);
   else
     optionsLayout = new QHBoxLayout(this);
    optionsLayout->addWidget(pb );
    optionsLayout->addWidget(configureViewButton);
   optionsLayout->addWidget( createRestoreVolumeButton(1) );
   optionsLayout->addWidget( createRestoreVolumeButton(2) );
   optionsLayout->addWidget( createRestoreVolumeButton(3) );
   optionsLayout->addWidget( createRestoreVolumeButton(4) );
   
   _layoutMDW->addLayout(optionsLayout, sliderRow+1, 0, 1, 1);
}

    QPushButton* ViewDockAreaPopup::createRestoreVolumeButton ( int storageSlot )
    {
	QString buttonText = QString("%1").arg(storageSlot);
// 	buttonText.arg(storageSlot);
	QPushButton* profileButton = new QPushButton(buttonText, this);
	profileButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	return profileButton;
    }

void ViewDockAreaPopup::refreshVolumeLevels() {
  foreach ( QWidget* qw, _mdws )
  {
    //kDebug() << "rvl: " << qw;
    MixDeviceWidget* mdw = qobject_cast<MixDeviceWidget*>(qw);
    if ( mdw != 0 ) mdw->update();
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
