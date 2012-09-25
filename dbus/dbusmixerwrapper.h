/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 1996-2004 Christian Esken <esken@kde.org>
 * Copyright 2011 Igor Poboiko <igor.poboiko@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DBUSMIXERWRAPPER_H
#define DBUSMIXERWRAPPER_H

#include <QObject>
#include <QStringList>

#include "core/mixer.h"

class DBusMixerWrapper : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString driverName READ driverName)
	Q_PROPERTY(QString masterControl READ masterControl)
	Q_PROPERTY(QString readableName READ readableName)
	Q_PROPERTY(bool opened READ isOpened)
	Q_PROPERTY(QString id READ id)
	Q_PROPERTY(QString udi READ udi)
	Q_PROPERTY(int balance READ balance WRITE setBalance)
	Q_PROPERTY(QStringList controls READ controls)

	public:
		DBusMixerWrapper(Mixer* parent, const QString& path);
		~DBusMixerWrapper();
		QString driverName();

		QStringList controls();
		QString masterControl();

		bool isOpened();
		QString readableName();
		QString id();
		QString udi();

		int balance();
		void setBalance(int balance);
	public slots:
		void controlsChange(int changeType);
	private:
		void createDeviceWidgets();
		void refreshVolumeLevels();
		Mixer *m_mixer;
		QString m_dbusPath;
};

#endif /* DBUSMIXERWRAPPER_H */
