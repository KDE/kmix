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

#ifndef DBUS_CONTROL_WRAPPER_H
#define DBUS_CONTROL_WRAPPER_H

#include <QObject>
#include "core/mixdevice.h"

class DBusControlWrapper : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString id READ id)
	Q_PROPERTY(QString readableName READ readableName)
	Q_PROPERTY(QString iconName READ iconName)
	Q_PROPERTY(int volume READ volume WRITE setVolume)
	Q_PROPERTY(long absoluteVolume READ absoluteVolume WRITE setAbsoluteVolume)
	Q_PROPERTY(long absoluteVolumeMin READ absoluteVolumeMin)
	Q_PROPERTY(long absoluteVolumeMax READ absoluteVolumeMax)
	Q_PROPERTY(bool mute READ isMuted WRITE setMute)
	Q_PROPERTY(bool recordSource READ isRecordSource WRITE setRecordSource)
	Q_PROPERTY(bool canMute READ canMute)
	Q_PROPERTY(bool hasCaptureSwitch READ hasCaptureSwitch)

	public:
		DBusControlWrapper(shared_ptr<MixDevice> parent, const QString& path);
		~DBusControlWrapper();

		void increaseVolume();
		void decreaseVolume();
		void toggleMute();
	private:
		shared_ptr<MixDevice> m_md;
		
		QString id();
		QString readableName();
		QString iconName();

		void setVolume(int percentage);
		int volume();

		void setAbsoluteVolume(long absoluteVolume);
		long absoluteVolumeMin();
		long absoluteVolumeMax();
		long absoluteVolume();

		bool canMute();
		void setMute(bool muted);
		bool isMuted();

		bool hasCaptureSwitch();
		void setRecordSource(bool on);
		bool isRecordSource();
};

#endif /* DBUS_MIXER_WRAPPER_H */
