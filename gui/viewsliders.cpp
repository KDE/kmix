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

#include "gui/viewsliders.h"

// KMix
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"
#include "core/mixdevicecomposite.h"
#include "core/mixer.h"
#include "gui/guiprofile.h"
#include "gui/mdwenum.h"
#include "gui/mdwslider.h"
#include "gui/verticaltext.h"


// KDE
#include <kdebug.h>
#include <KIcon>
#include <KLocale>

// Qt
#include <QPushButton>
#include <QLabel>
#include <QLayoutItem>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * Generic View implementation. This can hold now all kinds of controls (not just Sliders, as
 *  the class name suggests).
 */
ViewSliders::ViewSliders(QWidget* parent, QString id, Mixer* mixer, ViewBase::ViewFlags vflags, QString guiProfileId,
	KActionCollection *actColl) :
	ViewBase(parent, id, Qt::FramelessWindowHint, vflags, guiProfileId, actColl), _layoutEnum(0)
{
	addMixer(mixer);

	_configureViewButton = 0;
	_layoutMDW = 0;
	_layoutSliders = 0;
	_layoutEnum = 0;
	emptyStreamHint = 0;

	createDeviceWidgets();

	ControlManager::instance().addListener(mixer->id(),
		(ControlChangeType::Type) (ControlChangeType::GUI | ControlChangeType::ControlList | ControlChangeType::Volume),
		this, QString("ViewSliders.%1").arg(mixer->id()));

}

ViewSliders::~ViewSliders()
{
    ControlManager::instance().removeListener(this);
    delete _layoutMDW;
//     qDeleteAll(_separators);
}


void ViewSliders::controlsChange(int changeType)
{
  ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
  switch (type )
  {
    case  ControlChangeType::ControlList:
      createDeviceWidgets();
      break;
    case ControlChangeType::GUI:
    	updateGuiOptions();
      break;
      
    case ControlChangeType::Volume:
      kDebug() << "NOW I WILL REFRESH VOLUME LEVELS. I AM " << id(); // TODO RELEASE Remove debug
      refreshVolumeLevels();
      break;
      
    default:
      ControlManager::warnUnexpectedChangeType(type, this);
      break;
  }
    
}


QWidget* ViewSliders::add(shared_ptr<MixDevice> md)
{
    MixDeviceWidget *mdw;
    Qt::Orientation orientation = GlobalConfig::instance().toplevelOrientation;

    if ( md->isEnum() )
    {
        mdw = new MDWEnum(
                md,           // MixDevice (parameter)
                orientation,  // Orientation
                this,         // parent
                this          // View widget
                , md->controlProfile()
        );
//        if ( _layoutEnum == 0 ) {
//            // lazily creation of Layout for the first enum
//            _layoutEnum = new QVBoxLayout();
//            _layoutMDW->addLayout( _layoutEnum );
//        }
        _layoutEnum->addWidget(mdw);
    } // an enum
    else
    {
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
    }

    return mdw;
}


