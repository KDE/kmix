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

/* !! @todo : Add 2 fields:
 * bool update_from_Hardware;
 * bool update_from_UI;
 * They will show whether there are pending changes from both sides.
 * Updates will be faster and more reliable by this.
 */
class MixDevice : public QObject
{
     Q_OBJECT

   public:
      // For each ChannelType a special icon exists
      enum ChannelType {AUDIO = 1, BASS, CD, EXTERNAL, MICROPHONE,
                        MIDI, RECMONITOR, TREBLE, UNKNOWN, VOLUME,
                        VIDEO, SURROUND, HEADPHONE, DIGITAL, AC97,
			SURROUND_BACK, SURROUND_LFE, SURROUND_CENTERFRONT, SURROUND_CENTERBACK
     };


      // The DeviceCategory tells the type of the device
      // It is used in bitmasks, so you must use values of 2^n .
      enum DeviceCategory { UNDEFINED= 0x00, SLIDER=0x01, SWITCH=0x02, ENUM=0x04, ALL=0xff };


      MixDevice(int num, Volume &vol, bool recordable, bool mute,
                                QString name, ChannelType type = UNKNOWN, DeviceCategory category =
SLIDER );
      MixDevice(const MixDevice &md);
      ~MixDevice();

      int num()                    { return _num; };
      QString   name()         { return _name; };
      /**
       * Returns an unique ID of this MixDevice. By default the number
       * 'num' from the constructor is returned. It is recommended that
       * a better ID is set directly after constructing the MixDevice using
       * the setUniqueID().
       */
      QString& id();
      /**
       * Set a suitable ID for this MixDevice. It is used in looking up
       * the keys in kmixrc. It is advised to set a nice name, like
       * 'PCM:2', which would mean "2nd PCM device of the sound card".
       * The ID's may NOT contain whitespace
       * The ID's are managed by the MixerBackend's (only those know how to avoid name clashes).
       */
      void setId(QString &id);
      bool isRecordable()    { return _recordable; };
      bool isRecSource()    { return _recSource; };
      bool isSwitch()        { return _switch; } // !! change to _category == MixDevice::SWITCH
      bool isEnum()          { return _category == MixDevice::ENUM; }
      bool isMuted()         { return _volume.isMuted(); };
      bool hasMute()         { return _mute; }

      void setMuted(bool value)            { _volume.setMuted( value ); };
      void setVolume( int channel, int volume );
      void setRecSource( bool rec ) { _recSource = rec; }
      long getVolume(Volume::ChannelID chid);
      Volume& getVolume();
      long maxVolume();
      long minVolume();

      void setEnumId(int);
      unsigned int enumId();
      QList<QString>& enumValues();

      void read( KConfig *config, const QString& grp );
      void write( KConfig *config, const QString& grp );

      void setType( ChannelType channeltype ) { _type = channeltype; };
      ChannelType type() { return _type; };

      DeviceCategory category() { return _category; };

   signals:
      void newVolume( int num, Volume volume );

   protected:
      Volume _volume;
      ChannelType _type;
      // The DeviceCategory tells, how "important" a MixDevice is.
      // The driver (e.g. mixer_oss.cpp) must set this value. It is
      // used for deciding what Sliders to show and for distributing
      // the sliders. It is advised to use the following categories:
      // BASIC:     Master, PCM
      // PRIMARY:   CD, Headphone, Microphone, Line
      // SECONDARY: All others
      // SWITCH:    All devices which only have a On/Off-Switch
      int _num; // ioctl() device number of mixer
      bool _recordable; // Can it be recorded?
      bool _switch; // On/Off switch // !! remove
      bool _mute; // Available mute option
      bool _recSource; // Current rec status
      DeviceCategory _category; //  category
      QString _name;   // Ascii channel name
      QString _id;     // Primary key, used as part in config file keys
      // A MixDevice, that is an ENUM, has these _enumValues
      QList<QString> _enumValues;
      int _enumCurrentId;

};

#endif

