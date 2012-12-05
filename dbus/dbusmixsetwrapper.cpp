/*
 * KMix -- KDE's full featured mini mixer
 *
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

#include "dbusmixsetwrapper.h"

#include "core/mixdevice.h"
#include "core/ControlManager.h"
#include "mixsetadaptor.h"

DBusMixSetWrapper* DBusMixSetWrapper::instanceSingleton;

void DBusMixSetWrapper::initialize(QObject* parent, const QString& path)
{
	/* This should not happen! */
	if (instanceSingleton)
		delete instanceSingleton;
	instanceSingleton = new DBusMixSetWrapper(parent, path);
}

DBusMixSetWrapper* DBusMixSetWrapper::instance() {
	return instanceSingleton;
}

DBusMixSetWrapper::DBusMixSetWrapper(QObject* parent, const QString& path)
	: QObject(parent)
	, m_dbusPath( path )
{
	new MixSetAdaptor( this );
	QDBusConnection::sessionBus().registerObject( m_dbusPath, this );
	
	ControlManager::instance().addListener(
		QString(),
		ControlChangeType::MasterChanged, 
		this,
		QString("DBusMixSetWrapper"));
}

DBusMixSetWrapper::~DBusMixSetWrapper()
{
}

void DBusMixSetWrapper::controlsChange(int changeType)
{
	ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
	switch (type)
	{
		case ControlChangeType::MasterChanged:
			signalMasterChanged();
			break;
		default:
			ControlManager::warnUnexpectedChangeType(type, this);
	}
}

QStringList DBusMixSetWrapper::mixers() const
{
	QStringList result;
	Q_FOREACH(Mixer* mixer, Mixer::mixers())
		result.append( mixer->dbusPath() );
	return result;
}

QString DBusMixSetWrapper::currentMasterMixer() const
{
	Mixer* masterMixer = Mixer::getGlobalMasterMixer();
	return masterMixer ? masterMixer->id() : QString();
}

QString DBusMixSetWrapper::currentMasterControl() const
{
	shared_ptr<MixDevice> masterControl = Mixer::getGlobalMasterMD();
	return masterControl ? masterControl->id() : QString();
}

QString DBusMixSetWrapper::preferredMasterMixer() const
{
	return Mixer::getGlobalMasterPreferred().getCard();
}

QString DBusMixSetWrapper::preferredMasterControl() const
{
	return Mixer::getGlobalMasterPreferred().getControl();
}

void DBusMixSetWrapper::setCurrentMaster(const QString &mixer, const QString &control)
{
	Mixer::setGlobalMaster(mixer, control, false);
}

void DBusMixSetWrapper::setPreferredMaster(const QString &mixer, const QString &control)
{
	Mixer::setGlobalMaster(mixer, control, true);
}

void DBusMixSetWrapper::signalMixersChanged()
{
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
			"org.kde.KMix.MixSet", "mixersChanged" );
	QDBusConnection::sessionBus().send( signal );
}

void DBusMixSetWrapper::signalMasterChanged()
{
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
			"org.kde.KMix.MixSet", "masterChanged" );
	QDBusConnection::sessionBus().send( signal );
}

#include "dbusmixsetwrapper.moc"
