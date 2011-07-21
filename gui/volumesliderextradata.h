//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#ifndef VOLUMESLIDEREXTRADATA_H
#define VOLUMESLIDEREXTRADATA_H

#include <QAbstractSlider>
#include "core/volume.h"

class VolumeSliderExtraData
{
public:
	VolumeSliderExtraData() : subcontrolLabel(0) {};
	~VolumeSliderExtraData() {};
	void setChid(Volume::ChannelID chid) { this->chid = chid; };
	Volume::ChannelID getChid() { return chid; };
	void setSubcontrolLabel(QWidget *subcontrolLabel) { this->subcontrolLabel = subcontrolLabel; };
	QWidget* getSubcontrolLabel() {return subcontrolLabel; }

protected:
	Volume::ChannelID chid;
	QWidget *subcontrolLabel;
};

class AbstractVolumeSlider : public QAbstractSlider, public VolumeSliderExtraData
{
};

#endif
