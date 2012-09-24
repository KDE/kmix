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

#include <KDebug>

ControlManager ControlManager::instanceSingleton;

ControlManager& ControlManager::instance()
{
  return instanceSingleton;
}


  void ControlManager::announce(QString mixerId, ControlChangeType::Type changeType, QString sourceId)
  {
    QList<Listener>::iterator it;
    for(it=listeners.begin(); it != listeners.end(); ++it)
    {
      Listener& listener = *it;
//       kDebug() << "Checking listener for " << listener.getMixerId() << " == " << mixerId;
      bool mixerIsOfInterest = listener.getMixerId().isEmpty() || mixerId.isEmpty() || listener.getMixerId() == mixerId;
      if (mixerIsOfInterest && listener.getChangeType() == changeType)
      {
	bool success = QMetaObject::invokeMethod(listener.getTarget(),
		                              "controlsChange",
		                              Qt::DirectConnection,
		                              Q_ARG(int, changeType));
	kDebug() << "Listener " << listener.getSourceId() << " is interested in " << mixerId
	<< ", " << ControlChangeType::toString(changeType);

	if (!success)
	{
	  kError() << "Listener Failed to send to " << listener.getTarget()->metaObject()->className();
	}
      }
    }
    
    kDebug() << "Announcing " << ControlChangeType::toString(changeType)
    << " for " << ( mixerId.isEmpty() ? "all cards" : mixerId)
    << " by " << sourceId;
  }
  
  /**
   * Adds a listener for the given mixerId and changeType.
   * Listeners are informed about all corresponding changes via a signal.
   * Listeners are not informed about changes that originates from oneself (according to sourceId).
   * @param mixerId The id of the Mixer you are interested in
   * @param changetType The changeType of interest
   * @param target The QObject, where the notification signal is sent to. It must implement the SLOT controlChanged(QString mixerId, ControlChangeType::Type changeType).
   * @param sourceId Only for logging
   */
  void ControlManager::addListener(QString mixerId, ControlChangeType::Type changeType, QObject* target, QString sourceId)
  {
        kDebug() << "Listening to " << ControlChangeType::toString(changeType)
    << " for " << ( mixerId.isEmpty() ? "all cards" : mixerId)
    << " by " << sourceId
    << ". Announcements are sent to " << target;
    
    Listener* listener = new Listener(mixerId, changeType, target, sourceId);
    listeners.append(*listener);
    kDebug() << "We now have" << listeners.size() << "listeners";
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
    
        QList<Listener>::iterator it;
    for(it=listeners.begin(); it != listeners.end(); ++it)
    {
      Listener& listener = *it;
      if ( listener.getTarget() == target )
      {
            kDebug() << "Stop Listening of " << listener.getSourceId() << " requested by " << sourceId  << " from " << target;
      }
    }
  }

  void ControlManager::shutdownNow()
  {
    kDebug() << "Shutting down ControlManager";
    QList<Listener>::iterator it;
    for(it=listeners.begin(); it != listeners.end(); ++it)
    {
      Listener& listener = *it;
      kDebug() << "Listener still connected. Closing it. source=" << listener.getSourceId() << "listener=" << listener.getTarget()->metaObject()->className();
    }
  }

  #include "ControlManager.moc"
  