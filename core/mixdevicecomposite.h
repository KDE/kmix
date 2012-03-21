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
#ifndef MixDeviceComposite_h
#define MixDeviceComposite_h

//KMix
class Mixer;
class MixSet;
#include "core/mixdevice.h"
#include "core/volume.h"

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
class MixDeviceComposite : public MixDevice
{
Q_OBJECT

public:

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
   MixDeviceComposite( Mixer* mixer,  const QString& id, QList<shared_ptr<MixDevice> >& mds, const QString& name, ChannelType type );
//   MixDevice( Mixer* mixer, const QString& id, const QString& name, const QString& iconName = "", bool doNotRestore = false, MixSet* moveDestinationMixSet = 0 );
   ~MixDeviceComposite();



   // Methods for handling the switches. This methods are useful, because the Sswitch in the Volume object
   // is an abstract concept. It places no interpration on the meaning of the switch (e.g. does "switch set" mean
   // "mute on", or does it mean "playback on".
   virtual bool isMuted();
   virtual void setMuted(bool value);
   virtual bool isRecSource();
   virtual void setRecSource(bool value);
   virtual bool isEnum();

   // Refresh the composite from its components
   void update();

   virtual Volume& playbackVolume();
   //virtual Volume& captureVolume();

private:
   long calculateVolume(Volume::VolumeType vt);

   Mixer *_mixer;
   QList<shared_ptr<MixDevice> > _mds;

   static const long VolMax;

   Volume* _compositePlaybackVolume;
 //  Volume* _compositeCaptureVolume;
};

#endif
