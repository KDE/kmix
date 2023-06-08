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

#include "mixerbackend.h"

#include <klocalizedstring.h>

// for the "ERR_" declarations, #include mixer.h
#include "core/mixer.h"
#include "core/ControlManager.h"


#define POLL_RATE_SLOW 1500
#define POLL_RATE_FAST 50


MixerBackend::MixerBackend(Mixer *mixer, int device) :
m_devnum (device) , m_isOpen(false), m_recommendedMaster(), _mixer(mixer), _pollingTimer(0), _cardInstance(1), _cardRegistered(false)

{
	// In all cases create a QTimer. We will use it once as a singleShot(), even if something smart
	// like ::select() is possible (as in ALSA). And force to do an update.
	_readSetFromHWforceUpdate = true;
	_pollingTimer = new QTimer(); // will be started on open() and stopped on close()
	connect( _pollingTimer, SIGNAL(timeout()), this, SLOT(readSetFromHW()), Qt::QueuedConnection);

}

void MixerBackend::closeCommon()
{
	freeMixDevices();
}

int MixerBackend::close()
{
	qCDebug(KMIX_LOG) << "Implicit close on " << this << ". Please instead call closeCommon() and close() explicitly (in concrete Backend destructor)";
	// ^^^ Background. before the destructor runs, the C++ runtime changes the virtual pointers to point back
	//     to the common base class. So what actually runs is not run Mixer_ALSA::close(), but this method.
	//
	//     See https://stackoverflow.com/questions/99552/where-do-pure-virtual-function-call-crashes-come-from?lq=1
	//
	//     Comment: IMO this is totally stupid and insane behavior of C++, because you cannot simply cannot call
	//              the overwritten (cleanup) methods in the destructor.
	return 0;
}

MixerBackend::~MixerBackend()
{
	unregisterCard(this->getName());
	if (!m_mixDevices.isEmpty())
	{
		qCDebug(KMIX_LOG) << "Implicit close on " << this << ". Please instead call closeCommon() and close() explicitly (in concrete Backend destructor)";
	}
	delete _pollingTimer;
}

void MixerBackend::freeMixDevices()
{
	for (shared_ptr<MixDevice> md : std::as_const(m_mixDevices)) md->close();
	m_mixDevices.clear();
}


bool MixerBackend::openIfValid()
{
	const int ret = open();
	if (ret!=0)
	{
		//qCWarning(KMIX_LOG) << "open" << getName() << "failed" << ret;
		return false;				// could not open
	}

	qCDebug(KMIX_LOG) << "opened" << getName() <<  "count" << m_mixDevices.count()
			  << "dynamic?" << _mixer->isDynamic() << "needsPolling?" << needsPolling();
	if (m_mixDevices.count() > 0 || _mixer->isDynamic())
	{
		if (needsPolling())
		{
			_pollingTimer->start(POLL_RATE_FAST);
		}
		else
		{
			// The initial state must be read manually
			QTimer::singleShot( POLL_RATE_FAST, this, SLOT(readSetFromHW()));
		}
		return true;				// could be opened
	}
	else
	{
		qCWarning(KMIX_LOG) << "no mix devices and not dynamic";
		return false;				// could not open
	}
}


bool MixerBackend::isOpen() {
	return m_isOpen;
}

/**
 * Queries the backend driver whether there are new changes in any of the controls.
 * If you cannot find out for a backend, return "true" - this is also the default implementation.
 * @return true, if there are changes. Otherwise false is returned.
 */
bool MixerBackend::hasChangedControls()
{
	return true;
}

/**
 * The name of the Mixer this backend represents.
 * Often it is just a name/id for the kernel. so name and id are usually identical. Virtual/abstracting backends are
 * different, as they represent some distinct function like "Application streams" or "Capture Devices". Also backends
 * that do not have names might can to set ID and name different like i18n("SUN Audio") and "SUNAudio".
 */
QString MixerBackend::getName() const
{
	return m_mixerName;
}

/**
 * The id of the Mixer this backend represents. The default implementation simply returns the name.
 * Often it is just a name/id for the kernel. so name and id are usually identical. See also
 * MixerBackend::getName().
 * You must override this method if you want to set ID different from name.
 */
QString MixerBackend::getId() const
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
void MixerBackend::readSetFromHWforceUpdate() const
{
	_readSetFromHWforceUpdate = true;
}


