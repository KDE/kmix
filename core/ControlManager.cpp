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
    kDebug() << "Announcing " << ControlChangeType::toString(changeType)
    << " for " << ( mixerId.isEmpty() ? "all cards" : mixerId)
    << " by " << sourceId;
  }
  
  /**
   * Adds a listener for the given mixerId and changeType.
   * Listeners are informed about all corresponding changes via a signal.
   * Listeners are not informed about changes that originates from oneself (according to sourceId).
   */
  void ControlManager::addListener(QString mixerId, ControlChangeType::Type changeType, QObject* target, QString sourceId)
  {
    
  }
