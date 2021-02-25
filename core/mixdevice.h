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

#if defined(HAVE_STD_SHARED_PTR)
#include <memory>
using std::shared_ptr;
#elif defined(HAVE_STD_TR1_SHARED_PTR)
#include <tr1/memory>
using std::tr1::shared_ptr;
#endif

//KMix
#include "core/MediaController.h"
class Mixer;
class MixSet;
class ProfControl;
#include "core/volume.h"
#include "kmixcore_export.h"

class DBusControlWrapper;

// KDE
#include <kconfig.h>
#include <kconfiggroup.h>

// Qt
#include <QList>
#include <QObject>
#include <QString>

/**
 * This is the abstraction of a single control of a sound card, e.g. the PCM control. A control
 * can contain the 5 following subcontrols: playback-volume, capture-volume, playback-switch,
 * capture-switch and enumeration.

   The class is called MixDevice for historical reasons. Today it is just the Synonym for "Control".

   Design hint: In the past I (esken) considered merging the MixDevice and Volume classes.
                I finally decided against it, as it seems better to have the MixDevice being the container
                for the embedded subcontrol(s). These could be either Volume, Enum or some virtual MixDevice.
 */
class KMIXCORE_EXPORT MixDevice : public QObject
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
                       MICROPHONE_FRONT,

                       KMIX_COMPOSITE,
		       APPLICATION_STREAM
                     };

   enum SwitchType { OnOff, Mute, Capture, Activator };

   /**
    * Constructor for a MixDevice.
    * After having constructed a MixDevice, you <b>must</b> add it to the DBus
    * interfaces by calling addToPool(). You may then <b>not</b> delete this object.
    *
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
   MixDevice(Mixer* mixer, const QString& id, const QString& name, ChannelType type );
   MixDevice(Mixer* mixer, const QString& id, const QString& name, const QString& iconName = "", MixSet* moveDestinationMixSet = nullptr);
   virtual ~MixDevice();

   void close();

   shared_ptr<MixDevice> addToPool();

   const QString &iconName() const			{ return (_iconName); }
   void setIconName(const QString &newName);

   void addPlaybackVolume(Volume &playbackVol);
   void addCaptureVolume (Volume &captureVol);
   void addEnums(QList<QString*>& ref_enumList);
   
   // Media controls. New for KMix 4.0
   MediaController* mediaController() const		{ return (_mediaController); }
   // TODO move all media player controls to the MediaController class
   int mediaPlay();
   int mediaPrev();
   int mediaNext();

   // Returns a user readable name of the control.
   QString readableName() const				{ return (_name); }
   // Sets a user readable name for the control.
   void setReadableName(const QString &name)		{ _name = name; }

   QString configGroupName(QString prefix) const;

   /**
    * Returns an ID of this MixDevice, as passed in the constructor. The Creator (normally the backend) 
    * MUST ensure that all MixDevices's of one card have unique ID's.
    * The ID is used through the whole KMix application (including the config file) for identifying controls.
    */
   const QString &id() const				{ return (_id); }
   QString fullyQualifiedId() const;

   /**
    * Returns the DBus path for this MixDevice
    */
   const QString dbusPath() const;

   // Returns the associated mixer
   Mixer *mixer() const					{ return (_mixer); }

   // operator==() is used currently only for duplicate detection with QList's contains() method
   bool operator==(const MixDevice& other) const;

   // Methods for handling the switches. This methods are useful, because the Switch in the Volume object
   // is an abstract concept. It places no interpretation on the meaning of the switch (e.g. does "switch set" mean
   // "mute on", or does it mean "playback on", or "Capture active", or ...
   virtual bool isMuted() const;
   virtual void setMuted(bool value);
   virtual bool hasMuteSwitch() const;
   virtual void toggleMute();
   virtual bool isRecSource() const;
   virtual bool isNotRecSource() const;
   virtual void setRecSource(bool value);
   virtual bool isEnum() const;
   /**
    * Returns whether this MixDevice is virtually muted.
    * Only MixDevice objects w/o a physical switch can be muted virtually.
    */
   virtual bool isVirtuallyMuted() const;
   /**
    * Returns whether this is an application stream.
    */
   virtual bool isApplicationStream() const		{ return (_applicationStream); }
   /**
    * Mark this MixDevice as application stream
    */
   void setApplicationStream(bool applicationStream)	{ _applicationStream = applicationStream; }
   
   bool isMovable() const				{ return (_moveDestinationMixSet!=nullptr); }
   MixSet *moveDestinationMixSet() const		{ return (_moveDestinationMixSet); }

   bool isArtificial() const				{ return (_artificial); }
   void setArtificial(bool artificial)			{ _artificial = artificial; }

   void setControlProfile(ProfControl *control)		{ _profControl = control; }
   ProfControl* controlProfile() const 			{ return (_profControl); }
   
   virtual Volume &playbackVolume()			{ return (_playbackVolume); }
   virtual Volume &captureVolume()			{ return (_captureVolume); }

   void setEnumId(int);
   unsigned int enumId() const				{ return (_enumCurrentId); }
   const QStringList &enumValues() const		{ return (_enumValues); }

   bool hasPhysicalMuteSwitch() const;
   
   bool read(const KConfig *config, const QString &grp);
   bool write(KConfig *config, const QString &grp);
   int userVolumeLevel() const;

   void increaseOrDecreaseVolume(bool decrease, Volume::VolumeTypeFlag volumeType);

protected:
   void init( Mixer* mixer, const QString& id, const QString& name, const QString& iconName, MixSet* moveDestinationMixSet );

signals:
   void iconNameChanged(const QString &newName);

private:
   QString getVolString(Volume::ChannelID chid, bool capture);

   Mixer *_mixer;
   Volume _playbackVolume;
   Volume _captureVolume;
   int _enumCurrentId;
   QStringList _enumValues; // A MixDevice, that is an ENUM, has these _enumValues

   DBusControlWrapper *_dbusControlWrapper;
   MediaController* _mediaController;

   // A virtual control. It will not be saved/restored and/or doesn't get shortcuts
   // Actually we discriminate those "virtual" controls in artificial controls and dynamic controls:
   // Type        Shortcut  Restore
   // Artificial:    yes       no    Virtual::GlobalMaster or Virtual::CaptureGroup_3   (controls that are constructed artificially from other controls)
   // Dynamic   :     no       no    Controls that come and go, like Pulse Stream controls
   bool _artificial;
   MixSet *_moveDestinationMixSet;
   QString _iconName;
   bool _applicationStream;

   QString _name;   // Channel name
   QString _id;     // Primary key, used as part in config file keys
   ProfControl *_profControl;

   void readPlaybackOrCapture(const KConfigGroup& config, bool capture);
   void writePlaybackOrCapture(KConfigGroup& config, bool capture);
};

#endif
