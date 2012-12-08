/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken <esken@kde.org>
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

#include "mixer_backend.h"

#include <klocale.h>

// for the "ERR_" declartions, #include mixer.h
#include "core/mixer.h"
#include "core/ControlManager.h"

#include <QTimer>

#define POLL_OSS_RATE_SLOW 1500
#define POLL_OSS_RATE_FAST 50


#include "mixer_backend_i18n.cpp"

Mixer_Backend::Mixer_Backend(Mixer *mixer, int device) :
m_devnum (device) , m_isOpen(false), m_recommendedMaster(), _mixer(mixer), _pollingTimer(0)

{
	// In all cases create a QTimer. We will use it once as a singleShot(), even if something smart
	// like ::select() is possible (as in ALSA).
	_pollingTimer = new QTimer(); // will be started on open() and stopped on close()
	connect( _pollingTimer, SIGNAL(timeout()), this, SLOT(readSetFromHW()), Qt::QueuedConnection);

}

void Mixer_Backend::closeCommon()
{
	freeMixDevices();
}

int Mixer_Backend::close()
{
	kDebug() << "Implicit close on " << this << ". Please instead call closeCommon() and close() explicitly (in concrete Backend destructor)";
	// ^^^ Background. before the destructor runs, the C++ runtime changes the virtual pointers to point back
	//     to the common base class. So what actually runs is not run Mixer_ALSA::close(), but this method.
	//
	//     Comment: IMO this is totally stupid and insane behavior of C++, because you cannot simply cannot call
	//              the overwritten (cleanup) methods in the destructor.
	return 0;
}

Mixer_Backend::~Mixer_Backend()
{
	if (!m_mixDevices.isEmpty())
	{
		kDebug() << "Implicit close on " << this << ". Please instead call closeCommon() and close() explicitly (in concrete Backend destructor)";
	}
	kDebug() << "Destruct " << this;
// 	qDebug() << "Running Mixer_Backend destructor";
	delete _pollingTimer;
}

void Mixer_Backend::freeMixDevices()
{
	foreach (shared_ptr<MixDevice> md, m_mixDevices)
		md->close();

	m_mixDevices.clear();
}

bool Mixer_Backend::openIfValid() {
	bool valid = false;
	int ret = open();
	if ( ret == 0  && (m_mixDevices.count() > 0 || _mixer->isDynamic())) {
		valid = true;
		// A better ID is now calculated in mixertoolbox.cpp, and set via setID(),
		// but we want a somehow usable fallback just in case.

		if ( needsPolling() ) {
			_pollingTimer->start(POLL_OSS_RATE_FAST);
		}
		else {
			// The initial state must be read manually
			QTimer::singleShot( POLL_OSS_RATE_FAST, this, SLOT(readSetFromHW()) );
		}
	} // could be opened
	else
	{
		//shutdown();
	}
	return valid;
}

bool Mixer_Backend::isOpen() {
	return m_isOpen;
}

/**
 * Queries the backend driver whether there are new changes in any of the controls.
 * If you cannot find out for a backend, return "true" - this is also the default implementation.
 * @return true, if there are changes. Otherwise false is returned.
 */
bool Mixer_Backend::prepareUpdateFromHW() {
	return true;
}

/**
 * The name of the Mixer this backend represents.
 * Often it is just a name/id for the kernel. so name and id are usually identical. Virtual/abstracting backends are
 * different, as they represent some distinct function like "Application streams" or "Capture Devices". Also backends
 * that do not have names might can to set ID and name different like i18n("SUN Audio") and "SUNAudio".
 */
QString Mixer_Backend::getName() const
{
	return m_mixerName;
}

/**
 * The id of the Mixer this backend represents. The default implementation simply returns the name.
 * Often it is just a name/id for the kernel. so name and id are usually identical. See also #Mixer_Backend::getName().
 * You must override this method if you want to set ID different from name.
 */
QString Mixer_Backend::getId() const
{
	return m_mixerName; // Backwards compatibility. PulseAudio overrides it.
}

/**
 * After calling this, readSetFromHW() will do a complete update. This will
 * trigger emitting the appropriate signals like controlChanged().
 *
 * This method is useful, if you need to get a "refresh signal" - used at:
 * 1) Start of KMix - so that we can be sure an initial signal is emitted
 * 2) When reconstructing any MixerWidget (e.g. DockIcon after applying preferences)
 */
void Mixer_Backend::readSetFromHWforceUpdate() const {
	_readSetFromHWforceUpdate = true;
}


/**
 * You can call this to retrieve the freshest information from the mixer HW.
 * This method is also called regularly by the mixer timer.
 */
