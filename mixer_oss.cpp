/*
 *              KMix -- KDE's full featured mini mixer
 *
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

// Linux section, by Christian Esken
#if defined(__linux__)
#include <sys/soundcard.h>
// FreeBSD section, according to Sebestyen Zoltan
#elif defined(__FreeBSD__)
#include "machine/soundcard.h"
// NetBSD section, according to  Lennart Augustsson <augustss@cs.chalmers.se>
#elif defined(__NetBSD__)
#include <soundcard.h>
// BSDI section, according to <tom@foo.toetag.com>
#elif defined(__bsdi__)
#include <sys/soundcard.h>
// UnixWare includes
#elif defined(_UNIXWARE)
#include <sys/soundcard.h>
#endif

#include "mixer_oss.h"
#include <klocale.h>


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

Mixer* OSS_getMixer( int device, int card )
{
  Mixer *l_mixer;
  l_mixer = new Mixer_OSS( device, card );
  l_mixer->setupMixer();
  return l_mixer;
}

Mixer* OSS_getMixerSet( MixSet set, int device, int card )
{
  Mixer *l_mixer;
  l_mixer = new Mixer_OSS( device, card );
  l_mixer->setupMixer( set );
  return l_mixer;
}


Mixer_OSS::Mixer_OSS(int device, int card) : Mixer(device, card)
{
  if( device == -1 ) m_devnum = 0;
  if( card   == -1 ) m_cardnum = 0;
}

int Mixer_OSS::openMixer()
{
    if ( m_cardnum!=0 ) return Mixer::ERR_OPEN; // OSS doesn't support different card numbers
  release();            // To be sure, release mixer before (re-)opening

  if ((m_fd= ::open( deviceName( m_devnum ).latin1(), O_RDWR)) < 0)
    {
      if ( errno == EACCES )
        return Mixer::ERR_PERM;
      else
        return Mixer::ERR_OPEN;
    }
  else
    {
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
      if (!devmask)
        return Mixer::ERR_NODEV;
      int maxVolume =100;

      if( m_mixDevices.isEmpty() )
        {
          int idx = 0;
          while( devmask && idx < MAX_MIXDEVS )
            {
              if( devmask & ( 1 << idx ) ) // device active?
                {
                  Volume vol( stereodevs & ( 1 << idx ) ? 2 : 1, maxVolume);
                  readVolumeFromHW( idx, vol );
                  MixDevice* md =
                    new MixDevice( idx, vol, recmask & ( 1 << idx ),
                                   QString(MixerDevNames[idx]),
                                   MixerChannelTypes[idx]);
                  md->setRecsrc( isRecsrcHW( idx ) );
                  m_mixDevices.append( md );
                }
              idx++;
            }
        }
      else
        for( unsigned int idx = 0; idx < m_mixDevices.count(); idx++ )
          {
            MixDevice* md = m_mixDevices.at( idx );
            if( !md )
              return ERR_INCOMPATIBLESET;
            writeVolumeToHW( idx, md->getVolume() );
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
}

int Mixer_OSS::releaseMixer()
{
  int l_i_ret = ::close(m_fd);
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

QString Mixer_OSS::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n("kmix: You do not have permission to access the mixer device.\n" \
                        "Login as root and do a 'chmod a+rw /dev/mixer*' to allow the access.");
      break;
    case ERR_OPEN:
      l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
                        "Please check that the soundcard is installed and the\n" \
                        "soundcard driver is loaded.\n" \
                        "On Linux you might need to use 'insmod' to load the driver.\n" \
                        "Use 'soundon' when using commercial OSS.");
      break;
    default:
      l_s_errmsg = Mixer::errorText(mixer_error);
    }
  return l_s_errmsg;
}


bool Mixer_OSS::setRecsrcHW( int devnum, bool on )
{
  int i_recsrc, oldrecsrc;
  if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
    errormsg(Mixer::ERR_READ);

  oldrecsrc = i_recsrc = on ?
             (i_recsrc | (1 << devnum )) :
             (i_recsrc & ~(1 << devnum ));

  // Change status of record source(s)
  if (ioctl(m_fd, SOUND_MIXER_WRITE_RECSRC, &i_recsrc) == -1)
    errormsg (Mixer::ERR_WRITE);
  // Re-read status of record source(s). Just in case, OSS does not like
  // my settings. And with this line mix->recsrc gets its new value. :-)
  if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
    errormsg(Mixer::ERR_READ);

  // PORTING: Hint: Do not forget to set i_recsrc to the new valid
  //                record source mask.

  return i_recsrc == oldrecsrc;
}

bool Mixer_OSS::isRecsrcHW( int devnum )
{
  int i_recsrc;
  if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
    errormsg(Mixer::ERR_READ);

  return i_recsrc & (1 << devnum );
}


int Mixer_OSS::readVolumeFromHW( int devnum, Volume &vol )
{
  if( vol.isMuted() ) return 0; // Don't alter volume when muted

  int volume;
  if (ioctl(m_fd, MIXER_READ( devnum ), &volume) == -1)
    {
      /* Oops, can't read mixer */
      return(Mixer::ERR_READ);
    }
  else
    {
      vol.setVolume( Volume::LEFT, (volume & 0x7f));
      if( vol.channels() > 1 )
        vol.setVolume( Volume::RIGHT, ((volume>>8) & 0x7f));
      return 0;
    }
}



int Mixer_OSS::writeVolumeToHW( int devnum, Volume vol )
{
  int volume;
  if( vol.isMuted() ) volume = 0;
  else
    if ( vol.channels() > 1 )
      volume = vol[ Volume::LEFT ] + ((vol[ Volume::RIGHT ])<<8);
    else
      volume = vol[ Volume::LEFT ];

  if (ioctl(m_fd, MIXER_WRITE( devnum ), &volume) == -1)
    return Mixer::ERR_WRITE;

  return 0;
}
