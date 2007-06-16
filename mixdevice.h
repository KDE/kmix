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

// !!! This SHOULD be subclassed (MixDeviceVolume, MixDeviceEnum).
//     The isEnum() works out OK as a workaround, but it is insane
//     in the long run.
//     Additionally there might be Implementations for virtual MixDevice's, e.g.
//     MixDeviceRecselector, MixDeviceCrossfader.
//     I am not sure if a MixDeviceBalancing would work out.

/**
 * This is the abstraction of a single control of a sound card, e.g. the PCM control. A control
 * can contain the 5 following subcontrols: playback-volume, capture-volume, playback-switch,
 * capture-switch and enumeration.

   Design hint: In the past I (esken) considered merging the MixDevice and Volume classes.
                I finally decided against it, as it seems better to have the MixDevice being the container
                for the embedded control(s). These could be either Volume, Enum or some virtual MixDevice.
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


    // Returns a user readable name of the control.
    QString   readableName()         { return _name; }
    // Sets a user readable name for the control.
    void      setReadableName(QString& name)      { _name = name; }

    /**
    * Returns an unique ID of this MixDevice. By default the number
    * 'num' from the constructor is returned. It is recommended that
    * a better ID is set directly after constructing the MixDevice using
    * the setUniqueID().
    */
    const QString& id() const;
    // operator==() is used currently only for duplicate detection with QList's contains() method
    bool operator==(const MixDevice& other) const;

    // @todo possibly remove the following 4 methods: isMuted(), ...
    bool isMuted()                  { return ( ! _playbackVolume.hasSwitch() || ! _playbackVolume.isSwitchActivated() ); }
    void setMuted(bool value)       { _playbackVolume.setSwitch( ! value ); }
    bool isRecSource()              { return ( _captureVolume.hasSwitch() && _captureVolume.isSwitchActivated() ); }
    void setRecSource(bool value)   { _captureVolume.setSwitch( value ); }

    bool isEnum()                   { return ( ! _enumValues.empty() ); }


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
