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

#include <sys/asoundlib.h>

#include "mixer_alsa.h"
#include "volume.h"

#include <stdlib.h>
#include <stdio.h>

int Mixer_ALSA::identify( int idx, const char* id )
{
  if( !strcmp( id, SND_MIXER_IN_SYNTHESIZER )) return MixDevice::MIDI;
  if( !strcmp( id, SND_MIXER_IN_PCM         )) return MixDevice::AUDIO;
  if( !strcmp( id, SND_MIXER_IN_DAC         )) return MixDevice::AUDIO;
  if( !strcmp( id, SND_MIXER_IN_FM          )) return MixDevice::AUDIO;
  if( !strcmp( id, SND_MIXER_IN_DSP         )) return MixDevice::AUDIO;
  if( !strcmp( id, SND_MIXER_IN_LINE        )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_MIC         )) return MixDevice::MICROPHONE;
  if( !strcmp( id, SND_MIXER_IN_CD          )) return MixDevice::CD;
  if( !strcmp( id, SND_MIXER_IN_VIDEO       )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_RADIO       )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_PHONE       )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_MONO        )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_SPEAKER     )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_AUX         )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_CENTER      )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_WOOFER      )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_SURROUND    )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_MASTER        ))
    {
      m_masterDevice = idx;
      return MixDevice::VOLUME;
    }
  if( !strcmp( id, SND_MIXER_OUT_MASTER_MONO   )) return MixDevice::VOLUME;
  if( !strcmp( id, SND_MIXER_OUT_MASTER_DIGITAL )) return MixDevice::VOLUME;
  if( !strcmp( id, SND_MIXER_OUT_HEADPHONE  )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_PHONE      )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_CENTER     )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_WOOFER     )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_SURROUND   )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_DSP        )) return MixDevice::AUDIO;
  return MixDevice::UNKNOWN;
}

Mixer* Mixer::getMixer( int device, int card )
{
  Mixer *l_mixer;
  l_mixer = new Mixer_ALSA( device, card );
  l_mixer->setupMixer();
  return l_mixer;
}

Mixer* Mixer::getMixer( MixSet set, int device, int card )
{
  Mixer *l_mixer;
  l_mixer = new Mixer_ALSA( device, card );
  l_mixer->setupMixer( set );
  return l_mixer;
}



Mixer_ALSA::Mixer_ALSA( int device, int card ) : Mixer( device, card ),
  handle(0), gid(0)
{  
   groups.pgroups = 0;
}


Mixer_ALSA::~Mixer_ALSA()
{
   kdDebug() << "-> Mixer_ALSA::~Mixer_ALSA" << endl;
   if ( groups.pgroups ) free(groups.pgroups);
   kdDebug() << "<- Mixer_ALSA::~Mixer_ALSA" << endl;
}

int Mixer_ALSA::openMixer()
{
  release();		// To be sure, release mixer before (re-)opening

  bool initset = m_mixDevices.isEmpty();

  int err, idx;

  if( m_cardnum == -1 ) m_cardnum = snd_defaults_mixer_card();
  if( m_devnum == -1 ) m_devnum = snd_defaults_mixer_device(); /* card 0 mixer 0 */

  if ((err = snd_mixer_open( &handle, m_cardnum, m_devnum )) < 0 )
    return Mixer::ERR_OPEN;

  bool deviceWork = false;

  bzero(&groups, sizeof(groups));
  if ((err = snd_mixer_groups(handle, &groups)) < 0) {
    return Mixer::ERR_NODEV;
  }
  groups.pgroups = (snd_mixer_gid_t *)
                   malloc(groups.groups_over * sizeof(snd_mixer_eid_t));
  if (!groups.pgroups) {
    return Mixer::ERR_NOMEM;
  }
  groups.groups_size = groups.groups_over;
  groups.groups_over = groups.groups = 0;
  if ((err = snd_mixer_groups(handle, &groups)) < 0) {
    return Mixer::ERR_READ;
  }
  for (idx = 0; idx < groups.groups; idx++) {
    gid = &groups.pgroups[idx];

    snd_mixer_group_t group;

    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ((err = snd_mixer_group_read(handle, &group)) < 0)
      return Mixer::ERR_READ;

    bool ismono = true;
    bool canrecord = false;

    int channels = numChannels( group.channels );

    if ( group.channels && (group.caps & SND_MIXER_GRPCAP_VOLUME) ) {
      if ( channels > 1 )
        ismono = false;
      if ( group.caps & SND_MIXER_GRPCAP_CAPTURE )
        canrecord = true;
      deviceWork = true;

      int maxVolume = group.max;
      //  	ret = snd_mixer_channel_read( devhandle, i, &data );
      //  	if ( !ret ) {
      //  	  if ( data.flags & SND_MIXER_FLG_RECORD )
      //  	    i_recsrc |= 1 << i;
      //  	}
      MixDevice::ChannelType ct = (MixDevice::ChannelType)identify( idx, (const char *)gid->name );
      if( initset )
        {
          Volume vol( channels, maxVolume);
          readVolumeFromHW( idx, vol );
          m_mixDevices.append(
                              new MixDevice( idx, vol, canrecord,
                                             QString((char*)gid->name), ct ));
        }
      else
        {
          MixDevice* md = m_mixDevices.at( idx );
          if( !md )
            return ERR_INCOMPATIBLESET;
          writeVolumeToHW( idx, md->getVolume() );
        }
    }
  }
  if ( !deviceWork ) {
    return Mixer::ERR_NODEV;
  }

  snd_mixer_info_t info;
  if ((err = snd_mixer_info(handle, &info)) < 0)
    return Mixer::ERR_READ;
  m_mixerName = QString((char*)info.name);

  m_isOpen = true;
  return 0;
}


