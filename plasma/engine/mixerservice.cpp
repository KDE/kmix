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

#include "mixerservice.h"

MixerService::MixerService(QObject *parent, OrgKdeKMixControlInterface *iface)
	: Plasma::Service( parent )
{
	m_iface = iface;

	setName("mixer");
	setDestination( "mixer" );
}

OrgKdeKMixControlInterface* MixerService::iface()
{
	return m_iface;
}

Plasma::ServiceJob* MixerService::createJob(const QString& operation,
											QMap<QString,QVariant>& parameters)
{
	return new MixerJob( this, operation, parameters );
}

MixerJob::MixerJob( MixerService* service, const QString &operation,
				QMap<QString,QVariant>& parameters )
	: Plasma::ServiceJob(service->destination(), operation, parameters, service)
	, m_service( service )
{
}

void MixerJob::start()
{
	QString operation = operationName();
	if ( operation == "setVolume" )	{
		bool res = m_service->iface()->setProperty( "volume", parameters().value("level").toInt() );
		setResult( res );
		return;
	} 
	else if ( operation == "setMute" ) {
		bool res = m_service->iface()->setProperty( "mute", parameters().value("muted").toBool() );
		setResult( res );
		return;
	}
	else if ( operation == "setRecordSource" ) {
		bool res = m_service->iface()->setProperty( "recordSource", parameters().value("recordSource").toBool() );
		setResult( res );
		return;
	}
}

#include "mixerservice.moc"
