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
