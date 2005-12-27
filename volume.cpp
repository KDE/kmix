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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// for operator<<()
#include <iostream>

#include <kdebug.h>

#include "volume.h"


int Volume::_channelMaskEnum[10] =
    { MLEFT, MRIGHT, MCENTER,
      MREARLEFT, MREARRIGHT, MWOOFER,
      MLEFTREC , MRIGHTREC ,
      MCUSTOM1, MCUSTOM2
    };

Volume::Volume( ChannelMask chmask, long maxVolume, long minVolume, bool isCapture  )
{
  init(chmask, maxVolume, minVolume, isCapture);
}


// @ compatiblity constructor
Volume::Volume( int channels, long maxVolume ) {
   if (channels == 1 ) {
       init(Volume::MLEFT, maxVolume, 0, false);
   }
   else if (channels == 2) {
      init(ChannelMask(Volume::MLEFT|Volume::MRIGHT), maxVolume, 0, false );
   }
   else {
     init(ChannelMask(Volume::MLEFT|Volume::MRIGHT), maxVolume, 0, false );
     kdError(67100) << "Warning: Multi-channel Volume object created with old constructor - this will not work fully\n";
   }
}

Volume::Volume( const Volume &v )
{
    _chmask     = v._chmask;
    _maxVolume  = v._maxVolume;
    _minVolume  = v._minVolume;
    _muted      = v._muted;
    _isCapture  = v._isCapture;
    setVolume(v, (ChannelMask)v._chmask);

    //    kdDebug(67100) << "Volume::copy-constructor initialized " << v << "\n";
}

void Volume::init( ChannelMask chmask, long maxVolume, long minVolume, bool isCapture )
{
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
	_volumes[i] = 0;
    }
    _chmask     = chmask;
    _maxVolume  = maxVolume;
    _minVolume  = minVolume;
    _isCapture = isCapture;
    _muted      = false;
}

// @ compatibility
void Volume::setAllVolumes(long vol)
{
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
        if (  (_channelMaskEnum[i]) & _chmask ) {
            // we are supposed to set it
            _volumes[i] = volrange(vol);
        }
    }
}

// @ compatibility
void Volume::setVolume( ChannelID chid, long vol)
{
    if ( chid>=0 && chid<=Volume::CHIDMAX ) {
        // accepted. we don't care if we support the channel,
        // because there is NO good action we could take.
        // Anyway: getVolume() on an unsupported channel will return 0 all the time
        _volumes[chid] = volrange(vol);
    }
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
void Volume::setVolume(const Volume &v, ChannelMask chmask) {
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
        if ( _channelMaskEnum[i] & _chmask & (int)chmask ) {
            // we are supposed to copy it
            _volumes[i] = volrange(v._volumes[i]);
        }
	else {
	    // Safety first! Lets play safe here and put sane values in
	    _volumes[i] = 0;
	}
    }

}

long Volume::maxVolume() {
  return _maxVolume;
}

long Volume::minVolume() {
  return _minVolume;
}

// @ compatibility
long Volume::operator[](int id) {
  return getVolume( (Volume::ChannelID) id );
}

long Volume::getVolume(ChannelID chid) {
  long vol = 0;

  if ( chid < 0 || chid > (Volume::CHIDMAX) ) {
    // should throw exception here. I will return 0 instead
  }
  else {
    // check if channel is supported
    int chmask = _channelMaskEnum[chid];
    if ( (chmask & _chmask) != 0 ) {
       // channel is supported
      vol = _volumes[chid];
    }
    else {
      // should throw exception here. I will return 0 instead
    }
  }

  return vol;
}

long Volume::getAvgVolume(ChannelMask chmask) {
    int avgVolumeCounter = 0;
    long long sumOfActiveVolumes = 0;
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
        if ( (_channelMaskEnum[i] & _chmask) & (int)chmask ) {
            avgVolumeCounter++;
            sumOfActiveVolumes += _volumes[i];
        }
    }
    if (avgVolumeCounter != 0) {
        sumOfActiveVolumes /= avgVolumeCounter;
    }
    else {
        // just return 0;
    }
    return (long)sumOfActiveVolumes;
}

long Volume::getTopStereoVolume(ChannelMask chmask) {
    long long topVolumeCount = 0;
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
        if ( (_channelMaskEnum[i] & _chmask) & (int)chmask ) {
			  if ( topVolumeCount < _volumes[i] )
				  topVolumeCount = _volumes[i];
        }
    }
    return (long)topVolumeCount;
}

int Volume::count() {
    int counter = 0;
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
        if ( _channelMaskEnum[i] & _chmask ) {
            counter++;
        }
    }
    return counter;
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
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
	if ( i != 0 ) {
	    os << ",";
	}
	if ( Volume::_channelMaskEnum[i] & vol._chmask ) {
	    // supported channel: Print Volume
	    os << vol._volumes[i];
	}
	else {
	    // unsupported channel: Print "x"
	    os << "x";
	}
    } // all channels
    os << ")";

    os << " [" << vol._minVolume << "-" << vol._maxVolume;
    if ( vol._muted ) { os << " : muted ]"; } else { os << " : playing ]"; }

    return os;
}

kdbgstream& operator<<(kdbgstream &os, const Volume& vol) {
    os << "(";
    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
	if ( i != 0 ) {
	    os << ",";
	}
	if ( Volume::_channelMaskEnum[i] & vol._chmask ) {
	    // supported channel: Print Volume
	    os << vol._volumes[i];
	}
	else {
	    // unsupported channel: Print "x"
	    os << "x";
	}
    } // all channels
    os << ")";

    os << " [" << vol._minVolume << "-" << vol._maxVolume;
    if ( vol._muted ) { os << " : muted ]"; } else { os << " : playing ]"; }

    return os;
}
