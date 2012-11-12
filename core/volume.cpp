/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include "core/volume.h"

// for operator<<()
#include <iostream>

#include <kdebug.h>

int Volume::_channelMaskEnum[9] =
{ MLEFT, MRIGHT, MCENTER,
		MWOOFER,
		MSURROUNDLEFT, MSURROUNDRIGHT,
		MREARSIDELEFT, MREARSIDERIGHT,
		MREARCENTER
};

QString Volume::ChannelNameReadable[9] =
{
		"Left", "Right",
		"Center", "Subwoofer",
		"Surround Left", "Surround Right",
		"Side Left", "Side Right",
		"Rear Center"
};

char Volume::ChannelNameForPersistence[9][30] = {
		"volumeL", "volumeR",
		"volumeCenter", "volumeWoofer",
		"volumeSurroundL", "volumeSurroundR",
		"volumeSideL", "volumeSideR",
		"volumeRearCenter"
};

// Forbidden/private. Only here because if there is no CaptureVolume we need the values initialized
// And also QMap requires it.
Volume::Volume()
{
	_minVolume = 0;
	_maxVolume = 0;
	_hasSwitch = false;
	_switchActivated = false;
	_switchType = None;
	_isCapture = false;
	_chmask = MNONE;
	disallowSwitchDisallowRead = false;
}

// IIRC we need the default constructor implicitly for a Collection operation
VolumeChannel::VolumeChannel() {}

Volume::Volume(long maxVolume, long minVolume, bool hasSwitch, bool isCapture )
{
	init((ChannelMask)0, maxVolume, minVolume, hasSwitch, isCapture );
}

/**
 * @Deprecated
 */
void Volume::addVolumeChannels(ChannelMask chmask)
{
	for ( Volume::ChannelID chid=Volume::CHIDMIN; chid<= Volume::CHIDMAX;  )
	{
		if ( chmask & Volume::_channelMaskEnum[chid] )
		{
			addVolumeChannel(VolumeChannel(chid));
		}
		chid = (Volume::ChannelID)( 1 + (int)chid); // ugly
	} // for all channels
}

void Volume::addVolumeChannel(VolumeChannel vc)
{
	_volumesL.insert(vc.chid, vc);
	// Add the correpsonnding "muted version" of the chnnel.
//	VolumeChannel* zeroChannel = new VolumeChannel(vc.chid);
//	zeroChannel->volume = 0;
//	_volumesMuted.insert(zeroChannel->chid, *zeroChannel); // TODO remove _volumesMuted
}



void Volume::init( ChannelMask chmask, long maxVolume, long minVolume, bool hasSwitch, bool isCapture)
{
	_chmask          = chmask;
	_maxVolume       = maxVolume;
	_minVolume       = minVolume;
	_hasSwitch       = hasSwitch;
	_isCapture       = isCapture;
	//_muted           = false;
	// Presume that the switch is active. This will always work:
	// a) Physical switches will be updated after start from the hardware.
	// b) Emulated virtual/switches will not receive updates from the hardware, so they shouldn't disable the channels.
	_switchActivated = true;
	disallowSwitchDisallowRead = false;
}

QMap<Volume::ChannelID, VolumeChannel> Volume::getVolumesWhenActive() const
{
	return _volumesL;
}

QMap<Volume::ChannelID, VolumeChannel> Volume::getVolumes() const
{
	return _volumesL;
//	if ( isSwitchActivated() )
//		return _volumesL;
//	else
//	{
//		return _volumesMuted;
//	}
}

// @ compatibility
void Volume::setAllVolumes(long vol)
{
	long int finalVol = volrange(vol);
	QMap<Volume::ChannelID, VolumeChannel>::iterator it = _volumesL.begin();
	while (it != _volumesL.end())
	{
		it.value().volume = finalVol;
		//it.value().unmutedVolume= finalVol;
		++it;
	}
}

void Volume::changeAllVolumes( long step )
{
	QMap<Volume::ChannelID, VolumeChannel>::iterator it = _volumesL.begin();
	while (it != _volumesL.end())
	{
		long int finalVol = volrange(it.value().volume + step);
		it.value().volume = finalVol;
// 		it.value().unmutedVolume= finalVol;
		++it;
	}
}


/**
 * Sets the volume for the given Channel
 * @ compatibility
 */
void Volume::setVolume( ChannelID chid, long vol)
{
	QMap<Volume::ChannelID, VolumeChannel>::iterator it = _volumesL.find(chid);
	if ( it != _volumesL.end())
	{
		it.value().volume = vol;
		//it.value().unmutedVolume = vol;
	}
}

