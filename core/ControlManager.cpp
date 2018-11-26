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
#include "core/GlobalConfig.h"
#include "kmix_debug.h"

#include <QMutableListIterator>


ControlManager ControlManager::instanceSingleton;

ControlManager &ControlManager::instance()
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
void ControlManager::announce(const QString &mixerId, ControlManager::ChangeType changeType, const QString &sourceId)
{
	bool listenersModified = false;
	QSet<Listener *> processedListeners;
	do
	{
		listenersModified = false;
		for (QList<Listener *>::const_iterator it = listeners.constBegin(); it!=listeners.constEnd(); ++it)
		{
			Listener *listener = (*it);

			bool mixerIsOfInterest = listener->getMixerId().isEmpty() || mixerId.isEmpty()
				|| listener->getMixerId() == mixerId;

			bool listenerAlreadyProcesed = processedListeners.contains(listener);
			if ( listenerAlreadyProcesed )
			{
				if (GlobalConfig::instance().data.debugControlManager)
					qCDebug(KMIX_LOG) << "Skipping already processed listener";
				continue;
			}
			if (mixerIsOfInterest && listener->getChangeType() == changeType)
			{
				bool success = QMetaObject::invokeMethod(listener->getTarget(),
									 "controlsChange",
									 Qt::DirectConnection,
									 Q_ARG(ControlManager::ChangeType, changeType));
				if (GlobalConfig::instance().data.debugControlManager)
				{
					qCDebug(KMIX_LOG) << "Listener" << listener->getSourceId()
							  << "is interested in" << mixerId
							  << "type" << changeType;
				}

				if (!success)
				{
					qCCritical(KMIX_LOG) << "Listener failed to send to " << listener->getTarget()->metaObject()->className();
				}
				processedListeners.insert(listener);
				if (listenersChanged)
				{
					// The invokeMethod() above has changed the listeners => my Iterator is invalid => restart loop
					if (GlobalConfig::instance().data.debugControlManager)
						qCDebug(KMIX_LOG) << "Listeners modified => restart loop";
					listenersChanged = false;
					listenersModified = true;
					break; // break inner loop => restart via outer loop
				}

			}
		}
	}
	while (listenersModified);

	if (GlobalConfig::instance().data.debugControlManager)
	{
		qCDebug(KMIX_LOG)
		<< "Announcing" << changeType << "for"
		<< (mixerId.isEmpty() ? "all cards" : mixerId) << "by" << sourceId;
	}
}

/**
 * Adds a listener for the given mixerId and changeType.
 * Listeners are informed about all corresponding changes via a signal.
 * Listeners are not informed about changes that originate from oneself (according to sourceId).
 *
 * @param mixerId The id of the Mixer you are interested in
 * @param changetType The changeType of interest
 * @param target The QObject, where the notification signal is sent to. It must implement the SLOT controlChanged(QString mixerId,ControlManager::ChangeType changeType).
 * @param sourceId Only for logging
 */
void ControlManager::addListener(const QString &mixerId, ControlManager::ChangeTypes changeTypes,
				 QObject *target, const QString &sourceId)
{
	if (GlobalConfig::instance().data.debugControlManager)
	{
		qCDebug(KMIX_LOG)
		<< "Listening to" << changeTypes << "for"
		<< (mixerId.isEmpty() ? "all cards" : mixerId) << "by" << sourceId
		<< "sent to" << target;
	}

	for (ControlManager::ChangeType ct = ChangeType::First; ct!=ChangeType::Last;
	      ct = static_cast<ControlManager::ChangeType>(ct << 1))
	{
		if (changeTypes & ct)
		{
			// Add all listeners.
			Listener *listener = new Listener(mixerId, ct, target, sourceId);
			listeners.append(listener);
			listenersChanged = true;
		}
	}
	if (GlobalConfig::instance().data.debugControlManager)
	{
		qCDebug(KMIX_LOG) << "We now have" << listeners.size() << "listeners";
	}
}

/**
 * Removes all listeners of the given target.
 * @param target The QObject that was used to register via addListener()
 * @param sourceId Optional: Only for logging
 */
void ControlManager::removeListener(QObject *target, const QString &sourceId)
{
	QString src = sourceId;
	if (src.isEmpty()) src = target->metaObject()->className();

	QMutableListIterator<Listener *> it(listeners);
	while (it.hasNext())
	{
		Listener *listener = it.next();
		if (listener->getTarget() == target)
		{
			if (GlobalConfig::instance().data.debugControlManager)
				qCDebug(KMIX_LOG)
				<< "Stop Listening of" << listener->getSourceId() << "requested by" << src << "from" << target;
			it.remove();
			// Hint: As we have actual objects no explicit delete is needed
			listenersChanged = true;
		}
	}
}

void ControlManager::warnUnexpectedChangeType(ControlManager::ChangeType type, QObject *obj)
{
	qCWarning(KMIX_LOG) << "Unexpected type " << type << " received by " << obj->metaObject()->className();
}

void ControlManager::shutdownNow()
{
	if (GlobalConfig::instance().data.debugControlManager)
		qCDebug(KMIX_LOG) << "Shutting down ControlManager";
	for (QList<Listener *>::const_iterator it = listeners.constBegin(); it!=listeners.constEnd(); ++it)
	{
		Listener *listener = (*it);
		if (GlobalConfig::instance().data.debugControlManager)
			qCDebug(KMIX_LOG)
			<< "Listener still connected. Closing it. source" << listener->getSourceId()
			<< "target" << listener->getTarget()->metaObject()->className();
	}
}
