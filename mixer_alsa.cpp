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
  if( !strcmp( id, SND_MIXER_IN_VIDEO       )) return MixDevice::VIDEO;
  if( !strcmp( id, SND_MIXER_IN_RADIO       )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_PHONE       )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_MONO        )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_SPEAKER     )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_AUX         )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_CENTER      )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_IN_WOOFER      )) return MixDevice::BASS;
  if( !strcmp( id, SND_MIXER_IN_SURROUND    )) return MixDevice::SURROUND;
  if( !strcmp( id, "Rear" )) return MixDevice::SURROUND; // SB Live Rear
  if( !strcmp( id, SND_MIXER_OUT_MASTER ))
    {
      m_masterDevice = idx;
      return MixDevice::VOLUME;
    }
  if( !strcmp( id, SND_MIXER_OUT_MASTER_MONO   )) return MixDevice::VOLUME;
  if( !strcmp( id, SND_MIXER_OUT_MASTER_DIGITAL )) return MixDevice::VOLUME;
  if( !strcmp( id, SND_MIXER_OUT_HEADPHONE  )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_PHONE      )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_CENTER     )) return MixDevice::EXTERNAL;
  if( !strcmp( id, SND_MIXER_OUT_WOOFER     )) return MixDevice::BASS;
  if( !strcmp( id, SND_MIXER_OUT_SURROUND   )) return MixDevice::SURROUND;
  if( !strcmp( id, SND_MIXER_OUT_DSP        )) return MixDevice::AUDIO;
  return MixDevice::UNKNOWN;
}

Mixer* ALSA_getMixer( int device, int card )
{
  Mixer *l_mixer;
  l_mixer = new Mixer_ALSA( device, card );
  l_mixer->setupMixer();
  return l_mixer;
}

Mixer* ALSA_getMixerSet( MixSet set, int device, int card )
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
   if ( groups.pgroups ) free(groups.pgroups);
}

void printGroup( snd_mixer_group_t *grp )
{
   kdDebug() << "gid = " << QString((char*)grp->gid.name) << endl;
   kdDebug() << " elements = " << grp->elements << endl;

   for ( int n=0; n<grp->elements; n++ )
      kdDebug() << "  #" << n << " name = " << QString((char*)grp->pelements[n].name) << endl;

   kdDebug() << " caps = " << grp->caps << endl;
   kdDebug() << " channels = " << grp->channels << endl;
   kdDebug() << " mute = " << grp->mute << endl;
   kdDebug() << " capture = " << grp->capture << endl;
   kdDebug() << " capture_group = " << grp->capture_group << endl;
   kdDebug() << " min = " << grp->min << endl;
   kdDebug() << " max = " << grp->max << endl;
   kdDebug() << " front_left = " << grp->volume.names.front_left << endl;
   kdDebug() << " front_right = " << grp->volume.names.front_right << endl;
   kdDebug() << " front_center = " << grp->volume.names.front_center << endl;
   kdDebug() << " rear_left = " << grp->volume.names.rear_left << endl;
   kdDebug() << " rear_right = " << grp->volume.names.rear_right << endl;
   kdDebug() << " woofer = " << grp->volume.names.woofer << endl;
}

