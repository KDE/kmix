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
      if (listener.getMixerId() == mixerId )
      {
	kDebug() << "Listener might be interested in " << mixerId << ", " << changeType;
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
  }

  /**
   * Removes all listeners of the given target.
   * @param target The QObject that was used to register via addListener()
   * @param sourceId Optional: Only for logging
   */
  void ControlManager::removeListener(QObject* target, QString sourceId)
  {
            kDebug() << "Stop Listening by " << sourceId
    << " from " << target;
    
        QList<Listener>::iterator it;
    for(it=listeners.begin(); it != listeners.end(); ++it)
    {
      Listener& listener = *it;
     // if ( listener.
    }
  }

  