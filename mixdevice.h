#ifndef MixDevice_h
#define MixDevice_h

#include "volume.h"
#include <qstring.h>
#include <kconfig.h>
#include <qobject.h>

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
                        VIDEO, SURROUND, HEADPHONE, DIGITAL, AC97 };

      // The DeviceCategory tells, how "important" a MixDevice is. See _category.
      // It is used in bitmasks, so you must use values of 2^n .
      enum DeviceCategory { SLIDER=0x01, SWITCH=0x02, ALL=0xff };


      MixDevice(int num, Volume &vol, bool recordable, bool mute,
                                QString name, ChannelType type = UNKNOWN, DeviceCategory category =
SLIDER );
      MixDevice(const MixDevice &md);
      ~MixDevice() {};

      int num()                    { return _num; };
      QString   name()         { return _name; };
      bool isStereo()        { return (_volume.channels() > 1); };
      bool isRecordable()    { return _recordable; };
      bool isRecSource()    { return _recSource; };
      bool isSwitch()        { return _switch; }
      bool isMuted()         { return _volume.isMuted(); };
      bool hasMute()         { return _mute; }

      void setMuted(bool value)            { _volume.setMuted( value ); };
      //void setVolume( Volume &volume );
      void setVolume( int channel, int volume );
      void setRecSource( bool rec ) { _recSource = rec; }
      //long getVolume( int channel );
      long getVolume(Volume::ChannelID chid);
      Volume& getVolume();
      long getAvgVolume();
  long maxVolume();
  long minVolume();

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
      bool _switch; // On/Off switch
      bool _mute; // Available mute option
      bool _recSource; // Current rec status
      DeviceCategory _category; //  category
      QString _name;   // Ascii channel name
};

#endif

