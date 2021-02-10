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
#ifndef RADOMPREFIX_VOLUME_H
#define RADOMPREFIX_VOLUME_H

#include <fstream>

#include <QDebug>
#include <QList>
#include <QMap>

#include "kmixcore_export.h"

class VolumeChannel;


class KMIXCORE_EXPORT Volume
{
    friend class MixDevice;

public:
    // Channel definition:
    // For example a 2.0 system just has MLEFT and MRIGHT.
    // A 5.1 system adds MCENTER, MWOOFER, MSURROUNDLEFT and MSURROUNDRIGHT.
    // A 7.1 system furthermore adds MREARLEFT and MREARRIGHT.
    enum ChannelMaskBits
    {
        MNONE          = 0x000,

        MLEFT          = 0x001,
        MRIGHT         = 0x002,
        MCENTER        = 0x004,
        MMAIN          = 0x003,
        MFRONT         = 0x007,

        MWOOFER        = 0x008,

        // SURROUND (4.0 or 4.1 or in higher - like 5.1)
        MSURROUNDLEFT  = 0x010,
        MSURROUNDRIGHT = 0x020,
        // all SURROUND
        MSURROUND      = 0x030,

        // REARSIDE (Usually only in 7.1)
        MREARSIDELEFT  = 0x040,
        MREARSIDERIGHT = 0x080,
        // REARCENTER (Usually only in 6.1)
        MREARCENTER    = 0x100,
        // all REAR
        MREAR          = 0x1C0,

        MALL           = 0xFFF
    };
    Q_DECLARE_FLAGS(ChannelMask, ChannelMaskBits)

    enum ChannelID
    {
        NOCHANNEL     = -1,
        CHIDMIN       = 0,

        LEFT          = 0,
        RIGHT         = 1,
        CENTER        = 2,

        WOOFER        = 3,

        SURROUNDLEFT  = 4,
        SURROUNDRIGHT = 5,

        REARSIDELEFT  = 6,
        REARSIDERIGHT = 7,

        REARCENTER    = 8,

        CHIDMAX       = 8
    };

    static QString channelNameForPersistence(Volume::ChannelID id);
    static QString channelNameReadable(Volume::ChannelID id);

    enum VolumeType { PlaybackVT = 0 , CaptureVT = 1 };

    enum VolumeTypeFlag { Playback = 1, Capture = 2, Both = 3 };

    // regular constructor (old, deprecated)
    //Volume( ChannelMask chmask, long maxVolume, long minVolume, bool hasSwitch, bool isCapture );
    // regular constructor
    Volume(long maxVolume, long minVolume, bool hasSwitch, bool isCapture );
    void addVolumeChannel(VolumeChannel ch);
    /// @Deprecated
    void addVolumeChannels(Volume::ChannelMask chmask);
    
    // Set all volumes as given by vol
    void setAllVolumes(long vol);
    // Set all volumes to the ones given in vol
    //void setVolume(const Volume &vol );
    // Set volumes as specified by the channel mask
    //void setVolume( const Volume &vol, Volume::ChannelMask chmask);
    void setVolume( Volume::ChannelID chid, long volume);

    // Increase or decrease all volumes by step
    void changeAllVolumes( long step );
    
    long getVolume(Volume::ChannelID chid) const;
    long getVolumeForGUI(Volume::ChannelID chid) const;
    qreal getAvgVolume(Volume::ChannelMask chmask) const;
    int getAvgVolumePercent(Volume::ChannelMask chmask) const;

    //long operator[](int);
    long maxVolume() const				{ return (_maxVolume); }
    long minVolume() const				{ return (_minVolume); }

    /**
     * The number of valid volume levels
     */
    long volumeSpan() const				{ return (_maxVolume-_minVolume+1); }

    int count() const					{ return (getVolumes().count()); }
    bool hasSwitch() const				{ return (_hasSwitch); }
    bool hasVolume() const				{ return (_maxVolume!=_minVolume); }

    /**
     * Returns whether this is a playback or capture volume.
     *
     * @return true, if it is a capture volume
     */
    bool isCapture() const				{ return (_isCapture); }
    
    // Some playback switches control playback, and some are special.
    // ALSA doesn't differentiate between playback, OnOff and special, so users can add this information in the profile.
    // It is only used for GUI things, like showing a "Mute" text or tooltip
    // Capture is not really used, and has only been added for completeness and future extensibility.
    enum SwitchType { None, PlaybackSwitch, CaptureSwitch, OnSwitch, OffSwitch, SpecialSwitch };
    void setSwitchType(SwitchType type)			{ _switchType = type; }
    Volume::SwitchType switchType() const		{ return (_switchType); }

    friend std::ostream& operator<<(std::ostream& os, const Volume& vol);
    friend QDebug operator<<(QDebug os, const Volume& vol);

    // _channelMaskEnum[] and the following elements moved to public section. operator<<() could not
    // access it, when private. Strange, as operator<<() is declared friend.
    const QMap<Volume::ChannelID, VolumeChannel> &getVolumes() const;
    long volumeStep(bool decrease) const;

    // Sets the value from the GUI configuration.  This affects all volume
    // increment or decrement operations.
    static void setVolumeStep(int percent);

protected:
    long          _chmask;
    QMap<Volume::ChannelID, VolumeChannel> _volumesL;

    long          _minVolume;
    long          _maxVolume;
   // setSwitch() and isSwitchActivated() are tricky. No regular class (including the Backends) shall use
   // these functions. Our friend class MixDevice will handle that gracefully for us.
   void setSwitch( bool active );
   bool isSwitchActivated() const  // TODO rename to isActive()
   {
	   return _switchActivated;
   };


private:
    // constructor for dummy volumes
    Volume();

    void init( Volume::ChannelMask chmask, long maxVolume, long minVolume, bool hasSwitch, bool isCapture);

    long volrange( long vol );

    bool _hasSwitch;
    bool _switchActivated;
    SwitchType _switchType;
    bool _isCapture;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Volume::ChannelMask)


class KMIXCORE_EXPORT VolumeChannel
{  
public:
    VolumeChannel();
    /**
     * Construct a channel for the given channel id.
     *
     * @param chid
     */
    VolumeChannel(Volume::ChannelID chid);

    long volume;
    Volume::ChannelID chid;
};

std::ostream& operator<<(std::ostream& os, const Volume& vol);
QDebug operator<<(QDebug os, const Volume& vol);

#endif // RADOMPREFIX_VOLUME

