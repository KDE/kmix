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

#include <QObject>
#include <QString>


// typedef int ControlChangeType;
//   enum ControlChangeType {
//   Volume,  // Volume or Switch change (Mute or Capture Switch, or Enum)
//   ControlList, // Control added or deleted
//   GUI // Visual changes, like "split channel" OR "show labels"    
//   }; 

class ControlChangeType
{
  public:
  enum Type {
  Volume,  // Volume or Switch change (Mute or Capture Switch, or Enum)
  ControlList, // Control added or deleted
  GUI, // Visual changes, like "split channel" OR "show labels"    
  MasterChanged // Master (global or local) has changed  
  };
  
  static QString toString(Type type)
  {
    switch ( type )
    {
      case Volume: return "Volume";
      case ControlList: return "ControlList";
      case GUI: return "GUI";
      case MasterChanged: return "MasterChange";
      default: return "Invalid";
    }
  };
};

class Listener
{
  QString mixerId;
  ControlChangeType::Type controlChangeType;
  QObject* target;
  QString sourceId;
  
  Listener(const QString mixerId, ControlChangeType::Type changeType, QObject* target, QString& sourceId)
  {
    this->mixerId = mixerId;
    this->controlChangeType = changeType;
    // target is  bit dangerous, as it might get deleted.
    this->target = target;
    this->sourceId = sourceId;
  }  
};

class ControlManager
{

public:
  static ControlManager& instance();
  
  void announce(QString mixerId, ControlChangeType::Type changeType, QString sourceId);
  void addListener(QString mixerId, ControlChangeType::Type changeType, QObject* target, QString sourceId);

private:
    static ControlManager instanceSingleton;
};

#endif // CONTROLMANAGER_H
