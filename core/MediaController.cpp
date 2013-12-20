//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 1996-2014 The KMix authors. Maintainer: Christian Esken <esken@kde.org>
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


/*
 * MediaController.cpp
 *
 *  Created on: 17.12.2013
 *      Author: chris
 */

#include "core/MediaController.h"

//#include <phonon/audiooutput.h>
//#include <phonon/backendcapabilities.h>

#include <KDebug>

MediaController::MediaController(QString controlId) :
	id(controlId), playState(PlayUnknown)
{
    mediaPlayControl = false;
    mediaNextControl = false;
    mediaPrevControl = false;

    /*
	{
		// Phonon connection test code
		QList<Phonon::AudioOutputDevice> devs = Phonon::BackendCapabilities::availableAudioOutputDevices();

		if (devs.isEmpty())
			return;

		Phonon::AudioOutputDevice& dev = devs[0];

		QList<QByteArray> props = dev.propertyNames();
		kDebug() << "desc=" << dev.description() << ", name=" << dev.name() << ", props=";
		QByteArray prop;
		int i=0;
		foreach (prop, props)
		{
			kDebug() << "#"  << i << ": "<< prop;
			++i;
		}
	}
	*/
}

MediaController::~MediaController()
{
}

/**
 * Returns whether this device has at least one media player control.
 * @return
 */
bool MediaController::hasControls()
{
	return mediaPlayControl | mediaNextControl | mediaPrevControl;
}

MediaController::PlayState MediaController::getPlayState()
{
	return playState;
}

void MediaController::setPlayState(PlayState playState)
{
	this->playState = playState;
}
