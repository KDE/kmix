/*
 *              KMix -- KDE's full featured mini mixer
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
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

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

// Since we're guaranteed an OSS setup here, let's make life easier
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
	#include <sys/soundcard.h>
#else
	#include <soundcard.h>
#endif

#include "mixer_oss.h"
#include <klocale.h>

/*
  I am using a fixed MAX_MIXDEVS #define here.
   People might argue, that I should rather use the SOUND_MIXER_NRDEVICES
   #define used by OSS. But using this #define is not good, because it is
   evaluated during compile time. Compiling on one platform and running
   on another with another version of OSS with a different value of
   SOUND_MIXER_NRDEVICES is very bad. Because of this, usage of
   SOUND_MIXER_NRDEVICES should be discouraged.

   The #define below is only there for internal reasons.
   In other words: Don't play around with this value
 */
#define MAX_MIXDEVS 32

const char* MixerDevNames[32]={
    I18N_NOOP("Volume"), I18N_NOOP("Bass"), I18N_NOOP("Treble"),
    I18N_NOOP("Synth"), I18N_NOOP("Pcm"), I18N_NOOP("Speaker"),
    I18N_NOOP("Line"), I18N_NOOP("Microphone"), I18N_NOOP("CD"),
    I18N_NOOP("Mix"), I18N_NOOP("Pcm2"), I18N_NOOP("RecMon"),
    I18N_NOOP("IGain"), I18N_NOOP("OGain"), I18N_NOOP("Line1"),
    I18N_NOOP("Line2"), I18N_NOOP("Line3"), I18N_NOOP("Digital1"),
    I18N_NOOP("Digital2"), I18N_NOOP("Digital3"), I18N_NOOP("PhoneIn"),
    I18N_NOOP("PhoneOut"), I18N_NOOP("Video"), I18N_NOOP("Radio"),
    I18N_NOOP("Monitor"), I18N_NOOP("3D-depth"), I18N_NOOP("3D-center"),
    I18N_NOOP("unknown"), I18N_NOOP("unknown"), I18N_NOOP("unknown"),
    I18N_NOOP("unknown") , I18N_NOOP("unused") };

const MixDevice::ChannelType MixerChannelTypes[32] = {
  MixDevice::VOLUME,   MixDevice::BASS,       MixDevice::TREBLE,   MixDevice::MIDI,
  MixDevice::AUDIO,    MixDevice::EXTERNAL,   MixDevice::EXTERNAL, MixDevice::MICROPHONE,
  MixDevice::CD,       MixDevice::VOLUME,     MixDevice::AUDIO,    MixDevice::RECMONITOR,
  MixDevice::VOLUME,   MixDevice::RECMONITOR, MixDevice::EXTERNAL, MixDevice::EXTERNAL,
  MixDevice::EXTERNAL, MixDevice::AUDIO,      MixDevice::AUDIO,    MixDevice::AUDIO,
  MixDevice::EXTERNAL, MixDevice::EXTERNAL,   MixDevice::EXTERNAL, MixDevice::EXTERNAL,
  MixDevice::EXTERNAL, MixDevice::VOLUME,     MixDevice::VOLUME,   MixDevice::UNKNOWN,
  MixDevice::UNKNOWN,  MixDevice::UNKNOWN,    MixDevice::UNKNOWN,  MixDevice::UNKNOWN };

Mixer_Backend* OSS_getMixer( int device )
{
  Mixer_Backend *l_mixer;
  l_mixer = new Mixer_OSS( device );
  return l_mixer;
}

Mixer_OSS::Mixer_OSS(int device) : Mixer_Backend(device)
{
  if( device == -1 ) m_devnum = 0;
}

Mixer_OSS::~Mixer_OSS()
{
  close();
}

