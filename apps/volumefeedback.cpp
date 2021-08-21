/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2021 Jonathan Marten <jjm@keelhaul.me.uk>
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
 * License along with this program; if not, see
 * <https://www.gnu.org/licenses>.
 */

#include "volumefeedback.h"

// Qt
#include <QTimer>

// KDE
#include <klocalizedstring.h>

// KMix
#include "kmix_debug.h"
#include "core/mixer.h"
#include "settings.h"

// Others
extern "C"
{
#include <canberra.h>
}

// The Canberra API is described at
// https://developer.gnome.org/libcanberra/unstable/libcanberra-canberra.html


VolumeFeedback *VolumeFeedback::instance()
{
	static VolumeFeedback *sInstance = new VolumeFeedback;
	return (sInstance);
}


VolumeFeedback::VolumeFeedback()
{
	qCDebug(KMIX_LOG);
	m_currentMaster = nullptr;
	m_veryFirstTime = true;

	int ret = ca_context_create(&m_ccontext);
	if (ret<0)
	{
                qCDebug(KMIX_LOG) << "Canberra context create failed, volume feedback unavailable - " << ca_strerror(ret);
                m_ccontext = nullptr;
		return;
	}

	m_feedbackTimer = new QTimer(this);
	m_feedbackTimer->setSingleShot(true);
	// This timer interval should be longer than the expected duration of the
	// feedback sound, so that multiple sounds do not overlap or blend into
	// a continuous sound.  However, it should be short so as to be responsive
	// to the user's actions.  The freedesktop theme sound "audio-volume-change"
	// is about 70 milliseconds long.
	m_feedbackTimer->setInterval(150);
	connect(m_feedbackTimer, &QTimer::timeout, this, &VolumeFeedback::slotPlayFeedback);

	ControlManager::instance().addListener(QString(),			// any mixer
					       ControlManager::MasterChanged,	// type of change
					       this,				// receiver
					       "VolumeFeedback (master)");	// source ID
}


VolumeFeedback::~VolumeFeedback()
{
	if (m_ccontext!=nullptr) ca_context_destroy(m_ccontext);
}


void VolumeFeedback::init()
{
	masterChanged();
}


void VolumeFeedback::controlsChange(ControlManager::ChangeType changeType)
{
	switch (changeType)
	{
case ControlManager::MasterChanged:
		masterChanged();
		break;

case ControlManager::Volume:
		if (m_currentMaster==nullptr) return;		// no current master device
		if (!Settings::beepOnVolumeChange()) return;	// feedback sound not wanted
		volumeChanged();				// check volume and play sound
		break;

default:	ControlManager::warnUnexpectedChangeType(changeType, this);
		break;
	}
}


void VolumeFeedback::volumeChanged()
{
	const Mixer *m = Mixer::getGlobalMasterMixer();		// current global master
	const shared_ptr<MixDevice> md = m->getLocalMasterMD();	// its master device
	int newvol = md->userVolumeLevel();			// current volume level
	//qCDebug(KMIX_LOG) << m_currentVolume << "->" << newvol;

	if (newvol==m_currentVolume) return;			// volume has not changes
	m_feedbackTimer->start();				// restart the timer
	m_currentVolume = newvol;				// note new current volume
}


void VolumeFeedback::masterChanged()
{
	const Mixer *globalMaster = Mixer::getGlobalMasterMixer();
	if (globalMaster==nullptr)
	{
		qCDebug(KMIX_LOG) << "no current global master";
		m_currentMaster.clear();
		return;
	}

	const shared_ptr<MixDevice> md = globalMaster->getLocalMasterMD();
	if (md==nullptr)
	{
		qCDebug(KMIX_LOG) << "global master doest have a local master MD ( MixDevice )";
		m_currentMaster.clear();
		return;
	}
	const Volume &vol = md->playbackVolume();
	if (!vol.hasVolume())
	{
		qCDebug(KMIX_LOG) << "device" << md->id() << "has no playback volume";
		m_currentMaster.clear();
		return;
	}

	// Make a unique name for the mixer and master device.
	const QString masterId = globalMaster->id()+"|"+md->id();
	// Then check whether it is the same as already recorded.
	if (masterId==m_currentMaster)
	{
		qCDebug(KMIX_LOG) << "current master is already" << m_currentMaster;
		return;
	}

	qCDebug(KMIX_LOG) << "from" << (m_currentMaster.isEmpty() ? "(none)" : m_currentMaster)
			  << "to" << masterId;
	m_currentMaster = masterId;

	// Remove only the listener for ControlManager::Volume,
	// retaining the one for ControlManager::MasterChanged.
	ControlManager::instance().removeListener(this, ControlManager::Volume, "VolumeFeedback");

	// Then monitor for a volume change on the new master
	ControlManager::instance().addListener(globalMaster->id(),		// mixer ID
					       ControlManager::Volume,		// type of change
					       this,				// receiver
					       "VolumeFeedback (volume)");	// source ID

	// Set the Canberra driver to match the master device.
	// I can't seem to find any documentation on the driver
	// names that are supported, so this is just a guess based
	// on the name set by original PulseAudio implementation.
	QString driver = globalMaster->getDriverName().toLower();
	if (driver=="pulseaudio") driver = "pulse";
	qCDebug(KMIX_LOG) << "Setting Canberra driver to" << driver;
	ca_context_set_driver(m_ccontext, driver.toLocal8Bit());

	// Similarly, this is just a guess based on the existing
	// PulseAudio and ALSA support.  All existing backends
	// set the UDI to the equivalent of the hardware device
	// name.
	QString device = globalMaster->udi();
	if (!device.isEmpty())
	{
		qCDebug(KMIX_LOG) << "Setting Canberra device to" << device;
		ca_context_change_device(m_ccontext, device.toLocal8Bit());
	}

	m_currentVolume = -1;				// always make a sound after change
	controlsChange(ControlManager::Volume);		// simulate a volume change
}


// Originally taken from Mixer_PULSE::writeVolumeToHW()

void VolumeFeedback::slotPlayFeedback()
{
	if (m_ccontext==nullptr) return;		// Canberra is not initialised

	// Inhibit the very first feedback sound after KMix has started.
	// Otherwise it will be played during desktop startup, possibly
	// interfering with the login sound and definitely confusing users.
	if (m_veryFirstTime)
	{
		m_veryFirstTime = false;
		return;
	}

	int playing = 0;
	// Note that '2' is simply an index we've picked.
	// It's mostly irrelevant.
	int cindex = 2;

	ca_context_playing(m_ccontext, cindex, &playing);

	// Note: Depending on how this is desired to work,
	// we may want to simply skip playing, or cancel the
	// currently playing sound and play our
	// new one... for now, let's do the latter.
	if (playing)
	{
		ca_context_cancel(m_ccontext, cindex);
		playing = 0;
	}

	if (playing==0)
	{
		// ca_context_set_driver() and ca_context_change_device()
		// have already been done in masterChanged() above.

		// Ideally we'd use something like ca_gtk_play_for_widget()...
		ca_context_play(
			m_ccontext,
			cindex,
			CA_PROP_EVENT_DESCRIPTION, i18n("Volume Control Feedback Sound").toUtf8().constData(),
			CA_PROP_EVENT_ID, "audio-volume-change",
			CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
			CA_PROP_CANBERRA_ENABLE, "1",
			nullptr);

		ca_context_change_device(m_ccontext, nullptr);
	}
}