int Mixer_ALSA::releaseMixer()
{
  int ret = snd_mixer_close(handle);
  return ret;
}

bool Mixer_ALSA::isRecsrcHW( int devnum )
{
    gid = &groups.pgroups[devnum];

    snd_mixer_group_t group;

    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ( snd_mixer_group_read(handle, &group) < 0)
      return Mixer::ERR_READ;

    return group.capture && SND_MIXER_CHN_MASK_FRONT_LEFT;
}

bool Mixer_ALSA::setRecsrcHW( int devnum, bool on )
{
    snd_mixer_open( &handle, m_cardnum, m_devnum );
    gid = &groups.pgroups[devnum];

    snd_mixer_group_t group;

    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ( snd_mixer_group_read(handle, &group) < 0)
      return true;

    group.capture = on ? group.capture | SND_MIXER_CHN_MASK_FRONT_LEFT :
      group.capture & ~SND_MIXER_CHN_MASK_FRONT_LEFT;
    if ( numChannels( group.channels ) > 1 )
      {
        group.capture = on ? ~0 : 0;
      }
    if ( snd_mixer_group_write(handle, &group) < 0)
      return true;

    return false;
}

int Mixer_ALSA::numChannels( int mask )
{
  int channels = 0;
  if ( mask & SND_MIXER_CHN_MASK_FRONT_LEFT ) channels++;
  if ( mask & SND_MIXER_CHN_MASK_FRONT_RIGHT   ) channels++;
  if ( mask & SND_MIXER_CHN_MASK_FRONT_CENTER  ) channels++;
  if ( mask & SND_MIXER_CHN_MASK_REAR_LEFT     ) channels++;
  if ( mask & SND_MIXER_CHN_MASK_REAR_RIGHT    ) channels++;
  if ( mask & SND_MIXER_CHN_MASK_WOOFER ) channels++;
  return channels;
}

int Mixer_ALSA::readVolumeFromHW( int devnum, Volume &volume )
{
  gid = &groups.pgroups[devnum];

  snd_mixer_group_t group;

  bzero(&group, sizeof(group));
  group.gid = *gid;
  if ( snd_mixer_group_read(handle, &group) < 0)
    return Mixer::ERR_READ;

  int leftvol, rightvol;

  volume.setVolume( Volume::LEFT,
                    leftvol = group.volume.values[SND_MIXER_CHN_FRONT_LEFT] );
  if ( volume.channels() > 1 )
    {
      volume.setVolume( Volume::RIGHT,
                        rightvol =group.volume.values[SND_MIXER_CHN_FRONT_RIGHT] );

      if( devnum == m_masterDevice ) // correct balance
        {
          if( leftvol != rightvol )
            m_balance = leftvol > rightvol ?
                        - (leftvol - rightvol) * 100 / leftvol :
                         (rightvol - leftvol) * 100 / rightvol;
          else
            m_balance = 0;
        }
    }

  return 0;
}


int Mixer_ALSA::writeVolumeToHW( int devnum, Volume volume )
{
    snd_mixer_open( &handle, m_cardnum, m_devnum );
    gid = &groups.pgroups[devnum];

    snd_mixer_group_t group;

    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ( snd_mixer_group_read(handle, &group) < 0)
      return Mixer::ERR_READ;

    group.mute = volume.isMuted() ? group.mute | SND_MIXER_CHN_MASK_FRONT_LEFT :
      group.mute & ~SND_MIXER_CHN_MASK_FRONT_LEFT;
    group.volume.values[SND_MIXER_CHN_FRONT_LEFT] = volume[ Volume::LEFT ];
    if ( numChannels( group.channels ) > 1 )
      {
        group.volume.values[SND_MIXER_CHN_FRONT_RIGHT] = volume[ Volume::RIGHT ];
        group.mute = volume.isMuted() ? ~0 : 0;
      }
    if ( snd_mixer_group_write(handle, &group) < 0)
      return Mixer::ERR_WRITE;

    return 0;
}

