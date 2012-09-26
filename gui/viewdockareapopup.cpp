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
#include <QGridLayout>
#include <QLabel>
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
#include "apps/kmix.h"
#include "core/mixer.h"
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"
#include "gui/guiprofile.h"
#include "gui/mdwslider.h"


ViewDockAreaPopup::ViewDockAreaPopup(QWidget* parent, const char* name, ViewBase::ViewFlags vflags, QString guiProfileId, KMixWindow *dockW )
    : ViewBase(parent, name, /*Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint*/0, vflags, guiProfileId), _dock(dockW)
{
  seperatorBetweenMastersAndStreams = 0;
  optionsLayout = 0;
  foreach ( Mixer* mixer, Mixer::mixers() )
  {
    // Adding all mixers, as we potentially want to show all master controls
    addMixer(mixer);
  }

  // Register listeners for all mixers
      	ControlManager::instance().addListener(
	  QString(), // all mixers
	ControlChangeType::GUI,
	this,
	QString("ViewDockAreaPopup")	  
	);

  	ControlManager::instance().addListener(
	  QString(), // all mixers
	ControlChangeType::ControlList,
	this,
	QString("ViewDockAreaPopup")	  
	);

  	ControlManager::instance().addListener(
	  QString(), // all mixers
	ControlChangeType::Volume,
	this,
	QString("ViewDockAreaPopup")	  
	);
	
	  	ControlManager::instance().addListener(
	  QString(), // all mixers
	ControlChangeType::MasterChanged,
	this,
	QString("ViewDockAreaPopup")	  
	);

  //_layoutControls = new QHBoxLayout(this);
    _layoutMDW = new QGridLayout( this );
    _layoutMDW->setSpacing( KDialog::spacingHint() );
    _layoutMDW->setMargin(0);
    _layoutMDW->setObjectName( QLatin1String( "KmixPopupLayout" ) );
    createDeviceWidgets();

}


ViewDockAreaPopup::~ViewDockAreaPopup()
{
  ControlManager::instance().removeListener(this);
  delete _layoutMDW;
  // Hint: optionsLayout and "everything else" is deleted when delete _layoutMDW; cacades down
}


void ViewDockAreaPopup::controlsChange(int changeType)
{
  ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
  switch (type )
  {
    case  ControlChangeType::ControlList:
    case  ControlChangeType::MasterChanged:
      createDeviceWidgets();
      break;
    case ControlChangeType::GUI:
        setTicks(GlobalConfig::instance().showTicks);
	setLabels(GlobalConfig::instance().showLabels);
      break;

    case ControlChangeType::Volume:
      refreshVolumeLevels();
      break;

    default:
      ControlManager::warnUnexpectedChangeType(type, this);
      break;
  }
    
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
  delete seperatorBetweenMastersAndStreams;
  separatorBetweenMastersAndStreamsInserted = false;
  separatorBetweenMastersAndStreamsRequired = false;
  delete optionsLayout;
  optionsLayout = 0;

// TODO code somewhat similar to ViewSliders => refactor  
	// -- remove controls
// 	if ( isDynamic() ) {
		// Our _layoutMDW now should only contain spacer widgets from the QSpacerItem's in add() below.
		// We need to trash those too otherwise all sliders gradually migrate away from the edge :p
		QLayoutItem *li;
		while ( ( li = _layoutMDW->takeAt(0) ) )
			delete li;
// 	}

	// A loop that adds the Master controls of each card
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

QWidget* ViewDockAreaPopup::add(shared_ptr<MixDevice> md)
{
  bool vertical = (_dock->toplevelOrientation() == Qt::Vertical); // TODO use vflags instead (and set them when Constructing the object)
  
    QString dummyMatchAll("*");
    QString matchAllPlaybackAndTheCswitch("pvolume,cvolume,pswitch,cswitch");
    ProfControl *pctl = new ProfControl( dummyMatchAll, matchAllPlaybackAndTheCswitch);
    
    if ( !md->isApplicationStream() )
    {
      separatorBetweenMastersAndStreamsRequired = true;
    }
    if ( !separatorBetweenMastersAndStreamsInserted && separatorBetweenMastersAndStreamsRequired && md->isApplicationStream() )
    {
      // First application stream => add separator
      separatorBetweenMastersAndStreamsInserted = true;

   int sliderColumn = vertical ? _layoutMDW->columnCount() : _layoutMDW->rowCount();
   int row = vertical ? 0 : sliderColumn;
   int col = vertical ? sliderColumn : 0;
   seperatorBetweenMastersAndStreams = new QFrame();
   if (vertical)
     seperatorBetweenMastersAndStreams->setFrameStyle(QFrame::VLine);
   else
     seperatorBetweenMastersAndStreams->setFrameStyle(QFrame::HLine);
_layoutMDW->addWidget( seperatorBetweenMastersAndStreams, row, col );
//_layoutMDW->addItem( new QSpacerItem( 5, 5 ), row, col );
    }
    
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
   
   _layoutMDW->addWidget( mdw, row, col );

   //kDebug(67100) << "ADDED " << md->id() << " at column " << sliderColumn;
   return mdw;
}

void ViewDockAreaPopup::constructionFinished()
{
   kDebug(67100) << "ViewDockAreaPopup::constructionFinished()\n";

   Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
   bool vertical = (_vflags & ViewBase::Vertical);
   
//   _layoutMDW->addItem( new QSpacerItem( 5, 20 ), 0, sliderRow ); // TODO add this on "polish()"
   QPushButton *pb = new QPushButton( i18n("Mixer") );
   pb->setObjectName( QLatin1String("MixerPanel" ));
   connect ( pb, SIGNAL(clicked()), SLOT(showPanelSlot()) );
   
    const KIcon& icon = KIcon( QLatin1String( "configure" ));
    QPushButton* configureViewButton = new QPushButton(icon, "");
    configureViewButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

   optionsLayout = new QHBoxLayout();
    optionsLayout->addWidget(pb );
    optionsLayout->addWidget(configureViewButton);
   optionsLayout->addWidget( createRestoreVolumeButton(1) );
   optionsLayout->addWidget( createRestoreVolumeButton(2) );
   optionsLayout->addWidget( createRestoreVolumeButton(3) );
   optionsLayout->addWidget( createRestoreVolumeButton(4) );
   
      int sliderRow = _layoutMDW->rowCount();
      _layoutMDW->addLayout(optionsLayout, sliderRow, 0, 1, _layoutMDW->columnCount());
}

    QPushButton* ViewDockAreaPopup::createRestoreVolumeButton ( int storageSlot )
    {
	QString buttonText = QString("%1").arg(storageSlot);
	QPushButton* profileButton = new QPushButton(buttonText);
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
