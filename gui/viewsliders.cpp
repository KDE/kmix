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

//#define TEST_MIXDEVICE_COMPOSITE
#undef TEST_MIXDEVICE_COMPOSITE

#ifdef TEST_MIXDEVICE_COMPOSITE
#ifdef __GNUC__
#warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#warning !!! MIXDEVICE COMPOSITE TESTING IS ACTIVATED   !!!
#warning !!! THIS IS PRE-ALPHA CODE!                    !!!
#warning !!! DO NOT SHIP KMIX IN THIS STATE             !!!
#warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#endif
#endif

// KMix
#include "viewsliders.h"
#include "gui/guiprofile.h"
#include "mdwenum.h"
#include "mdwslider.h"
#include "core/mixdevicecomposite.h"
#include "core/mixer.h"
#include "verticaltext.h"

// KDE
#include <kdebug.h>

// Qt
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
//#include <QFormLayout>



/**
 * Generic View implementation. This can hold now all kinds of controls (not just Sliders, as
 *  the class name suggests).
 */
ViewSliders::ViewSliders(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof, KActionCollection *actColl)
      : ViewBase(parent, name, mixer, Qt::FramelessWindowHint, vflags, guiprof, actColl)
      , _layoutEnum(0)
{
   if ( _vflags & ViewBase::Vertical ) {
      _layoutMDW = new QVBoxLayout(this);
      _layoutMDW->setAlignment(Qt::AlignLeft|Qt::AlignTop);
      _layoutSliders = new QVBoxLayout();
      _layoutSliders->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
   }
   else
   {
      _layoutMDW = new QHBoxLayout(this);
      _layoutMDW->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
      _layoutSliders = new QHBoxLayout();
      _layoutSliders->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
      // Place enums in an own box right from the sliders.
   }
   _layoutSliders->setContentsMargins(0,0,0,0);
   _layoutSliders->setSpacing(0);
   _layoutMDW->setContentsMargins(0,0,0,0);
   _layoutMDW->setSpacing(0);
   _layoutMDW->addItem( _layoutSliders );

    setMixSet();
}

ViewSliders::~ViewSliders()
{
  qDeleteAll(_separators);
}



QWidget* ViewSliders::add(MixDevice *md)
{
    MixDeviceWidget *mdw;
    Qt::Orientation orientation = (_vflags & ViewBase::Vertical) ? Qt::Horizontal : Qt::Vertical;



    if ( md->isEnum() ) {
        mdw = new MDWEnum(
                md,           // MixDevice (parameter)
                orientation,  // Orientation
                this,         // parent
                this          // View widget
                , md->controlProfile()
        );
        if ( _layoutEnum == 0 ) {
            // lazily creation of Layout for the first enum
            _layoutEnum = new QVBoxLayout(); // new QFormLayout();
            _layoutMDW->addLayout( _layoutEnum );
        }
        _layoutEnum->addWidget(mdw);
    } // an enum
    else {
        // add a separator before the device
        QFrame *_frm = new QFrame(this);
        if ( orientation == Qt::Vertical)
            _frm->setFrameStyle(QFrame::VLine | QFrame::Sunken);
        else
            _frm->setFrameStyle(QFrame::HLine | QFrame::Sunken);
        _separators.insert(md->id(),_frm);
        _layoutSliders->addWidget(_frm);
        mdw = new MDWSlider(
                md,           // MixDevice (parameter)
                true,         // Show Mute LED
                true,         // Show Record LED
                false,        // Small
                orientation,  // Orientation
                this,         // parent
                this
                , md->controlProfile()
        ); // View widget
        _layoutSliders->addWidget(mdw);
//        QHBoxLayout* lay = ::qobject_cast<QHBoxLayout*>(_layoutSliders);
//        if ( lay )
//            lay->addSpacing(2);
//        else
//            qobject_cast<QVBoxLayout*>(_layoutSliders)->addSpacing(2);
    }


    return mdw;
}


