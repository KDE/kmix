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

#include <QDebug>

DBusControlWrapper::DBusControlWrapper(MixDevice* parent, QString path)
	: QObject(parent)
	, m_dbusPath(path)
{
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
    // @todo Is hardcoded to PlaybackVolume
    // @todo This will not work, if minVolume != 0      !!!
	//       e.g.: minVolume=5 or minVolume=-10
	// The solution is to check two cases:
	//     volume < 0 => use minVolume for volumeRange
	//     volume > 0 => use maxVolume for volumeRange
	//     If chosen volumeRange==0 => return 0
	// As this is potentially used often (Sliders, ...), it
	// should be implemented in the Volume class.
	// For now we go with "maxVolume()", like in the rest of KMix.
	//  - esken
	Volume& volP = m_md->playbackVolume();
	Volume& volC = m_md->captureVolume();
	volP.setAllVolumes( (percentage * volP.maxVolume()) / 100 );
	volC.setAllVolumes( (percentage * volP.maxVolume()) / 100 );
	m_md->mixer()->commitVolumeChange( m_md );
}

int DBusControlWrapper::volume()
{
	Volume& vol = m_md->playbackVolume();
	return vol.maxVolume()
		? vol.getAvgVolume( (Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT) ) * 100 / vol.maxVolume()
		: 0;
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
	// @todo Is hardcoded do playbackVolume
	return m_md->playbackVolume().minVolume();
}

long DBusControlWrapper::absoluteVolumeMax()
{
	// @todo Is hardcoded do playbackVolume
	return m_md->playbackVolume().maxVolume();
}

void DBusControlWrapper::setAbsoluteVolume(long absoluteVolume)
{
	Volume& volP = m_md->playbackVolume();
	Volume& volC = m_md->captureVolume();
	volP.setAllVolumes( absoluteVolume );
	volC.setAllVolumes( absoluteVolume );
	m_md->mixer()->commitVolumeChange( m_md );
}

long DBusControlWrapper::absoluteVolume()
{
	// @todo hardcoded
	Volume& vol = m_md->playbackVolume();
	return ( vol.getAvgVolume( (Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT) ) );
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
    return m_md->playbackVolume().hasSwitch();
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
	m_md->mixer()->setRecordSource( m_md->id(), on );
}
