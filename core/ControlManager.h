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

class ControlChangeType: QObject
{
Q_OBJECT

public:
	enum Type
	{
		None = 0,  //
		TypeFirst = 1,
		Volume = 1,  // Volume or Switch change (Mute or Capture Switch, or Enum)
		ControlList = 2, // Control added or deleted
		GUI = 4, // Visual changes, like "split channel" OR "show labels"
		MasterChanged = 8 // Master (global or local) has changed
		,
		TypeLast = 16
	};

	static QString toString(Type changeType)
	{
		QString ret;
		bool needsSeparator = false;
		for (ControlChangeType::Type ct = ControlChangeType::TypeFirst; ct != ControlChangeType::TypeLast; ct =
			(ControlChangeType::Type) (ct << 1))
		{
			if (changeType & ct)
			{
				if (needsSeparator)
					ret.append('|');
				switch (ct)
				{
				case Volume:
					ret.append("Volume");
					break;
				case ControlList:
					ret.append("ControlList");
					break;
				case GUI:
					ret.append("GUI");
					break;
				case MasterChanged:
					ret.append("MasterChange");
					break;
				default:
					ret.append("Invalid");
					break;
				}

				needsSeparator = true;
			}
		}

		return ret;

	};
  
    static ControlChangeType::Type fromInt(int type)
  {
    switch ( type )
    {
      case 1: return Volume;
      case 2: return ControlList;
      case 4: return GUI;
      case 8: return MasterChanged;
      default: return None;
    }
  };
  
};

class Listener
{
public:
  Listener(const QString mixerId, ControlChangeType::Type changeType, QObject* target, QString& sourceId)
  {
    this->mixerId = mixerId;
    this->controlChangeType = changeType;
    // target is  bit dangerous, as it might get deleted.
    this->target = target;
    this->sourceId = sourceId;
  }
  
  const QString& getMixerId() { return mixerId; };
    ControlChangeType::Type& getChangeType() { return controlChangeType; };
    QObject* getTarget() { return target; };
    const QString& getSourceId() { return sourceId; };

private:
  QString mixerId;
  ControlChangeType::Type controlChangeType;
  QObject* target;
  QString sourceId;

  
};

class ControlManager
{
public:
  static ControlManager& instance();
  
  void announce(QString mixerId, ControlChangeType::Type changeType, QString sourceId);
  void addListener(QString mixerId, ControlChangeType::Type changeType, QObject* target, QString sourceId);
  void removeListener(QObject* target);
  void removeListener(QObject* target, QString sourceId);
  
  static void warnUnexpectedChangeType(ControlChangeType::Type type, QObject *obj);
  void shutdownNow();
  
private:
    ControlManager();
    static ControlManager instanceSingleton;
    QList<Listener> listeners;
    bool listenersChanged;
};

#endif // CONTROLMANAGER_H