int Mixer_OSS::open()
{
  if ((m_fd= ::open( deviceName( m_devnum ).toAscii().data(), O_RDWR)) < 0)
    {
      if ( errno == EACCES )
        return Mixer::ERR_PERM;
      else {
		  if ((m_fd= ::open( deviceNameDevfs( m_devnum ).toAscii().data(),
						  O_RDWR)) < 0)
		    {
      			if ( errno == EACCES )
        			return Mixer::ERR_PERM;
				else
					return Mixer::ERR_OPEN;
		    }
	  }
    }

      int devmask, recmask, i_recsrc, stereodevs;
      // Mixer is open. Now define properties
      if (ioctl(m_fd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
        return Mixer::ERR_READ;
      if (ioctl(m_fd, SOUND_MIXER_READ_RECMASK, &recmask) == -1)
        return Mixer::ERR_READ;
      if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
        return Mixer::ERR_READ;
      if (ioctl(m_fd, SOUND_MIXER_READ_STEREODEVS, &stereodevs) == -1)
        return Mixer::ERR_READ;

      int maxVolume =100;

          int idx = 0;
          while( devmask && idx < MAX_MIXDEVS )
            {
              if( devmask & ( 1 << idx ) ) // device active?
                {
                  // recmask & ( 1 << idx ), true,
                  Volume vol( stereodevs & ( 1 << idx ) ? 2 : 1, maxVolume);
                  Volume volCapture;
                  QString id;
                  id.setNum(idx);
                  MixDevice* md =
                    new MixDevice( id, vol, volCapture,
                                   i18n(MixerDevNames[idx]),
                                   MixerChannelTypes[idx]);
                  m_mixDevices.append( md );
                }
              idx++;
            }

#if !defined(__FreeBSD__)
      struct mixer_info l_mix_info;
      if (ioctl(m_fd, SOUND_MIXER_INFO, &l_mix_info) != -1)
        {
          m_mixerName = l_mix_info.name;
        }
      else
#endif

        m_mixerName = "OSS Audio Mixer";

      m_isOpen = true;
      return 0;
}

int Mixer_OSS::close()
{
    _pollingTimer->stop();
    m_isOpen = false;
    int l_i_ret = ::close(m_fd);
    m_mixDevices.clear();
    return l_i_ret;
}


QString Mixer_OSS::deviceName(int devnum)
{
  switch (devnum) {
  case 0:
    return QString("/dev/mixer");
    break;

  default:
    QString devname("/dev/mixer");
    devname += ('0'+devnum);
    return devname;
  }
}

QString Mixer_OSS::deviceNameDevfs(int devnum)
{
  switch (devnum) {
  case 0:
    return QString("/dev/sound/mixer");
    break;

  default:
    QString devname("/dev/sound/mixer");
    devname += ('0'+devnum);
    return devname;
  }
}

QString Mixer_OSS::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case Mixer::ERR_PERM:
      l_s_errmsg = i18n("kmix: You do not have permission to access the mixer device.\n" \
                        "Login as root and do a 'chmod a+rw /dev/mixer*' to allow the access.");
      break;
    case Mixer::ERR_OPEN:
      l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
                        "Please check that the soundcard is installed and the\n" \
                        "soundcard driver is loaded.\n" \
                        "On Linux you might need to use 'insmod' to load the driver.\n" \
                        "Use 'soundon' when using commercial OSS.");
      break;
    default:
      l_s_errmsg = Mixer_Backend::errorText(mixer_error);
    }
  return l_s_errmsg;
}

/*
bool Mixer_OSS::isRecsrcHW( const QString& id )
{
   int devnum = id2num(id);
   bool isRecsrc = false;
   int recsrcMask;
   if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &recsrcMask) == -1)
      errormsg(Mixer::ERR_READ);
   else {
      // test if device bit is set in record bit mask
      isRecsrc =  ( (recsrcMask & ( 1<<devnum)) != 0 );
   }
   return isRecsrc;
}
*/


void Mixer_OSS::setRecsrcHW( const QString& id, bool on )
{
    int i_recsrc, oldrecsrc;
    int devnum = id2num(id);
    if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
        errormsg(Mixer::ERR_READ);

    oldrecsrc = i_recsrc = on ?
             (i_recsrc | (1 << devnum )) :
             (i_recsrc & ~(1 << devnum ));

    // Change status of record source(s)
    if (ioctl(m_fd, SOUND_MIXER_WRITE_RECSRC, &i_recsrc) == -1)
    {
        errormsg (Mixer::ERR_WRITE);
    }
    else {
        // Re-read status of record source(s). Just in case the hardware/driver has
        // some limitaton (like exclusive switches)
        int recsrcMask;
        if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &recsrcMask) == -1)
            errormsg(Mixer::ERR_READ);
        else
        {
            for(int i=0; i< m_mixDevices.count() ; i++ )
            {
                MixDevice *md = m_mixDevices[i];
                bool isRecsrc =  ( (recsrcMask & ( 1<<devnum)) != 0 );
                md->captureVolume().setSwitch(isRecsrc);
            } // for all controls
        } // reading newrecsrcmask is OK
    }
    
}


int Mixer_OSS::readVolumeFromHW( const QString& id, MixDevice* md )
{
    int ret = 0;

    // --- VOLUME ---
    Volume& vol = md->playbackVolume();
    int devnum = id2num(id);

    if ( vol.hasVolume() ) {
        int volume;
         if (ioctl(m_fd, MIXER_READ( devnum ), &volume) == -1)
        {
            /* Oops, can't read mixer */
            ret = Mixer::ERR_READ;
        }
        else
        {
            vol.setVolume( Volume::LEFT, (volume & 0x7f));
            if( vol.count() > 1 )
                vol.setVolume( Volume::RIGHT, ((volume>>8) & 0x7f));
        }
    }


    // --- RECORD SWITCH ---
    int recsrcMask;
    if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &recsrcMask) == -1)
        ret = Mixer::ERR_READ;
    else {
        // test if device bit is set in record bit mask
        bool isRecsrc =  ( (recsrcMask & ( 1<<devnum)) != 0 );
        md->captureVolume().setSwitch(isRecsrc);
    }

    return ret;
}



int Mixer_OSS::writeVolumeToHW( const QString& id, MixDevice *md)
{
    int volume;
    int devnum = id2num(id);

    Volume& vol = md->playbackVolume();
    if( vol.isSwitchActivated() )
       volume = 0;
    else
    {
        if ( vol.count() > 1 )
            volume = (vol[ Volume::LEFT ]) + ((vol[ Volume::RIGHT ])<<8);
        else
            volume = vol[ Volume::LEFT ];
    }
    
    if (ioctl(m_fd, MIXER_WRITE( devnum ), &volume) == -1)
        return Mixer::ERR_WRITE;

    return 0;
}

QString OSS_getDriverName() {
   return "OSS";
}

QString Mixer_OSS::getDriverName() {
   return "OSS";
}

