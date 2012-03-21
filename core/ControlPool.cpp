/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (c) The KMix Authors
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

#include "ControlPool.h"

#include <QString>
#include <QMap>


shared_ptr<MixDevice> ControlPool::TheEmptyDevice; // = shared_ptr<MixDevice>(ControlPool::TheEmptyDevicePtr);

ControlPool::ControlPool()
{
	pool = new QMap<QString, shared_ptr<MixDevice> >();
}

ControlPool* ControlPool::_instance = 0;

ControlPool* ControlPool::instance()
{
	if ( _instance == 0 )
		ControlPool::_instance = new ControlPool();

	return ControlPool::_instance;
}

/**
 * Adds a Control to the pool, and returns it wrapped in QSharedPointer.
 * if the Control was already in the Pool, the existing Control is returned
 *
 * @param key A key, unique over all controls of all cards, e.g. "Master:0@ALSA::Creative_XFI:0"
 * @param mixDevice
 * @return
 */

shared_ptr<MixDevice> ControlPool::add(const QString& key, MixDevice* md)
{
	shared_ptr<MixDevice> controlFromPool(get(key));
	if ( controlFromPool.get() != 0)
	{
		kDebug() << "----ControlPool already cached key=" << key;
		return controlFromPool;
	}

	// else: Add the control to the pool
	kDebug() << "----ControlPool add key=" << key;
	shared_ptr<MixDevice> mdShared(md);
	pool->insert(key, mdShared);
	return mdShared;

}


/**
 * Retrieves a Control from the pool as QSharedPointer. If the Control is not
 * in the pool, a QSharedPointer that points to null (0) is returned.
 *
 * @param key
 * @return The Control wrapped in QSharedPointer. If not found, a QSharedPointer that points to null.
 */
shared_ptr<MixDevice> ControlPool::get(const QString& key)
{
	shared_ptr<MixDevice> mixDeviceShared = pool->value(key, TheEmptyDevice);
	return mixDeviceShared;
}
