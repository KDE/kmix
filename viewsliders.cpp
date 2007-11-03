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

// KMix
#include "viewsliders.h"
#include "guiprofile.h"
#include "mdwenum.h"
#include "mdwslider.h"
#include "mixer.h"
#include "verticaltext.h"

// KDE
#include <kdebug.h>

// Qt
#include <QLabel>
#include <QLayout>
#include <QWidget>




/**
 * Generic View implementation. This can hold now all kinds of controls (not just Sliders, as
 *  the class name suggests).
 */
ViewSliders::ViewSliders(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof, KActionCollection *actColl)
      : ViewBase(parent, name, mixer, Qt::FramelessWindowHint, vflags, guiprof, actColl)
{
   if ( _vflags & ViewBase::Vertical ) {
      _layoutMDW = new QVBoxLayout(this);
      _layoutSliders = new QVBoxLayout();
      _layoutMDW->addItem( _layoutSliders );
      _layoutEnum = new QVBoxLayout(); // always vertical!
      _layoutMDW->addItem( _layoutEnum );
   }
   else {
      _layoutMDW = new QHBoxLayout(this);
      _layoutSliders = new QHBoxLayout();
      _layoutMDW->addItem( _layoutSliders );
      // Place enums right from the sliders: This is done, so that there will be no
      // ugly space on the left side, when no Switch is shown.
      // Actually it is not really clear yet, why there is empty space at all: There are 0 items in
      // the _layoutEnum, so it might be a sizeHint() or some other subtle layout issue.
      _layoutEnum = new QVBoxLayout();
      _layoutMDW->addItem( _layoutEnum );
   }
   _layoutMDW->setSpacing(0);
    setMixSet();
}

ViewSliders::~ViewSliders() {
}



QWidget* ViewSliders::add(MixDevice *md)
{
   MixDeviceWidget *mdw;
   Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;

   /* Hint: We allow to put Enum's in the same View as sliders and switches.
            Normally you won't do this, but the creator of the Profile is at least free to do so if he wishes. */
   if ( md->isEnum() ) {
      mdw = new MDWEnum(
               md,           // MixDevice (parameter)
               orientation,  // Orientation
               this,         // parent
               this          // View widget
      );
      _layoutEnum->addWidget(mdw);
   } // an enum
   else {
      mdw = new MDWSlider(
               md,           // MixDevice (parameter)
               true,         // Show Mute LED
               true,         // Show Record LED
               false,        // Small
               orientation,  // Orientation
               this,         // parent
               this       ); // View widget
      _layoutSliders->addWidget(mdw);
   }
   return mdw;
}


void ViewSliders::setMixSet()
{
    const MixSet& mixset = _mixer->getMixSet();


   // This method iterates the controls from the Profile
   // Each control is checked, whether it is also contained in the mixset, and
   // applicable for this kind of View. If yes, the control is accepted and inserted.
   
   std::vector<ProfControl*>::const_iterator itEnd = _guiprof->_controls.end();
   for ( std::vector<ProfControl*>::const_iterator it = _guiprof->_controls.begin(); it != itEnd; ++it)
   {
      ProfControl* control = *it;
      if ( control->tab == viewId() ) {
         // The TabName of the control matches this View name (!! attention: Better use some ID, due to i18n() )
         bool isUsed = false;
   
         QRegExp idRegexp(control->id);
         //kDebug(67100) << "ViewSliders::setMixSet(): Check GUIProfile id==" << control->id << "\n";
         // The following for-loop could be simplified by using a std::find_if
         for ( int i=0; i<mixset.count(); i++ ) {
            MixDevice *md = mixset[i];
            if ( md->id().contains(idRegexp) )
            {
               /*kDebug(67100) << "     ViewSliders::setMixSet(): match found for md->id()==" <<
               md->id()
                  << " ; control->id=="
                  << control->id << "\n"; */
               // OK, this control is handable by this View. Lets do a duplicate check
               if ( ! _mixSet->contains( md ) ) {
                  if ( !control->name.isNull() ) {
                     // Apply the custom name from the profile
                     md->setReadableName(control->name);  // @todo: This is the wrong place. It only applies to controls in THIS type of view
                  }
                  _mixSet->append(md);
                  isUsed = true;
                  // We use no "break;" ,as multiple devices could match
                  //break;
               }
               else {
                  //kDebug(67100) << "        But it is a duplicate and was not added\n";
               }
            } // name matches
         } // loop for finding a suitable MixDevice
         if ( ! isUsed ) {
            // There is something in the Profile, that doesn't correspond to a Mixer control
            kDebug(67100) << "ViewSliders::setMixSet(): No such control '" << control->id << "'in the mixer . Please check the GUIProfile\n";
         }
      } // Tab name matches
      else {
      }  // Tab name doesn't match (=> don't insert)
   } // iteration over all controls from the Profile
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
         if ( mdw->inherits("MixDeviceWidget")) { // sanity check
            static_cast<MixDeviceWidget*>(mdw)->update();
         }
         else {
            kError(67100) << "ViewSliders::refreshVolumeLevels(): mdw is not a MixDeviceWidget\n";
            // no slider. Cannot happen in theory => skip it
         }
      }
   }
}

#include "viewsliders.moc"
