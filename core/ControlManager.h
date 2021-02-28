/*
    <one line to give the program's name and a brief idea of what it does.>
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

#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <qflags.h>
#include <qlist.h>

#include "kmixcore_export.h"

class QObject;
class Listener;


class KMIXCORE_EXPORT ControlManager
{
public:
  static ControlManager &instance();

  enum ChangeType
  {
    None = 0,						// no change
    First = 1,
    Volume = 1,						// Volume or Switch change (Mute or Capture Switch, or Enum)
    ControlList = 2,					// Control added or deleted
    GUI = 4,						// Visual changes, like "split channel" OR "show labels"
    MasterChanged = 8,					// Master (global or local) has changed
    Last = 16
  };
  Q_DECLARE_FLAGS(ChangeTypes, ChangeType)

  /**
   * Announce a change for one or all mixers.
   *
   * The change is announced to all listeners interested in the specified change type
   * for the specified mixer, apart from the originating listener (identified by the
   * source ID).  The listeners are informed of the change via their @c controlChanged()
   * slot.
   *
   * @param mixerId The mixer ID, or an empty string to announce a change for all mixers
   * @param changeType The change type to be announced
   * @param sourceId The sender of this announcement, only used for logging
   **/
  void announce(const QString &mixerId, ControlManager::ChangeType changeType, const QString &sourceId);

  /**
   * Register a listener interested in a given mixer and change type.
   *
   * @param mixerId The ID of the mixer of interest, or an empty string if interested in all mixers
   * @param changeTypes The change types of interest
   * @param target The object to be notified.
   * @param sourceId An identifier for this addition, only used for logging
   *
   * @note If multiple change types are specified in @c changeTypes, the corresponding
   * number of listeners are registered, one for each change type.  This affects logging
   * messages and removing the listener where an explicit change type is specified.
   **/
  void addListener(const QString &mixerId, ControlManager::ChangeTypes changeTypes, QObject *target, const QString &sourceId);

  /**
   * Remove all listeners interested in the given target.
   *
   * @param target The object that was registered to be notified
   * @param sourceId An identifier for this removal, only used for logging
   */
  void removeListener(QObject *target, const QString &sourceId = QString());

  /**
   * Remove all listeners interested in the given target and change type.
   *
   * @param target The object that was registered to be notified
   * @param changeType The change type that was of interest, or @c None for any change type
   * @param sourceId An identifier for this removal, only used for logging
   */
  void removeListener(QObject *target, ControlManager::ChangeType changeType, const QString &sourceId = QString());

  /**
   * Warn that an an unexpected change type announcement has been received.
   *
   * @param changeType The change type that has been received
   * @param obj The object that received the notification
   **/
  static void warnUnexpectedChangeType(ControlManager::ChangeType changeType, QObject *obj);

  /**
   * Shut down and remove all listeners.
   *
   * Intended to be called when the application is about to quit.
   **/
  void shutdownNow();
  
private:
    ControlManager();

    QList<Listener *> m_listeners;
    bool m_listenersChanged;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ControlManager::ChangeTypes)

#endif // CONTROLMANAGER_H
