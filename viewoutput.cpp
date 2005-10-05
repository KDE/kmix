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

#include "viewoutput.h"
#include <qwidget.h>

#include "mixer.h"
#include "mixdevicewidget.h"

/*
 * View, showing only "output" sliders.
 * This View is deprecated, and ViewSliders will be removed once ViewSliderSet is fully operational
 * (only thing missing in ViewSliderSet is the evaluation of the "controls" information from the Profile.)
 */
ViewOutput::ViewOutput(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof)
      : ViewSliders(parent, name, mixer, vflags, guiprof)
{
    init();
}

ViewOutput::~ViewOutput() {
}

void ViewOutput::setMixSet(MixSet *mixset)
{
    for ( int i=0; i<mixset->count(); i++ ) {
	MixDevice *md = (*mixset)[i];
	if ( ! md->isRecordable() &&  ! md->isSwitch() && ! md->isEnum()) {
	    _mixSet->append(md);
	}
	else {
	}
    }
}


#include "viewoutput.moc"
