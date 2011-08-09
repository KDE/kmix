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

#include <klocale.h>

#include "mixer_backend.h"
// for the "ERR_" declartions, #include mixer.h
#include "core/mixer.h"
#include <QTimer>

#include "mixer_backend_i18n.cpp"

Mixer_Backend::Mixer_Backend(Mixer *mixer, int device) :
m_devnum (device) , m_isOpen(false), m_recommendedMaster(0), _mixer(mixer), _pollingTimer(0)

{
	// In all cases create a QTimer. We will use it once as a singleShot(), even if something smart
	// like ::select() is possible (as in ALSA).
	_pollingTimer = new QTimer(); // will be started on open() and stopped on close()
	connect( _pollingTimer, SIGNAL(timeout()), this, SLOT(readSetFromHW()));
}

Mixer_Backend::~Mixer_Backend()
{
	delete _pollingTimer;
	qDeleteAll(m_mixDevices);
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
			_pollingTimer->start(500);
		}
		else {
			// The initial state must be read manually
			QTimer::singleShot( 50, this, SLOT(readSetFromHW()) );
		}
	} // cold be opened
	else {
		close();
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
   You can call this to retrieve the freshest information from the mixer HW.
   This method is also called regulary by the mixer timer.
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

	//kDebug() << "---tick---" << QTime::currentTime();
	_readSetFromHWforceUpdate = false;

	int ret = Mixer::OK_UNCHANGED;

	int mdCount = m_mixDevices.count();
	for(int i=0; i<mdCount  ; ++i )
	{
		MixDevice *md = m_mixDevices[i];
		int retLoop = readVolumeFromHW( md->id(), md );
		if (md->isEnum() ) {
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
		_pollingTimer->setInterval(50);
		QTime fastPollingEndsAt = QTime::currentTime ();
		fastPollingEndsAt = fastPollingEndsAt.addSecs(5);
		_fastPollingEndsAt = fastPollingEndsAt;
		//_fastPollingEndsAt = fastPollingEndsAt;
		kDebug() << "Start fast polling from " << QTime::currentTime() <<"until " << _fastPollingEndsAt;
		emit controlChanged();
	}

	else
	{
		// This code path is entered on Mixer::OK_UNCHANGED and ERROR
		if ( !_fastPollingEndsAt.isNull() )
		{
			if( _fastPollingEndsAt < QTime::currentTime () )
			{
				kDebug() << "End fast polling";
				_fastPollingEndsAt = QTime();
				_pollingTimer->setInterval(500);
			}
		}
	}
}

/**
 * Return the MixDevice, that would qualify best as MasterDevice. The default is to return the
 * first device in the device list. Backends can override this (i.e. the ALSA Backend does so).
 * The users preference is NOT returned by this method - see the Mixer class for that.
 */
MixDevice* Mixer_Backend::recommendedMaster() {
	if ( m_recommendedMaster != 0 ) {
		return m_recommendedMaster;   // Backend has set a recommended master. Thats fine.
	} // recommendation from Backend
	else if ( m_mixDevices.count() > 0 ) {
		return m_mixDevices.at(0);  // Backend has NOT set a recommended master. Evil backend => lets help out.
	} //first device (if exists)
	else {
		if ( !_mixer->isDynamic()) {
			// This should never ever happen, as KMix doe NOT accept soundcards without controls
			kError(67100) << "Mixer_Backend::recommendedMaster(): returning invalid master. This is a bug in KMix. Please file a bug report stating how you produced this." << endl;
		}
		return (MixDevice*)0;
	}
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
