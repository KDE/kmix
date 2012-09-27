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

#ifndef MIXERSERVICE_H
#define MIXERSERVICE_H

#include <Plasma/Service>
#include <Plasma/ServiceJob>
#include "control_interface.h"

//
class MixerService : public Plasma::Service
{
	Q_OBJECT
public:
	MixerService( QObject *parent, OrgKdeKMixControlInterface *iface );
	OrgKdeKMixControlInterface* iface();
protected:
	Plasma::ServiceJob* createJob(const QString& operation,
								QMap<QString,QVariant>& parameters);
	OrgKdeKMixControlInterface* m_iface;
};

//
class MixerJob : public Plasma::ServiceJob
{
	Q_OBJECT
public:
	MixerJob( MixerService *parent, const QString &operation,
			QMap<QString,QVariant>& parameters );
	void start();
private:
	MixerService *m_service;
};

#endif /* MIXERSERVICE_H */
