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

#include <QMutableListIterator>

#include <KDebug>

ControlManager ControlManager::instanceSingleton;

ControlManager& ControlManager::instance()
{
	return instanceSingleton;
}

ControlManager::ControlManager()
{
	listenersChanged = false;
}

/**
 * Announce a change for one or all mixers.
 *
 * @param mixerId The mixerId. Use an empty QString() to announce a change for all mixers
 * @param changeType A bit array of ControlChangeType flags
 * @param sourceId Only for logging
 *
 */
void ControlManager::announce(QString mixerId, ControlChangeType::Type changeType, QString sourceId)
{

	bool listenersModified = false;
	QSet<Listener*> processedListeners;
	do
	{
		listenersModified = false;
		QList<Listener>::iterator it;
		for (it = listeners.begin(); it != listeners.end(); ++it)
		{
			Listener& listener = *it;
			if ( &listener == 0 )
			{
				kWarning() << "null Listener detected ... skipping";
				continue;
			}

			bool mixerIsOfInterest = listener.getMixerId().isEmpty() || mixerId.isEmpty()
				|| listener.getMixerId() == mixerId;

			bool listenerAlreadyProcesed = processedListeners.contains(&listener);
			if ( listenerAlreadyProcesed )
			{
				kDebug() << "Skipping already processed listener"; // TODO remove the kDebug()
				continue;
			}
			if (mixerIsOfInterest && listener.getChangeType() == changeType)
			{
				bool success = QMetaObject::invokeMethod(listener.getTarget(), "controlsChange", Qt::DirectConnection,
				Q_ARG(int, changeType));
				kDebug() << "Listener " << listener.getSourceId() <<" is interested in " << mixerId
				<< ", " << ControlChangeType::toString(changeType);

				if (!success)
				{
					kError() << "Listener Failed to send to " << listener.getTarget()->metaObject()->className();
				}
				processedListeners.insert(&listener);
				if (listenersChanged)
				{
					// The invokeMethod() above has changed the listeners => my Iterator is invalid => restart loop
					kDebug() << "Listeners modified => restart loop";
					listenersChanged = false;
					listenersModified = true;
					break; // break inner loop => restart via outer loop
				}

			}
		}
	}
	while ( listenersModified);

	kDebug()
	<< "Announcing " << ControlChangeType::toString(changeType) << " for "
		<< (mixerId.isEmpty() ? "all cards" : mixerId) << " by " << sourceId;
}

/**
 * Adds a listener for the given mixerId and changeType.
 * Listeners are informed about all corresponding changes via a signal.
 * Listeners are not informed about changes that originate from oneself (according to sourceId).
 *
 * @param mixerId The id of the Mixer you are interested in
 * @param changetType The changeType of interest
 * @param target The QObject, where the notification signal is sent to. It must implement the SLOT controlChanged(QString mixerId, ControlChangeType::Type changeType).
 * @param sourceId Only for logging
 */
void ControlManager::addListener(QString mixerId, ControlChangeType::Type changeType, QObject* target, QString sourceId)
{
	kDebug()
	<< "Listening to " << ControlChangeType::toString(changeType) << " for "
		<< (mixerId.isEmpty() ? "all cards" : mixerId) << " by " << sourceId << ". Announcements are sent to "
		<< target;

	for ( ControlChangeType::Type ct = ControlChangeType::TypeFirst; ct != ControlChangeType::TypeLast;  ct = (ControlChangeType::Type)(ct << 1))
	{
		if ( changeType & ct )
		{
			// Add all listeners.
			Listener listener = Listener(mixerId, ct, target, sourceId);
			listeners.append(listener);
			listenersChanged = true;
		}
	}
	kDebug()
	<< "We now have" << listeners.size() << "listeners";
}

/**
 * Removes all listeners of the given target.
 * @param target The QObject that was used to register via addListener()
 */
void ControlManager::removeListener(QObject* target)
{
	ControlManager::instance().removeListener(target, target->metaObject()->className());
}

/**
 * Removes all listeners of the given target.
 * @param target The QObject that was used to register via addListener()
 * @param sourceId Optional: Only for logging
 */
void ControlManager::removeListener(QObject* target, QString sourceId)
{
	QMutableListIterator<Listener> it(listeners);
	while ( it.hasNext())
	{
		Listener& listener = it.next();
		if (listener.getTarget() == target)
		{
			kDebug()
			<< "Stop Listening of " << listener.getSourceId() << " requested by " << sourceId << " from " << target;
			it.remove();
			// Hint: As we have actual objects no explicit delete is needed
			listenersChanged = true;
		}
	}
}

void ControlManager::warnUnexpectedChangeType(ControlChangeType::Type type, QObject *obj)
{
	kWarning() << "Unexpected type " << type << " received by " << obj->metaObject()->className();
}

void ControlManager::shutdownNow()
{
	kDebug()
	<< "Shutting down ControlManager";
	QList<Listener>::iterator it;
	for (it = listeners.begin(); it != listeners.end(); ++it)
	{
		Listener& listener = *it;
		kDebug()
		<< "Listener still connected. Closing it. source=" << listener.getSourceId() << "listener="
			<< listener.getTarget()->metaObject()->className();
	}
}

#include "ControlManager.moc"
