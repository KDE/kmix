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
 * Software Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "viewsliderset.h"
#include <qwidget.h>

#include "guiprofile.h"
#include "mixer.h"
#include "mixdevicewidget.h"

ViewSliderSet::ViewSliderSet(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof)
      : ViewSliders(parent, name, mixer, vflags, guiprof)
{
    _guiprof = guiprof;
    init();
    connect ( _mixer, SIGNAL(newRecsrc()), this, SLOT(refreshVolumeLevels()) ); // it is possible that this view will contain record sources
}

ViewSliderSet::~ViewSliderSet() {
}

void ViewSliderSet::setMixSet(MixSet *mixset)
{
	// This method iterates the controls from the Profile
	// Each control is checked, whether it is also contained in the mixset, and
	// applicable for this kind of View. If yes, the control is accepted and inserted.

	std::vector<ProfControl*>::const_iterator itEnd = _guiprof->_controls.end();
	for ( std::vector<ProfControl*>::const_iterator it = _guiprof->_controls.begin(); it != itEnd; ++it)
	{
	    ProfControl* control = *it;
	    if ( control->tab == name() ) {   //  !!! using name() here is BAD. An ID should be used
		// The TabName of the control matches this View name (!! attention: Better use some ID, due to i18n() )
		bool isUsed = false;

		// The following for-loop could be simplified by using a std::find_if
		for ( int i=0; i<mixset->count(); i++ ) {
			MixDevice *md = (*mixset)[i];
			if (	md->getPK() == control->id  &&      // name matches
				! md->isSwitch() && ! md->isEnum()  // and is applicable
			)
			{
			    // OK, this control is handable by this View.
			    _mixSet->append(md);
			    isUsed = true;
			    break;
			}
		} // loop for finding a suitable MixDevice
		if ( ! isUsed ) {
			// There is something in the Profile, that doesn't correspond to a Mixer control
			kdDebug(67100) << "ViewSliderSet::setMixSet(): No such control '" << control->id << "'in the mixer. Please check the GUIProfile\n";
		}
	    } // Tab name matches
	    else {
	    }  // Tab name doesn't match (=> don't insert)
	} // iteration over all controls from the Profile
}

#include "viewsliderset.moc"