/**
 * Copy the volume elements contained in v to this Volume object.
 */
// void Volume::setVolume(const Volume &v)
// {
// 	foreach (VolumeChannel vc, _volumesL )
// 	{
// 		ChannelID chid = vc.chid;
// 		v.getVolumes()[chid].volume = vc.volume;
// 		//v.getVolumes()[chid].unmutedVolume = vc.volume;
// 	}
// }

   void Volume::setSwitch( bool active )
   {
     _switchActivated = active;

//      if ( isCapture() )
//        return;
//      
     // for playback volumes we will not only do the switch, but also set the volume to 0
// 	QMap<Volume::ChannelID, VolumeChannel>::iterator it = _volumesL.begin();
//      if ( active )
//      {
// 	while (it != _volumesL.end())
// 	{
// 	  VolumeChannel& vc = it.value();
// 	  vc.volume = vc.unmutedVolume;
// 	  ++it;
// 	}	
//      }
//      else
//      {
// 	while (it != _volumesL.end())
// 	{
// 	  VolumeChannel& vc = it.value();
// 	  vc.unmutedVolume = vc.volume;
// 	  vc.volume = 0;
// 	  ++it;
//        }       
//      }
  }

long Volume::maxVolume() {
	return _maxVolume;
}

long Volume::minVolume() {
	return _minVolume;
}

long Volume::volumeSpan() {
	return _maxVolume - _minVolume + 1;
}

/**
 * Returns the volume of the given channel.
 */
long Volume::getVolume(ChannelID chid)
{
	return _volumesL.value(chid).volume;
}

/**
 * Returns the volume of the given channel. If this Volume is inactive (switched off), 0 is returned.
 */
long Volume::getVolumeForGUI(ChannelID chid)
{
	if (! isSwitchActivated() )
		return 0;

	return _volumesL.value(chid).volume;
}

qreal Volume::getAvgVolume(ChannelMask chmask)
{
	int avgVolumeCounter = 0;
	long long sumOfActiveVolumes = 0;
	foreach (VolumeChannel vc, _volumesL )
	{
		if (Volume::_channelMaskEnum[vc.chid] & chmask )
		{
			sumOfActiveVolumes += vc.volume;
			++avgVolumeCounter;
		}
	}
	if (avgVolumeCounter != 0) {
		qreal sumOfActiveVolumesQreal = sumOfActiveVolumes;
		sumOfActiveVolumesQreal /= avgVolumeCounter;
		return sumOfActiveVolumesQreal;
	}
	else
		return 0;
}


int Volume::getAvgVolumePercent(ChannelMask chmask)
{
	qreal volume = getAvgVolume(chmask);
	// min=-100, max=200 => volSpan = 301
	// volume = -50 =>  volShiftedToZero = -50+min = 50
	qreal volSpan = volumeSpan();
	qreal volShiftedToZero = volume - _minVolume;
	qreal percentReal = ( volSpan == 0 ) ? 0 : ( 100 * volShiftedToZero ) / ( volSpan - 1);
	int percent = qRound(percentReal);
	//kDebug() << "volSpan=" << volSpan << ", volume=" << volume << ", volShiftedToPositive=" << volShiftedToZero << ", percent=" << percent;

	return percent;
}

int Volume::count() {
	return getVolumes().count();
}

/**
 * returns a "sane" volume level. This means, it is a volume level inside the
 * valid bounds
 */
long Volume::volrange( long vol )
{
	if ( vol < _minVolume ) {
		return _minVolume;
	}
	else if ( vol < _maxVolume ) {
		return vol;
	}
	else {
		return _maxVolume;
	}
}


std::ostream& operator<<(std::ostream& os, const Volume& vol) {
	os << "(";

	bool first = true;
	foreach ( const VolumeChannel vc, vol.getVolumes() )
	{
		if ( !first )  os << ",";
		else first = false;
		os << vc.volume;
	} // all channels
	os << ")";

	os << " [" << vol._minVolume << "-" << vol._maxVolume;
	if ( vol._switchActivated ) { os << " : switch active ]"; } else { os << " : switch inactive ]"; }

	return os;
}

QDebug operator<<(QDebug os, const Volume& vol) {
	os << "(";
	bool first = true;
	foreach ( VolumeChannel vc, vol.getVolumes() )
	{
		if ( !first )  os << ",";
		else first = false;
		os << vc.volume;
	} // all channels
	os << ")";

	os << " [" << vol._minVolume << "-" << vol._maxVolume;
	if ( vol._switchActivated ) { os << " : switch active ]"; } else { os << " : switch inactive ]"; }

	return os;
}
