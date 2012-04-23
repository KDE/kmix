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
#include "mixsetadaptor.h"

DBusMixSetWrapper::DBusMixSetWrapper(QObject* parent, const QString& path)
	: QObject(parent)
	, m_dbusPath( path )
{
	new MixSetAdaptor( this );
	QDBusConnection::sessionBus().registerObject( m_dbusPath, this );

/*    connect( Mixer::getGlobalMasterPreferred(), SIGNAL(changed()),
        this, SLOT(slotPreferredMasterChanged()) );
    connect( Mixer::getGlobalMasterCurrent(), SIGNAL(changed()),
  		this, SLOT(slotCurrentMasterChanged()) );*/
}

DBusMixSetWrapper::~DBusMixSetWrapper()
{
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

void DBusMixSetWrapper::devicePlugged( const char* driverName, const QString& udi, QString& dev )
{
	Q_UNUSED( driverName )
	Q_UNUSED( udi )
	Q_UNUSED( dev )
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
			"org.kde.KMix.MixSet", "mixersChanged" );
	QDBusConnection::sessionBus().send( signal );
}

void DBusMixSetWrapper::deviceUnplugged( const QString& udi )
{
	Q_UNUSED( udi )
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
			"org.kde.KMix.MixSet", "mixersChanged" );
	QDBusConnection::sessionBus().send( signal );
}

/*
void DBusMixSetWrapper::slotPreferredMasterChanged()
{
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
			"org.kde.KMix.MixSet", "preferredMasterChanged" );
	QDBusConnection::sessionBus().send( signal );
}

void DBusMixSetWrapper::slotCurrentMasterChanged()
{
	QDBusMessage signal = QDBusMessage::createSignal( m_dbusPath, 
			"org.kde.KMix.MixSet", "currentMasterChanged" );
	QDBusConnection::sessionBus().send( signal );
}
*/
