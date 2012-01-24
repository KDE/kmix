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

#include "dbusmixerwrapper.h"

#include <QStringList>

#include "core/mixdevice.h"
#include "core/volume.h"
#include "mixeradaptor.h"

DBusMixerWrapper::DBusMixerWrapper(Mixer* parent, const QString& path)
	: QObject(parent)
	, m_dbusPath(path)
{
	m_mixer = parent;
	new MixerAdaptor( this );
	QDBusConnection::sessionBus().registerObject( path, this );
	connect( parent, SIGNAL(controlsReconfigured(QString)),
		this, SLOT(slotControlsReconfigured()) );
	connect( parent, SIGNAL(controlChanged()),
		this, SLOT(slotControlChanged()) );
}

DBusMixerWrapper::~DBusMixerWrapper()
{
}

QString DBusMixerWrapper::driverName()
{
	return m_mixer->getDriverName();
}

QStringList DBusMixerWrapper::controls()
{
	QStringList result;
	foreach ( MixDevice* md, m_mixer->getMixSet() )
	{
		result.append( md->dbusPath() );
	}
	return result;
}

QString DBusMixerWrapper::masterControl()
{
	MixDevice* md = m_mixer->getLocalMasterMD();
	// XXX: Since empty object path is invalid, using "/"
	return md ? md->dbusPath() : QString("/");
}

bool DBusMixerWrapper::isOpened()
{
	return m_mixer->isOpen();
}

int DBusMixerWrapper::balance()
{
	return m_mixer->balance();
}

void DBusMixerWrapper::setBalance(int balance)
{
	m_mixer->setBalance(balance);
}

QString DBusMixerWrapper::readableName()
{
	return m_mixer->readableName();
}

QString DBusMixerWrapper::id()
{
	return m_mixer->id();
}

QString DBusMixerWrapper::udi()
{
	return m_mixer->udi();
}

void DBusMixerWrapper::slotControlChanged()
{
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
				"org.kde.KMix.Mixer", "controlChanged" );
	QDBusConnection::sessionBus().send( signal );
}

void DBusMixerWrapper::slotControlsReconfigured()
{
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
				"org.kde.KMix.Mixer", "changed" );
	QDBusConnection::sessionBus().send( signal );
}
