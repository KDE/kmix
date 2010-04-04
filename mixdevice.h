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

//KMix
class Mixer;
#include "volume.h"

// KDE
#include <kconfig.h>
#include <kconfiggroup.h>

// Qt
#include <QList>
#include <QObject>
#include <QString>


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

   The class is called MixDevice for historical reasons. Today it is just the Synonym for "Control".

   Design hint: In the past I (esken) considered merging the MixDevice and Volume classes.
                I finally decided against it, as it seems better to have the MixDevice being the container
                for the embedded subcontrol(s). These could be either Volume, Enum or some virtual MixDevice.
 */
class MixDevice : public QObject
{
Q_OBJECT

public:
    // For each ChannelType a special icon exists
    enum ChannelType { AUDIO = 1,
                       BASS,
                       CD,
                       EXTERNAL,
                       MICROPHONE,
                       MIDI,
                       RECMONITOR,
                       TREBLE,
                       UNKNOWN,
                       VOLUME,
                       VIDEO,
                       SURROUND,
                       HEADPHONE,
                       DIGITAL,
                       AC97,
                       SURROUND_BACK,
                       SURROUND_LFE,
                       SURROUND_CENTERFRONT,
                       SURROUND_CENTERBACK,
                       SPEAKER,
                       MICROPHONE_BOOST,
                       MICROPHONE_FRONT_BOOST,
                       MICROPHONE_FRONT
                     };

   enum SwitchType { OnOff, Mute, Capture, Activator };

   /**
    * Constructor:
    * @par mixer The mixer this control belongs to
    * @par id  Defines the ID, e.g. used in looking up the keys in kmixrc. Also it is used heavily inside KMix as unique key. 
    *      It is advised to set a nice name, like 'PCM:2', which would  mean 
    *      "2nd PCM device of the sound card". The ID's may NOT contain whitespace.
    *       The Creator (normally the backend) MUST pass distinct ID's for each MixDevices of one card.
    *
    *      Virtual Controls (controls not created by a backend) are prefixed with "KMix::", e.g.
    *      "KMix::RecSelector:0"
    *  @par name is the readable name. This one is presented to the user in the GUI
    *  @par type The control type. It is only used to find an appropriate icon
    */
   MixDevice( Mixer* mixer, const QString& id, const QString& name, ChannelType type );
   MixDevice( Mixer* mixer, const QString& id, const QString& name, const QString& iconName = "", bool doNotRestore = false, bool movable = false );
   ~MixDevice();

   const QString& iconName() const { return _iconName; }

   void addPlaybackVolume(Volume &playbackVol);
   void addCaptureVolume (Volume &captureVol);
   void addEnums (QList<QString*>& ref_enumList);

   // Returns a user readable name of the control.
   QString   readableName()         { return _name; }
   // Sets a user readable name for the control.
   void      setReadableName(QString& name)      { _name = name; }

   /**
   * Returns an ID of this MixDevice, as passed in the constructor. The Creator (normally the backend) 
    * MUST ensure that all MixDevices's of one card have unique ID's.
   * The ID is used through the whole KMix application (including the config file) for identifying controls.
   */
   const QString& id() const;

   // Returns the associated mixer
   Mixer* mixer() { return _mixer; }

   // operator==() is used currently only for duplicate detection with QList's contains() method
   bool operator==(const MixDevice& other) const;

   // Methods for handling the switches. This methods are useful, because the Sswitch in the Volume object
   // is an abstract concept. It places no interpration on the meaning of the switch (e.g. does "switch set" mean
   // "mute on", or does it mean "playback on".
   bool isMuted()                  { return ( _playbackVolume.hasSwitch() && ! _playbackVolume.isSwitchActivated() ); }
   void setMuted(bool value)       { _playbackVolume.setSwitch( ! value ); }
   bool isRecSource()              { return ( _captureVolume.hasSwitch() && _captureVolume.isSwitchActivated() ); }
   void setRecSource(bool value)   { _captureVolume.setSwitch( value ); }

   bool isEnum()                   { return ( ! _enumValues.empty() ); }

   bool isMovable()                { return _movable; }

   Volume& playbackVolume();
   Volume& captureVolume();

   void setEnumId(int);
   unsigned int enumId();
   QList<QString>& enumValues();

   virtual void read( KConfig *config, const QString& grp );
   virtual void write( KConfig *config, const QString& grp );

private:
   Mixer *_mixer;
   Volume _playbackVolume;
   Volume _captureVolume;
   int _enumCurrentId;
   QList<QString> _enumValues; // A MixDevice, that is an ENUM, has these _enumValues

   bool _doNotRestore;
   bool _movable;
   QString _iconName;

   QString _name;   // Channel name
   QString _id;     // Primary key, used as part in config file keys

   void init( Mixer* mixer, const QString& id, const QString& name, const QString& iconName, bool doNotRestore, bool movable );
   void readPlaybackOrCapture(const KConfigGroup& config, bool capture);
   void writePlaybackOrCapture(KConfigGroup& config, bool capture);

};

#endif
