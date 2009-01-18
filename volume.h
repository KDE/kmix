//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef VOLUME_H
#define VOLUME_H

#include <fstream>

#include <kdebug.h>

class Volume
{

friend class MixDevice;

 public:
    // Channel definition:
    // For example a 2.0 system just has MLEFT and MRIGHT.
    // A 5.1 system adds MCENTER, MWOOFER, MSURROUNDLEFT and MSURROUNDRIGHT.
    // A 7.1 system furthermore adds MREARLEFT and MREARRIGHT.
    enum ChannelMask { MNONE     = 0,

                     MLEFT     =    1, MRIGHT     =    2, MCENTER =    4,
                     MMAIN     =    3, MFRONT     =    7,

                     MWOOFER   =    8,

                     // SURROUND (4.0 or 4.1 or in higher - like 5.1)
                     MSURROUNDLEFT = 0x10, MSURROUNDRIGHT = 0x20,
                     // MSURROUND
                     MSURROUND     = 0x30,

                     // REARSIDE (Usually only in 7.1)
                     MREARSIDELEFT = 0x40, MREARSIDERIGHT = 0x80,
                     // REARCENTER (Usually only in 6.1)
                     MREARCENTER   = 0x100,
                     // MREAR
                     MREAR         = 0x1C0,

                     MALL=0xFFFF };


    enum ChannelID { CHIDMIN      = 0,
                  LEFT         = 0, RIGHT         = 1, CENTER = 2,

                  WOOFER       = 3,

                  SURROUNDLEFT = 4, SURROUNDRIGHT = 5,

                  REARSIDELEFT = 6, REARSIDERIGHT = 7,

                  REARCENTER   = 8,

                  CHIDMAX      = 8 };



    // regular constructor
    Volume( ChannelMask chmask, long maxVolume, long minVolume, bool hasSwitch, bool isCapture );
    // compatibility constructor
    // Volume( int channels, long maxVolume );
    // copy constructor
    Volume( const Volume &v );
    // constructor for dummy volumes
    Volume();
    
    // Set all volumes as given by vol
    void setAllVolumes(long vol);
    // Set all volumes to the ones given in vol
    void setVolume(const Volume &vol );
    // Set volumes as specified by the channel mask
    void setVolume( const Volume &vol, ChannelMask chmask);
    void setVolume( ChannelID chid, long volume);

    // Increase or decrease all volumes by step
    void changeAllVolumes( long step );
    
    long getVolume(ChannelID chid);
    long getAvgVolume(ChannelMask chmask);
    long operator[](int);
    long maxVolume();
    long minVolume();
    int  percentage(long );
    int  count();
    
    bool hasSwitch()           { return _hasSwitch; }
    bool hasVolume()           { return (_maxVolume != _minVolume); }
    bool isCapture()           { return _isCapture; } // -<- Query thsi, to find out whether this is a capture or  a playback volume
    
   // Some playback switches control playback, and some are special.
   // ALSA doesn't differentiate between playback, OnOff and special, so users can add this information in the profile.
   // It is only used for GUI things, like showing a "Mute" text or tooltip
   // Capture is not really used, and has only been added for completeness and future extensibility.
   enum SwitchType { PlaybackSwitch, CaptureSwitch, OnSwitch, OffSwitch, SpecialSwitch };
    void setSwitchType(SwitchType type) { _switchType = type; }
   Volume::SwitchType switchType() { return _switchType; }

    friend std::ostream& operator<<(std::ostream& os, const Volume& vol);
    friend kdbgstream& operator<<(kdbgstream& os, const Volume& vol);

    // _channelMaskEnum[] and the following elements moved to public seection. operator<<() could not
    // access it, when private. Strange, as operator<<() is declared friend.
    static int    _channelMaskEnum[9];
    bool          _muted;
    long          _chmask;
    long          _volumes[CHIDMAX+1];
    long          _maxVolume;
    long          _minVolume;

protected:
   // setSwitch() and isSwitchActivated() are tricky. No regular class (incuding the Backends) shall use
   // these functions. Our friend class MixDevice will handle that gracefully for us.
   void setSwitch( bool val ) { _switchActivated = val; }
   bool isSwitchActivated()   { return _switchActivated && _hasSwitch; }


private:
    void init( ChannelMask chmask, long maxVolume, long minVolume, bool hasSwitch, bool isCapture);

    long volrange( int vol );
    long volrangeRec( int vol );

    bool _hasSwitch;
    bool _switchActivated;
    SwitchType _switchType;
    bool _isCapture;
};

std::ostream& operator<<(std::ostream& os, const Volume& vol);
kdbgstream& operator<<(kdbgstream &os, const Volume& vol);

#endif // VOLUME