int Mixer_ALSA::openMixer()
{
  bool virginOpen = m_mixDevices.isEmpty();
  int err;

  // only mixers with valid channels allowed
  bool validDevice = false;

  // release mixer before (re-)opening
  release();

  // default mixers?
  if( m_cardnum == -1 ) m_cardnum = snd_defaults_mixer_card();
  if( m_devnum == -1 ) m_devnum = snd_defaults_mixer_device();

  // open mixer
  if ((err = snd_mixer_open( &handle, m_cardnum, m_devnum )) < 0 )
     return Mixer::ERR_OPEN;

  // get number of groups
  bzero(&groups, sizeof(groups));
  if ((err = snd_mixer_groups(handle, &groups)) < 0)
     return Mixer::ERR_NODEV;

  // get groups
  groups.pgroups = (snd_mixer_gid_t *)malloc(groups.groups_over * sizeof(snd_mixer_eid_t));
  if (!groups.pgroups) return Mixer::ERR_NOMEM;
  groups.groups_size = groups.groups_over;
  groups.groups_over = groups.groups = 0;
  if ((err = snd_mixer_groups(handle, &groups)) < 0) return Mixer::ERR_READ;

  // iterate through groups
  for ( int groupNum = 0; groupNum < groups.groups; groupNum++ )
  {
     gid = &groups.pgroups[groupNum];

     // get group info
     snd_mixer_group_t group;
     bzero(&group, sizeof(group));
     group.gid = *gid;
     if ( (err = snd_mixer_group_read(handle, &group))<0 )  return Mixer::ERR_READ;

     // printGroup( &group );

     // parse group caps
     bool isMono = true;
     bool canRecord = false;
     int channels = numChannels( group.channels );

     if ( group.channels && (group.caps & SND_MIXER_GRPCAP_VOLUME) )
     {
        validDevice = true;
        if ( channels>1 ) isMono = false;
        if ( group.caps & SND_MIXER_GRPCAP_CAPTURE ) canRecord = true;

        // create mix device
        int maxVolume = group.max;
        MixDevice::ChannelType ct =
           (MixDevice::ChannelType)identify( groupNum, (const char *)gid->name );
        if( virginOpen )
        {
           Volume vol( channels, maxVolume);
           readVolumeFromHW( groupNum, vol );
           m_mixDevices.append(
              new MixDevice( groupNum, vol, canRecord, QString((char*)gid->name), ct) );
        } else
        {
           MixDevice* md = m_mixDevices.at( groupNum );
           if( !md ) return ERR_INCOMPATIBLESET;
           writeVolumeToHW( groupNum, md->getVolume() );
        }
     }
  }

  // return error for invlid devices
  if ( !validDevice ) return Mixer::ERR_NODEV;

  // get mixer name
  snd_mixer_info_t info;
  if ( (err = snd_mixer_info(handle, &info))<0 ) return Mixer::ERR_READ;
  m_mixerName = QString((char*)info.name);

  // return with success
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

    // get device caps to check for capture
    snd_mixer_group_t group;
    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ( snd_mixer_group_read(handle, &group)<0 ) return Mixer::ERR_READ;

    return group.capture && SND_MIXER_CHN_MASK_FRONT_LEFT;
}

bool Mixer_ALSA::setRecsrcHW( int devnum, bool on )
{
    snd_mixer_open( &handle, m_cardnum, m_devnum );
    gid = &groups.pgroups[devnum];

    // get current device caps
    snd_mixer_group_t group;
    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ( snd_mixer_group_read(handle, &group)<0 ) return true;

    // set capture flag
    group.capture = on ? group.capture | SND_MIXER_CHN_MASK_FRONT_LEFT :
       group.capture & ~SND_MIXER_CHN_MASK_FRONT_LEFT;
    if ( numChannels(group.channels)>1 ) group.capture = on ? ~0 : 0;

    // write caps back
    if ( snd_mixer_group_write(handle, &group)<0 ) return true;

    return false;
}

int Mixer_ALSA::numChannels( int mask )
{
  int channels = 0;
  // count set bits
  while ( mask )
  {
     channels += (mask & 1);
     mask = mask >> 1;
  }

  return channels;
}

int Mixer_ALSA::readVolumeFromHW( int devnum, Volume &volume )
{
   gid = &groups.pgroups[devnum];

   // read device info to get volumes
   snd_mixer_group_t group;
   bzero(&group, sizeof(group));
   group.gid = *gid;
   if ( snd_mixer_group_read(handle, &group)<0 ) return Mixer::ERR_READ;

   // update mute flag
   volume.setMuted( group.mute!=0 );

   // read volumes channel for channel
   int volChannel = 0;
   int leftvol = 1;
   int rightvol = 1;
   for ( int channel=0; channel<32; channel++ )
   {
      if ( group.channels & (1 << channel) )
      {
         volume.setVolume( volChannel, group.volume.values[channel] );
         if ( volChannel==0 ) leftvol = group.volume.values[channel];

         // correct balance
         if( volChannel==1 && devnum==m_masterDevice )
         {
            rightvol = group.volume.values[channel];
            if( leftvol!=rightvol )
            {
               m_balance = (leftvol>rightvol) ?
                  (-(leftvol-rightvol)*100/leftvol) :
                  (+(rightvol-leftvol)*100/rightvol);
            } else
               m_balance = 0;
         }

         volChannel++;
      }
   }

   return 0;
}


int Mixer_ALSA::writeVolumeToHW( int devnum, Volume volume )
{
   snd_mixer_open( &handle, m_cardnum, m_devnum );
   gid = &groups.pgroups[devnum];

   // get current group info
   snd_mixer_group_t group;
   bzero(&group, sizeof(group));
   group.gid = *gid;
   if ( snd_mixer_group_read(handle, &group)<0 ) return Mixer::ERR_READ;

   // update muting flags
   group.mute = volume.isMuted() ? ~0 : 0;

   // write volumes channel for channel
   int volChannel = 0;
   for ( int channel=0; channel<32; channel++ )
   {
      if ( group.channels & (1 << channel) )
      {
         group.volume.values[channel] = volume[volChannel];
         volChannel++;
      }
   }

   // write volumes back
   if ( snd_mixer_group_write(handle, &group)<0 ) return Mixer::ERR_WRITE;

   return 0;
}

