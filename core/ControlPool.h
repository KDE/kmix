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

#ifndef CONTROL_POOL_H
#define CONTROL_POOL_H

#include "config.h"

#if defined(HAVE_STD_SHARED_PTR)
#include <memory>
using std::shared_ptr;
#elif defined(HAVE_STD_TR1_SHARED_PTR)
#include <tr1/memory>
using std::tr1::shared_ptr;
#endif

#include "core/mixdevice.h"

class ControlPool
{

public:
	static ControlPool* instance();
	shared_ptr<MixDevice> add(const QString& key, MixDevice* mixDevice);
	shared_ptr<MixDevice> get(const QString& key);


private:
	ControlPool();
	virtual ~ControlPool() {};


	QMap<QString, shared_ptr<MixDevice> > *pool;
	static ControlPool* _instance;
	static shared_ptr<MixDevice> TheEmptyDevice;
};

#endif