void Mixer_Backend::readSetFromHW()
{
	bool updated = prepareUpdateFromHW();
	if ( (! updated) && (! _readSetFromHWforceUpdate) ) {
		// Some drivers (ALSA) are smart. We don't need to run the following
		// time-consuming update loop if there was no change
		kDebug(67100) << "Mixer::readSetFromHW(): smart-update-tick";
		return;
	}

	_readSetFromHWforceUpdate = false;

	int ret = Mixer::OK_UNCHANGED;

	foreach (shared_ptr<MixDevice> md, m_mixDevices )
	{
	  bool debugMe = (md->id() == "PCM:0" );
	  if (debugMe) kDebug() << "Old PCM:0 playback state" << md->isMuted()
	    << ", vol=" << md->playbackVolume().getAvgVolumePercent(Volume::MALL);
	    
		int retLoop = readVolumeFromHW( md->id(), md );
	  if (debugMe) kDebug() << "New PCM:0 playback state" << md->isMuted()
	    << ", vol=" << md->playbackVolume().getAvgVolumePercent(Volume::MALL);
		if (md->isEnum() )
		{
			/*
			 * This could be reworked:
			 * Plan: Read everything (incuding enum's) in readVolumeFromHW().
			 * readVolumeFromHW() should then be renamed to readHW().
			 */
			md->setEnumId( enumIdHW(md->id()) );
		}
		if ( retLoop == Mixer::OK && ret == Mixer::OK_UNCHANGED )
		{
			ret = Mixer::OK;
		}
		else if ( retLoop != Mixer::OK && retLoop != Mixer::OK_UNCHANGED )
		{
			ret = retLoop;
		}
	}

	if ( ret == Mixer::OK )
	{
		// We explicitely exclude Mixer::OK_UNCHANGED and Mixer::ERROR_READ
		if ( needsPolling() )
		{
			_pollingTimer->setInterval(POLL_OSS_RATE_FAST);
			QTime fastPollingEndsAt = QTime::currentTime ();
			fastPollingEndsAt = fastPollingEndsAt.addSecs(5);
			_fastPollingEndsAt = fastPollingEndsAt;
			//_fastPollingEndsAt = fastPollingEndsAt;
			kDebug() << "Start fast polling from " << QTime::currentTime() <<"until " << _fastPollingEndsAt;
		}

		kDebug() << "Announcing the readSetFromHW()";
		ControlManager::instance().announce(_mixer->id(), ControlChangeType::Volume, QString("Mixer.fromHW"));
	}

	else
	{
		// This code path is entered on Mixer::OK_UNCHANGED and ERROR
		if ( !_fastPollingEndsAt.isNull() )
		{
			// Fast polling is currently active
			if( _fastPollingEndsAt < QTime::currentTime () )
			{
				kDebug() << "End fast polling";
				_fastPollingEndsAt = QTime();
				if ( needsPolling() )
					_pollingTimer->setInterval(POLL_OSS_RATE_SLOW);
			}
		}
	}
}

/**
 * Return the MixDevice, that would qualify best as MasterDevice. The default is to return the
 * first device in the device list. Backends can override this (i.e. the ALSA Backend does so).
 * The users preference is NOT returned by this method - see the Mixer class for that.
 */
shared_ptr<MixDevice> Mixer_Backend::recommendedMaster()
{
	if ( m_recommendedMaster != 0 )
	{
		// Backend has set a recommended master. Thats fine. Using it.
		return m_recommendedMaster;
	}
	else if ( ! m_mixDevices.isEmpty() )
	{
		// Backend has NOT set a recommended master. Evil backend
		// => lets help out, using the first device (if exists)
		return m_mixDevices.at(0);
	}
	else
	{
		if ( !_mixer->isDynamic())
			// This should never ever happen, as KMix does NOT accept soundcards without controls
			kError(67100) << "Mixer_Backend::recommendedMaster(): returning invalid master. This is a bug in KMix. Please file a bug report stating how you produced this." << endl;
	}

	// If we reach this code path, then obiously m_recommendedMaster == 0 (see above)
	return m_recommendedMaster;

}

/**
 * Sets the ID of the currently selected Enum entry.
 * This is a dummy implementation - if the Mixer backend
 * wants to support it, it must implement the driver specific 
 * code in its subclass (see Mixer_ALSA.cpp for an example).
 */
void Mixer_Backend::setEnumIdHW(const QString& , unsigned int) {
	return;
}

/**
 * Return the ID of the currently selected Enum entry.
 * This is a dummy implementation - if the Mixer backend
 * wants to support it, it must implement the driver specific
 * code in its subclass (see Mixer_ALSA.cpp for an example).
 */
unsigned int Mixer_Backend::enumIdHW(const QString& ) {
	return 0;
}

/**
 * Move the stream to a new destination
 */
bool Mixer_Backend::moveStream( const QString& id, const QString& destId ) {
	Q_UNUSED(id);
	Q_UNUSED(destId);
	return false;
}

void Mixer_Backend::errormsg(int mixer_error)
{
	QString l_s_errText;
	l_s_errText = errorText(mixer_error);
	kError() << l_s_errText << "\n";
}

int Mixer_Backend::id2num(const QString& id)
{
	return id.toInt();
}

QString Mixer_Backend::errorText(int mixer_error)
{
	QString l_s_errmsg;
	switch (mixer_error)
	{
	case Mixer::ERR_PERM:
		l_s_errmsg = i18n("kmix:You do not have permission to access the mixer device.\n" \
				"Please check your operating systems manual to allow the access.");
		break;
	case Mixer::ERR_WRITE:
		l_s_errmsg = i18n("kmix: Could not write to mixer.");
		break;
	case Mixer::ERR_READ:
		l_s_errmsg = i18n("kmix: Could not read from mixer.");
		break;
	case Mixer::ERR_OPEN:
		l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
				"Please check that the soundcard is installed and that\n" \
				"the soundcard driver is loaded.\n");
		break;
	default:
		l_s_errmsg = i18n("kmix: Unknown error. Please report how you produced this error.");
		break;
	}
	return l_s_errmsg;
}




#include "mixer_backend.moc"
