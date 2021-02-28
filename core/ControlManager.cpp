/*
 KMix -- KDE's full featured mini mixer
 Copyright (C) 2012  Christian Esken <esken@kde.org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ControlManager.h"

#include "settings.h"
#include "kmix_debug.h"


struct Listener
{
	QString mixerId;
	ControlManager::ChangeType changeType;
	// TODO: recording 'target' is bit dangerous, as it might get deleted.
	// Use a smart pointer?
	QObject *target;
	QString sourceId;
};


ControlManager &ControlManager::instance()
{
	static ControlManager *sInstance = new ControlManager;
	return (*sInstance);
}


ControlManager::ControlManager()
{
	m_listenersChanged = false;
}


// TODO: this does not seem to implement the anti-loopback test as described in
// the original comments for addListener(), "Listeners are not informed about changes
// that originate from oneself (according to sourceId)".

void ControlManager::announce(const QString &mixerId, ControlManager::ChangeType changeType, const QString &sourceId)
{
	bool listenersModified = true;			// do loop at least once
	QSet<const Listener *> processedListeners;	// listeners that have been processed

	if (Settings::debugControlManager())
	{
		qCDebug(KMIX_LOG) << "Announcing" << changeType << "for mixer"
				  << (mixerId.isEmpty() ? "(all)" : mixerId)
				  << "by" << sourceId;
	}

	while (listenersModified)
	{
		listenersModified = false;		// unmodified so far this loop
		for (QList<Listener *>::const_iterator it = m_listeners.constBegin(); it!=m_listeners.constEnd(); ++it)
		{
			const Listener *listener = (*it);

			const bool listenerAlreadyProcessed = processedListeners.contains(listener);
			if (listenerAlreadyProcessed)
			{
				if (Settings::debugControlManager()) qCDebug(KMIX_LOG) << "Skipping already processed listener";
				continue;
			}

			const bool listenerInterested = listener->mixerId.isEmpty() ||	// listener wants all mixers
			                                mixerId.isEmpty() ||		// announce for all mixers
			                                listener->mixerId==mixerId;	// listener wants this mixer
			if (!listenerInterested) continue;				// don't want this listener
			if (listener->changeType!=changeType) continue;			// doesn't want this change type

			if (Settings::debugControlManager())
			{
				qCDebug(KMIX_LOG) << "Listener" << listener->sourceId
						  << "interested in" << mixerId
						  << "change type" << changeType;
			}

			bool success = QMetaObject::invokeMethod(listener->target,
								 "controlsChange",
								 Qt::DirectConnection,
								 Q_ARG(ControlManager::ChangeType, changeType));
			if (!success)
			{
				qCWarning(KMIX_LOG) << "failed to signal"
						    << listener->target->metaObject()->className();
			}

			processedListeners.insert(listener);
			if (m_listenersChanged)
			{
				// The invokeMethod() above has changed the list of
				// listeners, therefore the iterator may not be valid.
				// Restart the loop, but retain the list of processed
				// listeners so that those already processed are not
				// done again.
				if (Settings::debugControlManager()) qCDebug(KMIX_LOG) << "Listeners modified, restart loop";
				m_listenersChanged = false;
				listenersModified = true;
				break;			// break inner loop, continue outer loop
			}
		}					// inner loop
	}						// outer loop
}


void ControlManager::addListener(const QString &mixerId, ControlManager::ChangeTypes changeTypes,
				 QObject *target, const QString &sourceId)
{
	if (Settings::debugControlManager())
	{
		qCDebug(KMIX_LOG) << "Listening to" << changeTypes << "for mixer"
				  << (mixerId.isEmpty() ? "(all)" : mixerId)
				  << "by" << sourceId
				  << "target" << target->metaObject()->className();
	}

	for (ControlManager::ChangeType ct = ChangeType::First; ct!=ChangeType::Last;
	     ct = static_cast<ControlManager::ChangeType>(ct << 1))
	{
		// See if this change type is wanted.
		if (changeTypes & ct)
		{
			// Add a new listener for each wanted change type.
			Listener *listener = new Listener;
			listener->mixerId = mixerId;
			listener->changeType = ct;
			listener->target = target;
			listener->sourceId = sourceId;
			m_listeners.append(listener);
			// Note that the list of listeners has changed, in the event
			// that we are being called by a target of an announce loop
			// that is in progress.
			m_listenersChanged = true;
		}
	}

	if (Settings::debugControlManager())
	{
		qCDebug(KMIX_LOG) << "now have" << m_listeners.size() << "listeners";
	}
}


void ControlManager::removeListener(QObject *target, const QString &sourceId)
{
	removeListener(target, ControlManager::None, sourceId);
}


void ControlManager::removeListener(QObject *target, ControlManager::ChangeType changeType, const QString &sourceId)
{
	QMutableListIterator<Listener *> it(m_listeners);
	while (it.hasNext())
	{
		Listener *listener = it.next();
		if (listener->target!=target) continue;
		if (changeType!=ControlManager::None && listener->changeType!=changeType) continue;

		if (Settings::debugControlManager())
		{
			qCDebug(KMIX_LOG) << "Remove of" << listener->sourceId << "target" << target << "requested by" << sourceId;
		}

		it.remove();
		// TODO: this original comment does not make sense, the Listener was
		// allocated on the heap by us and a pointer to it is stored in the
		// list.  So surely it needs to be deleted?
		// "Hint: As we have actual objects stored, no explicit delete is needed"

		// See comment in addListener() above.
		m_listenersChanged = true;
	}
}


void ControlManager::warnUnexpectedChangeType(ControlManager::ChangeType changeType, QObject *obj)
{
	qCWarning(KMIX_LOG) << "Unexpected change type" << changeType << "received by" << obj;
}


void ControlManager::shutdownNow()
{
	if (Settings::debugControlManager()) qCDebug(KMIX_LOG) << "Shutting down";
	for (QList<Listener *>::const_iterator it = m_listeners.constBegin(); it!=m_listeners.constEnd(); ++it)
	{
		Listener *listener = (*it);
		if (Settings::debugControlManager())
		{
			// Despite the original log message which said
			// "Listener still connected. Closing it."
			// this actually did nothing apart from the
			// log message.
			qCDebug(KMIX_LOG) << "Listener still registered by" << listener->sourceId << "target" << listener->target;
		}
	}
}
