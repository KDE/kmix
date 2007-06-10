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
#ifndef MixDevice_h
#define MixDevice_h

#include "volume.h"
#include <QString>
#include <kconfig.h>
#include <QObject>
#include <qlist.h>

// ! @todo : CONSIDER MERGING OF MixDevice and Volume classes:
//           Not easy possible, because Volume is used in the driver backends
// !!!       But a lot of stuff MUST still be moved in the Volume class. Especially
//           the Switch management (reason: there might a capture and an playback switch, and
//           they just belong to their corresponding volumes.

// !!! This SHOULD be subclassed (MixDeviceVolume, MixDeviceEnum).
//     The DeviceCategory stuff worked out OK as a workaround, but it is actually insane
//     in the long run.
//     Additionally there might be Implementations for virtual MixDevice's, e.g.
//     MixDeviceRecselector, MixDeviceCrossfader.
//     I am not sure if a MixDeviceBalancing would work out.

/**
 * This is the abstraction of a single control of a sound card, e.g. the PCM control. A control
 * can contain the 5 following subcontrols: playback-volume, capture-volume, playback-switch,
 * capture-switch and enumeration.
 */
class MixDevice : public QObject
{
Q_OBJECT

public:
    // For each ChannelType a special icon exists
    enum ChannelType {AUDIO = 1, BASS, CD, EXTERNAL, MICROPHONE,
                        MIDI, RECMONITOR, TREBLE, UNKNOWN, VOLUME,
                        VIDEO, SURROUND, HEADPHONE, DIGITAL, AC97,
                        SURROUND_BACK, SURROUND_LFE, SURROUND_CENTERFRONT, SURROUND_CENTERBACK };


    /**
     * Constructor:
     * @par id  Defines the ID, e.g. used in looking up the keys in kmixrc.
     *          It is advised to set a nice name, like
     *          'PCM:2', which would mean "2nd PCM device of the sound card".
     *          The ID's may NOT contain whitespace
     */
    MixDevice(const QString& id, Volume &playbackVol, Volume &captureVol, const QString& name, ChannelType type = UNKNOWN );
    MixDevice(const MixDevice &md);
    ~MixDevice();


    /** 
     * Returns the name of the control.
     */
    QString   name()         { return _name; }
    /**
    * Returns an unique ID of this MixDevice. By default the number
    * 'num' from the constructor is returned. It is recommended that
    * a better ID is set directly after constructing the MixDevice using
    * the setUniqueID().
    */
    const QString& id() const;
    // operator==() is used currently only for duplicate detection with QList's contains() method
    bool operator==(const MixDevice& other) const;

    // @todo Should I remove the following 4 methods: isRecordable(), ...
    bool isMuteable()               { return _playbackVolume.hasSwitch(); }
    bool isMuted()                  { return ! _playbackVolume.isSwitchActivated(); }
    bool isRecordable()             { return _captureVolume.hasSwitch(); }
    bool isRecSource()              { return _captureVolume.isSwitchActivated(); }

    bool isEnum()                   { return ( ! _enumValues.empty() ); }

    void setMuted(bool value)       { _playbackVolume.setSwitch( value ); }
    void setRecSource(bool value)   { _captureVolume.setSwitch( value ); }

    Volume& playbackVolume();
    Volume& captureVolume();

    void setEnumId(int);
    unsigned int enumId();
    QList<QString>& enumValues();

    void read( KConfig *config, const QString& grp );
    void write( KConfig *config, const QString& grp );

    void setType( ChannelType channeltype ) { _type = channeltype; }
    ChannelType type() { return _type; }

signals:
    void newVolume( int num, Volume volume );

protected:
    Volume _playbackVolume;
    Volume _captureVolume;
    ChannelType _type;
/*
    // The 2 or 4 following items MUST be moved to _volumePlay / _volumeCapture
      bool _recordable; // Can it be recorded?  !!! move to _volumeCapture
      bool _mute; // Available mute option
    // This MIGHT remain in the class
      bool _recSource; // Current rec status
*/

    QString _name;   // Channel name
    QString _id;     // Primary key, used as part in config file keys
    // A MixDevice, that is an ENUM, has these _enumValues
    QList<QString> _enumValues;
    int _enumCurrentId;

private:
    void readPlaybackOrCapture(KConfig *config, const char* nameLeftVolume, const char* nameRightVolume, bool capture);
    void writePlaybackOrCapture(KConfig *config, const char* nameLeftVolume, const char* nameRightVolume, bool capture);

};

#endif
