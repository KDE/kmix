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

// for operator<<()
#include <iostream>

#include <kdebug.h>

#include "core/volume.h"


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
    
    // Forbidden/private. Only here because QMap requires it. 
Volume::Volume() {}
VolumeChannel::VolumeChannel() {}

// @Deprecated  use method without chmask
// Volume::Volume( ChannelMask chmask, long maxVolume, long minVolume, bool hasSwitch, bool isCapture  )
// {
//     init(chmask, maxVolume, minVolume, hasSwitch, isCapture );
// }

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
    
    void Volume::addVolumeChannel(VolumeChannel ch)
    {
      _volumesL.insert(ch.chid, ch);
    }

// copy constructor
Volume::Volume( const Volume &v )
{
    _chmask          = v._chmask;
    _maxVolume       = v._maxVolume;
    _minVolume       = v._minVolume;
    _hasSwitch       = v._hasSwitch;
    _switchActivated = v._switchActivated;
    _isCapture       = v._isCapture;
    setVolume(v, (ChannelMask)v._chmask);
    //    kDebug(67100) << "Volume::copy-constructor initialized " << v << "\n";
}

void Volume::init( ChannelMask chmask, long maxVolume, long minVolume, bool hasSwitch, bool isCapture )
{
    _chmask          = chmask;
    _maxVolume       = maxVolume;
    _minVolume       = minVolume;
    _hasSwitch       = hasSwitch;
    _isCapture       = isCapture;
    //_muted           = false;
    _switchActivated = false;
}

    QMap<Volume::ChannelID, VolumeChannel> Volume::getVolumes() const
    {
      return _volumesL;
    }

// @ compatibility
void Volume::setAllVolumes(long vol)
{
  long int finalVol = volrange(vol);
  foreach (VolumeChannel vc, _volumesL )
  {
    vc.volume = finalVol;
  }
}

void Volume::changeAllVolumes( long step )
{
  QMap<Volume::ChannelID, VolumeChannel>::iterator it = _volumesL.begin();
  while (it != _volumesL.end())
  {
    it.value().volume = volrange(it.value().volume + step);
    ++it;
    //VolumeChannel &vc = it.value();
  }
/*  foreach (const VolumeChannel& vc, _volumesL )
  {
    kDebug(67100) << &vc << " : " ; 
    vc.volume = volrange(vc.volume + step);
    kDebug(67100) << &vc << " : " ; 
  }*/
}


// @ compatibility
void Volume::setVolume( ChannelID chid, long vol)
{
  _volumesL[chid].volume = vol;
}

/**
 * Copy the volume elements contained in v to this Volume object.
 * Only those elments are copied, that are supported in BOTH Volume objects.
 */
void Volume::setVolume(const Volume &v)
{
     setVolume(v, (ChannelMask)(v._chmask&_chmask) );
}

/**
 * Copy the volume elements contained in v to this Volume object.
 * Only those elments are copied, that are supported in BOTH Volume objects
 * and match the ChannelMask given by chmask.
 */
void Volume::setVolume(const Volume &v, ChannelMask chmask)
{
  foreach (VolumeChannel vc, _volumesL )
  {
    ChannelID chid = vc.chid;
    if ( v.getVolumes().contains(chid) && (Volume::_channelMaskEnum[chid] & chmask) )
    {
      v.getVolumes()[chid].volume = vc.volume;
    }
  }
}

long Volume::maxVolume() {
  return _maxVolume;
}

long Volume::minVolume() {
  return _minVolume;
}

int Volume::percentage(long absoluteVolume)
{
   int relativeVolume = 0;
   if ( _maxVolume == 0 )
      return 0;

   if ( absoluteVolume > _maxVolume )
      relativeVolume = 100;
   else if ( absoluteVolume < _minVolume )
      relativeVolume = -100;
   else if ( absoluteVolume > 0 )
      relativeVolume = ( 100*absoluteVolume) / _maxVolume;
   else if ( absoluteVolume < 0 )
      relativeVolume = ( 100*absoluteVolume) / _minVolume;

   return relativeVolume;
}

long Volume::getVolume(ChannelID chid) {
  return _volumesL.value(chid).volume;
}

long Volume::getAvgVolume(ChannelMask chmask)
{
  int avgVolumeCounter = 0;
  long long sumOfActiveVolumes = _volumesL.size();
  foreach (VolumeChannel vc, _volumesL )
  {
    if (Volume::_channelMaskEnum[vc.chid] & chmask )
    {
      sumOfActiveVolumes += vc.volume;
      ++avgVolumeCounter;
    }
  }
    if (avgVolumeCounter != 0) {
        sumOfActiveVolumes /= avgVolumeCounter;
    }
    // else: just return 0;
    return (long)sumOfActiveVolumes;
}


int Volume::count() {
  return getVolumes().count();
}

/**
  * returns a "sane" volume level. This means, it is a volume level inside the
  * valid bounds
  */
long Volume::volrange( int vol )
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