void ViewSliders::_setMixSet()
{
    const MixSet& mixset = _mixer->getMixSet();

    if ( _mixer->dynamic() ) {
        // We will be recreating our sliders, so make sure we trash all the separators too.
        qDeleteAll(_separators);
        _separators.clear();
        // Our _layoutSliders now should only contain spacer widgets from the addSpacing() calls in add() above.
        // We need to trash those too otherwise all sliders gradually migrate away from the edge :p
        QLayoutItem *li;
        while ( ( li = _layoutSliders->takeAt(0) ) )
            delete li;
    }


#ifdef TEST_MIXDEVICE_COMPOSITE
    QList<MixDevice*> mds;  // For temporary test
#endif

    // This method iterates the controls from the Profile
    // Each control is checked, whether it is also contained in the mixset, and
    // applicable for this kind of View. If yes, the control is accepted and inserted.
   
    foreach ( ProfControl* control, _guiprof->getControls() )
    {
        //ProfControl* control = *it;
        // The TabName of the control matches this View name (!! attention: Better use some ID, due to i18n() )
        bool isUsed = false;

        QRegExp idRegexp(control->id);
        //bool isExactRegexp = control->id.startsWith('^') && control->id.endsWith('$'); // for optimizing
        //isExactRegexp &= ( ! control->id.contains(".*") ); // For now. Might be removed in the future, as it cannot be done properly !!!
        //kDebug(67100) << "ViewSliders::setMixSet(): Check GUIProfile id==" << control->id << "\n";
        // The following for-loop could be simplified by using a std::find_if
        for ( int i=0; i<mixset.count(); i++ ) {
            MixDevice *md = mixset[i];
            if ( md->id().contains(idRegexp) )
            {
                // Match found (by name)
                if ( _mixSet->contains( md ) ) continue; // dup check

                // Now check whether subcontrols match
                bool subcontrolPlaybackWanted = (control->useSubcontrolPlayback() && md->playbackVolume().hasVolume());
                bool subcontrolCaptureWanted  = (control->useSubcontrolCapture()  && md->captureVolume().hasVolume());
                bool subcontrolEnumWanted  = (control->useSubcontrolEnum() && md->isEnum());
                bool subcontrolWanted =  subcontrolPlaybackWanted | subcontrolCaptureWanted | subcontrolEnumWanted;
		bool splitWanted = control->isSplit();

                if ( !subcontrolWanted ) continue;

                md->setControlProfile(control);
                if ( !control->name.isNull() ) {
                    // Apply the custom name from the profile
                    md->setReadableName(control->name);  // @todo: This is the wrong place. It only applies to controls in THIS type of view
                }
                if ( !control->getSwitchtype().isNull() ) {
                    if ( control->getSwitchtype() == "On"  )
                        md->playbackVolume().setSwitchType(Volume::OnSwitch);
                    else if ( control->getSwitchtype() == "Off"  )
                        md->playbackVolume().setSwitchType(Volume::OffSwitch);
                }
                _mixSet->append(md);

#ifdef TEST_MIXDEVICE_COMPOSITE
                if ( md->id() == "Front:0" || md->id() == "Surround:0") { mds.append(md); } // For temporary test
#endif

                isUsed = true;
                // We use no "break;" ,as multiple devices could match
                //if ( isExactRegexp ) break;  // Optimize! In this case, we can actually break the loop
            } // name matches
        } // loop for finding a suitable MixDevice
        if ( ! isUsed ) {
            // There is something in the Profile, that doesn't correspond to a Mixer control
            //kDebug(67100) << "ViewSliders::setMixSet(): No such control '" << control->id << "'in the mixer . Please check the GUIProfile\n";
        }
   } // iteration over all controls from the Profile

#ifdef TEST_MIXDEVICE_COMPOSITE
    // This is currently hardcoded, and instead must be read as usual from the Profile
    MixDeviceComposite *mdc = new MixDeviceComposite(_mixer, "Composite_Test", mds, "A Composite Control #1", MixDevice::KMIX_COMPOSITE);
    QString ctlId("Composite_Test");
    QString ctlMatchAll("*");
    ProfControl* pctl = new ProfControl(ctlId, ctlMatchAll);
    mdc->setControlProfile(pctl);
    _mixSet->append(mdc);
#endif
}


void ViewSliders::constructionFinished() {
    configurationUpdate();
}


void ViewSliders::configurationUpdate() {
   // Adjust height of top part by setting it to the maximum of all mdw's
   bool haveCaptureLEDs = false;
   int labelExtent = 0;
   bool haveMuteButtons = false;
   for ( int i=0; i<_mdws.count(); i++ ) {
      MDWSlider* mdw = ::qobject_cast<MDWSlider*>(_mdws[i]);
      if ( mdw && mdw->isVisibleTo(this) ) {
		 if ( mdw->labelExtentHint() > labelExtent ) labelExtent = mdw->labelExtentHint();
		 haveCaptureLEDs = haveCaptureLEDs || mdw->hasCaptureLED();
		 haveMuteButtons = haveMuteButtons || mdw->hasMuteButton();
      }
   }
   //kDebug(67100) << "topPartExtent is " << topPartExtent;
   bool firstVisibleControlFound = false;
   for ( int i=0; i<_mdws.count(); i++ ) {
      MDWSlider* mdw = ::qobject_cast<MDWSlider*>(_mdws[i]);
      if ( mdw ) {
		 mdw->setLabelExtent(labelExtent);
		 mdw->setMuteButtonSpace(haveMuteButtons);
		 mdw->setCaptureLEDSpace(haveCaptureLEDs);
         bool thisControlIsVisible = mdw->isVisibleTo(this);
         bool showSeparator = ( firstVisibleControlFound && thisControlIsVisible);
         if ( _separators.contains( mdw->mixDevice()->id() )) {
            QFrame* sep = _separators[mdw->mixDevice()->id()];
            sep->setVisible(showSeparator);
         }
         if ( thisControlIsVisible ) firstVisibleControlFound=true;
      }
    } // for all  MDW's
    _layoutMDW->activate();
}

void ViewSliders::refreshVolumeLevels() {
    //     kDebug(67100) << "ViewSliders::refreshVolumeLevels()\n";

    for ( int i=0; i<_mdws.count(); i++ ) {
        QWidget *mdwx = _mdws[i];
        if ( mdwx == 0 ) {
            kError(67100) << "ViewSliders::refreshVolumeLevels(): mdw == 0\n";
            break; // sanity check (normally the lists are set up correctly)
        }
        else {
            MixDeviceWidget* mdw = ::qobject_cast<MixDeviceWidget*>(mdwx);
            if ( mdw != 0 ) { // sanity check

#ifdef TEST_MIXDEVICE_COMPOSITE
                // --- start --- The following 4 code lines should be moved to a more
                //                      generic place, as it only works in this View. But it
                //                      should also work in the ViewDockareaPopup and everywhere else.
                MixDeviceComposite* mdc = ::qobject_cast<MixDeviceComposite*>(mdw->mixDevice());
                if (mdc != 0) {
                    mdc->update();
                }
                // --- end ---
#endif

                mdw->update();
            }
            else {
                kError(67100) << "ViewSliders::refreshVolumeLevels(): mdw is not a MixDeviceWidget\n";
                // no slider. Cannot happen in theory => skip it
            }
        }
    }
}

#include "viewsliders.moc"