void ViewSliders::_setMixSet()
{
	resetMdws();

	delete emptyStreamHint;
	emptyStreamHint = 0;

	// Our _layoutSliders now should only contain spacer widgets from the addSpacing() calls in add() above.
	// We need to trash those too otherwise all sliders gradually migrate away from the edge :p
	if (_layoutSliders != 0)
	{
		QLayoutItem *li;
		while ((li = _layoutSliders->takeAt(0)))
			delete li;
//		delete _layoutSliders;
		_layoutSliders = 0;
	}


	delete _configureViewButton;
	_configureViewButton = 0;

	if (_layoutEnum != 0)
	{
		QLayoutItem *li2;
		while ((li2 = _layoutEnum->takeAt(0)) != 0)
			delete li2;
//		delete _layoutEnum;
		_layoutEnum = 0;
	}

	delete _layoutMDW;
	_layoutMDW = 0;

	// We will be recreating our sliders, so make sure we trash all the separators too.
	//qDeleteAll(_separators);
	_separators.clear();

	if (GlobalConfig::instance().toplevelOrientation == Qt::Horizontal)
	{
		// Horizontal slider => put them vertically
		kDebug()
		<< "hor slider => vertical alignment";
		_layoutMDW = new QVBoxLayout(this);
		_layoutMDW->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		_layoutSliders = new QVBoxLayout();
		_layoutSliders->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	}
	else
	{
		// Vertical slider => put them horizontally
		kDebug()
		<< "vert slider => horizontal alignment";
		_layoutMDW = new QHBoxLayout(this);
		_layoutMDW->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		_layoutSliders = new QHBoxLayout();
		_layoutSliders->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	}
	_layoutSliders->setContentsMargins(0, 0, 0, 0);
	_layoutSliders->setSpacing(0);
	_layoutMDW->setContentsMargins(0, 0, 0, 0);
	_layoutMDW->setSpacing(0);
	_layoutMDW->addItem(_layoutSliders);

	_layoutEnum = new QVBoxLayout();
	_layoutMDW->addLayout(_layoutEnum);

	// Hint: This text comparison is not a clean solution, but one that will work for quite a while.
	QString viewId(id());
	if (viewId.contains(".Capture_Streams."))
		emptyStreamHint = new QLabel(i18n("Nothing is capturing audio."));
	else if (viewId.contains(".Playback_Streams."))
		emptyStreamHint = new QLabel(i18n("Nothing is playing audio."));
	else if (viewId.contains(".Capture_Devices."))
		emptyStreamHint = new QLabel(i18n("No capture devices."));
	else if (viewId.contains(".Playback_Devices."))
		emptyStreamHint = new QLabel(i18n("No playback devices."));
	else
		emptyStreamHint = new QLabel(i18n("Nothing is playing audio.")); // Fallback. Assume Playback stream

	emptyStreamHint->setAlignment(Qt::AlignCenter);
	emptyStreamHint->setWordWrap(true);
	emptyStreamHint->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	_layoutMDW->addWidget(emptyStreamHint);

#ifdef TEST_MIXDEVICE_COMPOSITE
	QList<shared_ptr<MixDevice> > mds;  // For temporary test
#endif

	// This method iterates the controls from the Profile
	// Each control is checked, whether it is also contained in the mixset, and
	// applicable for this kind of View. If yes, the control is accepted and inserted.

	GUIProfile* guiprof = guiProfile();
	if (guiprof != 0)
	{
		foreach (Mixer* mixer , _mixers ){
		const MixSet& mixset = mixer->getMixSet();

		foreach ( ProfControl* control, guiprof->getControls() )
		{
			//ProfControl* control = *it;
			// The TabName of the control matches this View name (!! attention: Better use some ID, due to i18n() )
			QRegExp idRegexp(control->id);
			//bool isExactRegexp = control->id.startsWith('^') && control->id.endsWith('$'); // for optimizing
			//isExactRegexp &= ( ! control->id.contains(".*") ); // For now. Might be removed in the future, as it cannot be done properly !!!
			//kDebug(67100) << "ViewSliders::setMixSet(): Check GUIProfile id==" << control->id << "\n";
			// The following for-loop could be simplified by using a std::find_if
			for ( int i=0; i<mixset.count(); i++ )
			{
				shared_ptr<MixDevice> md = mixset[i];

				if ( md->id().contains(idRegexp) )
				{
					// Match found (by name)
					if ( _mixSet.contains( md ) )
					{
						continue; // dup check
					}

					// Now check whether subcontrols match
					bool subcontrolPlaybackWanted = (control->useSubcontrolPlayback() && ( md->playbackVolume().hasVolume() || md->hasMuteSwitch()) );
					bool subcontrolCaptureWanted = (control->useSubcontrolCapture() && ( md->captureVolume() .hasVolume() || md->captureVolume() .hasSwitch()) );
					bool subcontrolEnumWanted = (control->useSubcontrolEnum() && md->isEnum());
					bool subcontrolWanted = subcontrolPlaybackWanted | subcontrolCaptureWanted | subcontrolEnumWanted;

					if ( !subcontrolWanted )
					{
						continue;
					}

					md->setControlProfile(control);
					if ( !control->name.isNull() )
					{
						// Apply the custom name from the profile
						md->setReadableName(control->name);// @todo: This is the wrong place. It only applies to controls in THIS type of view
					}
					if ( !control->getSwitchtype().isNull() )
					{
						if ( control->getSwitchtype() == "On" )
						md->playbackVolume().setSwitchType(Volume::OnSwitch);
						else if ( control->getSwitchtype() == "Off" )
						md->playbackVolume().setSwitchType(Volume::OffSwitch);
					}
					_mixSet.append(md);

#ifdef TEST_MIXDEVICE_COMPOSITE
					if ( md->id() == "Front:0" || md->id() == "Surround:0")
					{	mds.append(md);} // For temporary test
#endif
					// We use no "break;" ,as multiple devices could match the regexp (e.g. "^.*$")
				} // name matches
			} // loop for finding a suitable MixDevice
		} // iteration over all controls from the Profile
	} // if there is a profile
} // Iteration over all Mixers

	emptyStreamHint->setVisible(_mixSet.isEmpty() && isDynamic()); // show a hint why a tab is empty (dynamic controls!!!)
		//  visibleControls() == 0 could be used for the !isDynamic() case

#ifdef TEST_MIXDEVICE_COMPOSITE
	// @todo: This is currently hardcoded, and instead must be read as usual from the Profile
	MixDeviceComposite *mdc = new MixDeviceComposite(_mixer, "Composite_Test", mds, "A Composite Control #1", MixDevice::KMIX_COMPOSITE);
	Volume::ChannelMask chn = Volume::MMAIN;
	Volume* vol = new Volume( chn, 0, 100, true, true);
	mdc->addPlaybackVolume(*vol);
	QString ctlId("Composite_Test");
	QString ctlMatchAll("*");
	ProfControl* pctl = new ProfControl(ctlId, ctlMatchAll);
	mdc->setControlProfile(pctl);
	_mixSet->append(mdc);
#endif
}


