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

#include "dbuscontrolwrapper.h"
#include "controladaptor.h"
#include "core/mixer.h"
#include "core/volume.h"

DBusControlWrapper::DBusControlWrapper(shared_ptr<MixDevice> parent, const QString& path)
	: QObject(0)
{
	qDebug() << "QDBusConnection for control created" << path;
	m_md = parent;
	new ControlAdaptor( this );
	QDBusConnection::sessionBus().registerObject( path, this );
}

DBusControlWrapper::~DBusControlWrapper()
{
}

QString DBusControlWrapper::id()
{
	return m_md->id();
}

QString DBusControlWrapper::readableName()
{
	return m_md->readableName();
}

QString DBusControlWrapper::iconName()
{
	return m_md->iconName();
}

void DBusControlWrapper::setVolume(int percentage)
{
	Volume& volP = m_md->playbackVolume();
	Volume& volC = m_md->captureVolume();
	volP.setAllVolumes( volP.minVolume() + ((percentage * volP.volumeSpan()) / 100) );
	volC.setAllVolumes( volC.minVolume() + ((percentage * volC.volumeSpan()) / 100) );
	m_md->mixer()->commitVolumeChange( m_md );
}

int DBusControlWrapper::volume()
{
	Volume &useVolume = (m_md->playbackVolume().count() != 0) ? m_md->playbackVolume() : m_md->captureVolume();
	return useVolume.getAvgVolumePercent(Volume::MALL);
}

void DBusControlWrapper::increaseVolume()
{
	m_md->mixer()->increaseVolume(m_md->id());
}

void DBusControlWrapper::decreaseVolume()
{
	m_md->mixer()->decreaseVolume(m_md->id());
}

long DBusControlWrapper::absoluteVolumeMin()
{
	Volume &useVolume = (m_md->playbackVolume().count() != 0) ? m_md->playbackVolume() : m_md->captureVolume();
	return useVolume.minVolume();
}

long DBusControlWrapper::absoluteVolumeMax()
{
	Volume &useVolume = (m_md->playbackVolume().count() != 0) ? m_md->playbackVolume() : m_md->captureVolume();
	return useVolume.maxVolume();
}

void DBusControlWrapper::setAbsoluteVolume(long absoluteVolume)
{
	m_md->playbackVolume().setAllVolumes( absoluteVolume );
	m_md->captureVolume().setAllVolumes( absoluteVolume );
	m_md->mixer()->commitVolumeChange( m_md );
}

long DBusControlWrapper::absoluteVolume()
{
	Volume &useVolume = (m_md->playbackVolume().count() != 0) ? m_md->playbackVolume() : m_md->captureVolume();
	qreal avgVol= useVolume.getAvgVolume( Volume::MALL );
	long avgVolRounded = avgVol <0 ? avgVol-.5 : avgVol+.5;
	return avgVolRounded;
}

void DBusControlWrapper::setMute(bool muted)
{
	m_md->setMuted( muted );
	m_md->mixer()->commitVolumeChange( m_md );
}

void DBusControlWrapper::toggleMute()
{
	m_md->toggleMute();
	m_md->mixer()->commitVolumeChange( m_md );
}

bool DBusControlWrapper::canMute()
{
	return m_md->hasMuteSwitch();
}

bool DBusControlWrapper::isMuted()
{
	return m_md->isMuted();
}

bool DBusControlWrapper::isRecordSource()
{
	return m_md->isRecSource();
}

void DBusControlWrapper::setRecordSource(bool on)
{
	m_md->setRecSource(on);
	m_md->mixer()->commitVolumeChange( m_md );
}

bool DBusControlWrapper::hasCaptureSwitch()
{
	return m_md->captureVolume().hasSwitch();
}

#include "dbuscontrolwrapper.moc"
