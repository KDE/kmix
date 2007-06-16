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

#include "viewsliders.h"

// Qt
#include <QLabel>
#include <QLayout>
#include <QWidget>

// KDE
#include <kdebug.h>

// KMix
#include "mdwslider.h"
#include "mixer.h"
#include "verticaltext.h"

/**
 * Don't instanciate objects of this class directly. It won't work
 * correctly because init() does not get called.
 * See ViewInput and ViewOutput for "real" implementations.
 */
ViewSliders::ViewSliders(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof)
      : ViewBase(parent, name, mixer, Qt::FramelessWindowHint, vflags, guiprof)
{
   if ( _vflags & ViewBase::Vertical ) {
      _layoutMDW = new QVBoxLayout(this);
   }
   else {
      _layoutMDW = new QHBoxLayout(this);
   }
   _layoutMDW->setSpacing(0);
}

ViewSliders::~ViewSliders() {
}



QWidget* ViewSliders::add(MixDevice *md)
{

   Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;
   MixDeviceWidget *mdw = new MDWSlider(
                  _mixer,       // the mixer for this device
                  md,           // MixDevice (parameter)
                  true,         // Show Mute LED
                  true,         // Show Record LED
                  false,        // Small
                  orientation,  // Orientation
                  this,         // parent
                  this       ); // View widget
   _layoutMDW->addWidget(mdw);
   return mdw;
}

QSize ViewSliders::sizeHint() const {
   //    kDebug(67100) << "ViewSliders::sizeHint(): NewSize is " << _layoutMDW->sizeHint() << "\n";
   return( _layoutMDW->sizeHint() );
}

void ViewSliders::constructionFinished() {
   _layoutMDW->activate();
}

void ViewSliders::refreshVolumeLevels() {
   //     kDebug(67100) << "ViewSliders::refreshVolumeLevels()\n";

   for ( int i=0; i<_mdws.count(); i++ ) {
      QWidget *mdw = _mdws[i];
      if ( mdw == 0 ) {
         kError(67100) << "ViewSliders::refreshVolumeLevels(): mdw == 0\n";
         break; // sanity check (normally the lists are set up correctly)
      }
      else {
         if ( mdw->inherits("MDWSlider")) { // sanity check
            static_cast<MDWSlider*>(mdw)->update();
         }
         else {
            kError(67100) << "ViewSliders::refreshVolumeLevels(): mdw is not slider\n";
            // no slider. Cannot happen in theory => skip it
         }
      }
   }
}

/*
    // Mockup Hack
    static int num = 0;
    {
     QString labeltext;
     switch (num) {
        case 0:  labeltext = "Desktop"; break;
        case 2:  labeltext = "Music and Video"; break;
        case 5:  labeltext = "Desktop"; break;
        default: labeltext = ""; break;
     }
     num++;
     if ( !labeltext.isEmpty() ) {
        _layoutMDW->addStretch(10);
        if (_vflags & ViewBase::Vertical) { 
          QLabel* lbl = new QLabel(labeltext, this);
          //lbl->setBackgroundRole( QPalette::Dark );
          //lbl->setForegroundRole( QPalette::Midlight );
          lbl->setFrameShape( QFrame::Panel );
          lbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
          //lbl->setBackgroundRole( QPalette::Base );
          _layoutMDW->addWidget(lbl);
        }
        else {
          VerticalText* lbl = new VerticalText(this, labeltext.toUtf8().data());
          //lbl->setBackgroundRole( QPalette::Background );
          lbl->setBackgroundRole( QPalette::AlternateBase );
          lbl->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
          //lbl->setBackgroundRole( QPalette::Base );
          _layoutMDW->addWidget(lbl);
        }
        _layoutMDW->addStretch(10);
     } // if category label shall be inserted
    }
*/

#include "viewsliders.moc"