void ViewSliders::constructionFinished() {
    configurationUpdate();
    //if ( !pulseaudioPresent() ) // TODO 11 Dynamic view configuration
    if ( !isDynamic() )
    {
		_configureViewButton = createConfigureViewButton();
		_layoutEnum->addStretch();
		_layoutEnum->addWidget(_configureViewButton);
    }

    updateGuiOptions();
}


void ViewSliders::configurationUpdate()
{
	// Adjust height of top part by setting it to the maximum of all mdw's
	bool haveCaptureLEDs = false;
	int labelExtent = 0;
	bool haveMuteButtons = false;

	// Find out whether any MDWSlider has Switches. If one has, then we need "extents"
	for (int i = 0; i < _mdws.count(); i++)
	{
		MDWSlider* mdw = ::qobject_cast<MDWSlider*>(_mdws[i]);
		if (mdw && mdw->isVisibleTo(this))
		{
			if (mdw->labelExtentHint() > labelExtent)
				labelExtent = mdw->labelExtentHint();
			haveCaptureLEDs = haveCaptureLEDs || mdw->hasCaptureLED();
			haveMuteButtons = haveMuteButtons || mdw->hasMuteButton();
		}

		if (haveCaptureLEDs && haveMuteButtons)
			break; // We know all we want. Lets break.
	}
   //kDebug(67100) << "topPartExtent is " << topPartExtent;
   bool firstVisibleControlFound = false;
   for ( int i=0; i<_mdws.count(); i++ )
   {
      MixDeviceWidget* mdw = ::qobject_cast<MixDeviceWidget*>(_mdws[i]);
      MDWSlider* mdwSlider = ::qobject_cast<MDWSlider*>(_mdws[i]);
      if ( mdw )
      {
	 // This is a bit hacky. Using "simple" can be wrong on the very first start of KMix (but usually it is not!)
	ProfControl* matchingControl = findMdw(mdw->mixDevice()->id(), QString("simple"));
	mdw->setVisible(matchingControl != 0);
	if ( mdwSlider == 0 )
	{
	  // not a slider => debug output for enums
	  kDebug() << "Show ENUM " << mdw->mixDevice()->id() << " ?  matchingControl=" << matchingControl;
	}

	if ( mdwSlider )
	{
		  // additional options for sliders
		 mdwSlider->setLabelExtent(labelExtent);
		 mdwSlider->setMuteButtonSpace(haveMuteButtons);
		 mdwSlider->setCaptureLEDSpace(haveCaptureLEDs);
	}
         bool thisControlIsVisible = mdw->isVisibleTo(this);
         bool showSeparator = ( firstVisibleControlFound && thisControlIsVisible);
         if ( _separators.contains( mdw->mixDevice()->id() ))
	 {
            QFrame* sep = _separators[mdw->mixDevice()->id()];
            sep->setVisible(showSeparator);
         }
         if ( thisControlIsVisible )
	   firstVisibleControlFound=true;
      }
    } // for all  MDW's

    _layoutMDW->activate();
}

void ViewSliders::refreshVolumeLevels()
{
    for ( int i=0; i<_mdws.count(); i++ )
    {
        QWidget *mdwx = _mdws[i];

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
	  bool debugMe = (mdw->mixDevice()->id() == "PCM:0" );
	  if (debugMe) kDebug() << "Old PCM:0 playback state" << mdw->mixDevice()->isMuted()
	    << ", vol=" << mdw->mixDevice()->playbackVolume().getAvgVolumePercent(Volume::MALL);

                mdw->update();
            }
            else {
                kError(67100) << "ViewSliders::refreshVolumeLevels(): mdw is not a MixDeviceWidget\n";
                // no slider. Cannot happen in theory => skip it
            }
    }
}

#include "viewsliders.moc"