/**
 * You can call this to retrieve the freshest information from the mixer HW.
 * This method is also called regularly by the mixer timer.
 */
void MixerBackend::readSetFromHW()
{
	bool updated = hasChangedControls();
	if ( (! updated) && (! _readSetFromHWforceUpdate) ) {
		// Some drivers (ALSA) are smart. We don't need to run the following
		// time-consuming update loop if there was no change
		qCDebug(KMIX_LOG) << "smart-update-tick";
		return;
	}

	_readSetFromHWforceUpdate = false;

	int ret = Mixer::OK_UNCHANGED;

	for (shared_ptr<MixDevice> md : std::as_const(m_mixDevices))
	{
	  //bool debugMe = (md->id() == "PCM:0" );
	  bool debugMe = false;
	  if (debugMe) qCDebug(KMIX_LOG) << "Old PCM:0 playback state" << md->isMuted()
	    << ", vol=" << md->playbackVolume().getAvgVolumePercent(Volume::MALL);
	    
		int retLoop = readVolumeFromHW( md->id(), md );
	  if (debugMe) qCDebug(KMIX_LOG) << "New PCM:0 playback state" << md->isMuted()
	    << ", vol=" << md->playbackVolume().getAvgVolumePercent(Volume::MALL);
		if (md->isEnum() )
		{
			/*
			 * This could be reworked:
			 * Plan: Read everything (including enum's) in readVolumeFromHW().
			 * readVolumeFromHW() should then be renamed to readHW().
			 */
			md->setEnumId( enumIdHW(md->id()) );
		}

		// Transition the outer return value with the value from this loop iteration
		if ( retLoop == Mixer::OK && ret == Mixer::OK_UNCHANGED )
		{
			// Unchanged => OK (Changed)
			ret = Mixer::OK;
		}
		else if ( retLoop != Mixer::OK && retLoop != Mixer::OK_UNCHANGED )
		{
			// If current ret from loop in not OK, then transition to that: ret (Something) => retLoop (Error)
			ret = retLoop;
		}
	}

	if ( ret == Mixer::OK )
	{
		// We explicitly exclude Mixer::OK_UNCHANGED and Mixer::ERROR_READ
		if ( needsPolling() )
		{
			// Upgrade polling frequency temporarily to be more smoooooth
			_pollingTimer->setInterval(POLL_RATE_FAST);
			QTime fastPollingEndsAt = QTime::currentTime ();
			fastPollingEndsAt = fastPollingEndsAt.addSecs(5);
			_fastPollingEndsAt = fastPollingEndsAt;
			//_fastPollingEndsAt = fastPollingEndsAt;
			qCDebug(KMIX_LOG) << "Start fast polling from " << QTime::currentTime() <<"until " << _fastPollingEndsAt;
		}

		ControlManager::instance()->announce(_mixer->id(), ControlManager::Volume, QString("Mixer.fromHW"));
	}

	else
	{
		// This code path is entered on Mixer::OK_UNCHANGED and ERROR
		bool fastPollingEndsNow = (!_fastPollingEndsAt.isNull()) && _fastPollingEndsAt < QTime::currentTime ();
		if ( fastPollingEndsNow )
		{
			qCDebug(KMIX_LOG) << "End fast polling";
			_fastPollingEndsAt = QTime(); // NULL time
			_pollingTimer->setInterval(POLL_RATE_SLOW);
		}
	}
}

/**
 * Return the MixDevice, that would qualify best as MasterDevice. The default is to return the
 * first device in the device list. Backends can override this (i.e. the ALSA Backend does so).
 * The users preference is NOT returned by this method - see the Mixer class for that.
 */
shared_ptr<MixDevice> MixerBackend::recommendedMaster()
{
	if ( m_recommendedMaster )
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
			qCCritical(KMIX_LOG) << "MixerBackend::recommendedMaster(): returning invalid master. This is a bug in KMix. Please file a bug report stating how you produced this.";
	}

	// If we reach this code path, then obviously m_recommendedMaster == 0 (see above)
	return m_recommendedMaster;

}

/**
 * Sets the ID of the currently selected Enum entry.
 * This is a dummy implementation - if the Mixer backend
 * wants to support it, it must implement the driver specific 
 * code in its subclass (see Mixer_ALSA.cpp for an example).
 */
