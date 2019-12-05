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

#include <qstring.h>
#include <qflags.h>
#include <qlist.h>

class QObject;
class Listener;

#include "kmixcore_export.h"


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

  void announce(const QString &mixerId, ControlManager::ChangeType changeType, const QString &sourceId);
  void addListener(const QString &mixerId, ControlManager::ChangeTypes changeTypes, QObject *target, const QString &sourceId);
  void removeListener(QObject *target, const QString &sourceId = QString());

  static void warnUnexpectedChangeType(ControlManager::ChangeType type, QObject *obj);
  void shutdownNow();
  
private:
    ControlManager();
    static ControlManager instanceSingleton;
    QList<Listener *> listeners;
    bool listenersChanged;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ControlManager::ChangeTypes)



class Listener
{
public:
  Listener(const QString &mixerId, ControlManager::ChangeType changeType, QObject *target, const QString &sourceId)
  {
    this->mixerId = mixerId;
    this->controlChangeType = changeType;
    // target is  bit dangerous, as it might get deleted.
    this->target = target;
    this->sourceId = sourceId;
  }
  
  const QString getMixerId() const { return mixerId; }
    ControlManager::ChangeType getChangeType() const { return controlChangeType; }
    QObject *getTarget() const { return target; }
    const QString getSourceId() const { return sourceId; }

private:
  QString mixerId;
  ControlManager::ChangeType controlChangeType;
  QObject *target;
  QString sourceId;
};


#endif // CONTROLMANAGER_H
