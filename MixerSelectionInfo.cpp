/*
 * KMix -- KDE's full featured mini mixer
 *
 * $Id$
 *
 * MixerSelectionInfo
 * Copyright (C) 2003 Christian Esken <esken@kde.org>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "MixerSelectionInfo.h"

/**
 * Stores information of a mixer selection done with the MixerSelector dialog.
 * num is the number from the list of mixers.
 * Hint: The deviceTypeMask[123] design is probably a bit hacky. It should be
 *  reworked later if we cannot live with it.
 */
MixerSelectionInfo::MixerSelectionInfo(int num, QString name, bool tabDistribution,
		MixDevice::DeviceCategory deviceTypeMask1,
		MixDevice::DeviceCategory deviceTypeMask2,
		MixDevice::DeviceCategory deviceTypeMask3)
{
	m_num = num;
 	m_name = name;
 	m_tabDistribution = tabDistribution;
 	m_deviceTypeMask1 = deviceTypeMask1;
 	m_deviceTypeMask2 = deviceTypeMask2;
 	m_deviceTypeMask3 = deviceTypeMask3;
 }
 
MixerSelectionInfo::~MixerSelectionInfo()
{
}