void MixerBackend::setEnumIdHW(const QString& , unsigned int) {
	return;
}

/**
 * Return the ID of the currently selected Enum entry.
 * This is a dummy implementation - if the Mixer backend
 * wants to support it, it must implement the driver specific
 * code in its subclass (see Mixer_ALSA.cpp for an example).
 */
unsigned int MixerBackend::enumIdHW(const QString& ) {
	return 0;
}


/**
 * Move the stream to a new destination
 */
/* virtual */ bool MixerBackend::moveStream(const QString &id, const QString &destId)
{
	qCDebug(KMIX_LOG) << "called for unsupported" << id;
	Q_UNUSED(destId);
	return (false);
}

/**
 * Get the current destination device of a stream
 */
/* virtual */ QString MixerBackend::currentStreamDevice(const QString &id) const
{
	qCDebug(KMIX_LOG) << "called for unsupported" << id;
	return (QString());
}


/* virtual */ QString MixerBackend::errorText(int mixer_error)
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


/* virtual */ QString MixerBackend::translateKernelToWhatsthis(const QString &kernelName) const
{
        if (kernelName == "Mic:0") return (i18n("Recording level of the microphone input."));
	else if (kernelName == "Master:0") return (i18n("Controls the volume of the front speakers or all speakers (depending on your soundcard model). If you use a digital output, you might need to also use other controls like ADC or DAC. For headphones, soundcards often supply a Headphone control."));
	else if (kernelName == "PCM:0") return (i18n("Most media, such as MP3s or Videos, are played back using the PCM channel. As such, the playback volume of such media is controlled by both this and the Master or Headphone channels."));
	else if (kernelName == "Headphone:0") return (i18n("Controls the headphone volume. Some soundcards include a switch that must be manually activated to enable the headphone output."));
	else return (i18n("---"));
}


/**
 * Registers the card for this backend and sets the card discriminator for the
 * given card name.
 *
 * You MUST call this before creating the first @c MixDevice.  The reason is that
 * each @c MixDevice instance registers a DBUS name that includes the mixer ID
 * (and this means also the @c _cardInstance).
 *
 * The discriminator should always be 1, unless a second card with the same name
 * as a registered card was already registered.  The default implementation will
 * return 2, 3 and so on for more cards.  Subclasses can override this and return
 * arbitrary IDs, but any ID that is not 1 will be displayed to the user everywhere
 * where a mixer name is shown, like in the tab name.
 *
 * For the background please see BKO-327471 and read the following info:
 * "Count mixer nums for every mixer name to identify mixers with equal names.
 * This is for creating persistent (reusable) primary keys, which can safely
 * be referenced (especially for config file access, so it is meant to be
 * persistent!)."
 *
 * @param cardBaseName The base mixer name for the card
 */
void MixerBackend::registerCard(const QString &cardBaseName)
{
	m_mixerName = cardBaseName;
	int cardDiscriminator = 1 + m_mixerNums[cardBaseName];
	qCDebug(KMIX_LOG) << "cardBaseName=" << cardBaseName << "cardDiscriminator=" << cardDiscriminator;
	_cardInstance = cardDiscriminator;
	_cardRegistered = true;
}


/**
 * Unregisters the card of this backend.
 *
 * The card discriminator counter for this card name is reduced by 1.
 * See @c registerCard() for more information.
 *
 * TODO: This is not entirely correct.  For example, if the first card
 * (cardDiscrimiator==1) is unpluggged, then @c m_mixerNums["cardName"] is
 * changed from 2 to 1. The next plug of registerCard("cardName") will use
 * a @c cardDiscriminator of 2, but the card with that discriminator was not
 * unplugged => BANG!!!
 *
 * @param cardBaseName The base mixer name of the card
 */
void MixerBackend::unregisterCard(const QString &cardBaseName)
{
	const QMap<QString,int>::const_iterator it = m_mixerNums.constFind(cardBaseName);
	if (it != m_mixerNums.constEnd())
	{
		int beforeValue = it.value();
		int afterValue = beforeValue-1;
		if (beforeValue>0) m_mixerNums[cardBaseName] = afterValue;
		qCDebug(KMIX_LOG) << "beforeValue=" << beforeValue << "afterValue" << afterValue;
	}
}
