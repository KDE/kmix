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

#include <stdlib.h>
#include <stdio.h>

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_ALSA( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}



Mixer_ALSA::Mixer_ALSA() : Mixer(), 
  handle(0), gid(0) 
{};

Mixer_ALSA::Mixer_ALSA(int devnum, int SetNum) : Mixer(devnum, SetNum),
  handle(0), gid(0)
{};

Mixer_ALSA::~Mixer_ALSA()
{
  if ( groups.pgroups ) free(groups.pgroups);
}

MixDevice* Mixer_ALSA::createNewMixDevice(int i)
{
  if ( groups.pgroups ) {
    gid = &groups.pgroups[i];
    return new MixDevice( i, QCString( (const char*)gid->name, 24 ) );
  } else
    return new MixDevice( i );
}

int Mixer_ALSA::openMixer()
{
  release();		// To be sure, release mixer before (re-)opening

  int err, idx;

  if ((err = snd_mixer_open( &handle,
                             snd_defaults_mixer_card(),
                             snd_defaults_mixer_device())) /* card 0 mixer 0 */
      < 0 )
    return Mixer::ERR_OPEN;
  
  devmask = recmask = i_recsrc = stereodevs = 0;

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

    if ( group.channels && (group.caps & SND_MIXER_GRPCAP_VOLUME) ) {
      if (group.channels == SND_MIXER_CHN_MASK_STEREO) 
        stereodevs |= 1 << idx;
      if ( group.caps & SND_MIXER_GRPCAP_CAPTURE )
        recmask |= 1 << idx;
      devmask |= 1 << idx;

      MaxVolume = group.max;
        //  	ret = snd_mixer_channel_read( devhandle, i, &data );
//  	if ( !ret ) {
//  	  if ( data.flags & SND_MIXER_FLG_RECORD )
//  	    i_recsrc |= 1 << i;
//  	}
    }
  }
  if ( !devmask ) {
    return Mixer::ERR_NODEV;
  }

  snd_mixer_info_t info;
  if ((err = snd_mixer_info(handle, &info)) < 0) 
    return Mixer::ERR_READ;
  i_s_mixer_name = (const char*)info.name;

  isOpen = true;
  return 0;
}


int Mixer_ALSA::releaseMixer()
{
  int ret = snd_mixer_close(handle);
  return ret;
}

void Mixer_ALSA::setDevNumName_I(int devnum)
{
  devnum=0; //TODO
  devname = "ALSA";
}

void Mixer_ALSA::setRecsrc(unsigned int newRecsrc)
{
//    snd_mixer_channel_t data;
//    i_recsrc = 0;

//    MixDevice *MixPtr;
//    for ( unsigned int l_i_mixDevice = 1; l_i_mixDevice <= size(); l_i_mixDevice++) {
//      MixPtr = operator[](l_i_mixDevice);

//      ret = snd_mixer_channel_read( devhandle, MixDev->num(), &data ); /* get */
//      if ( ret )
//        errormsg(Mixer::ERR_READ);
//      if ( newRecsrc & ( 1 << MixDev->num() ) )
//        data.flags |= SND_MIXER_FLG_RECORD;
//      else
//        data.flags &= ~SND_MIXER_FLG_RECORD;
//      ret = snd_mixer_channel_write( devhandle, MixDev->num(), &data ); /* set */
//      if ( ret )
//        errormsg(Mixer::ERR_WRITE);
//      ret = snd_mixer_channel_read( devhandle, MixDev->num(), &data ); /* check */
//      if ( ret )
//        errormsg(Mixer::ERR_READ);
//      if ( ( data.flags & SND_MIXER_FLG_RECORD ) && /* if it's set and stuck */
//           ( newRecsrc & ( 1 << MixDev->num() ) ) ) {
//        i_recsrc |= 1 << MixDev->num();
//        MixDev->setRecsrc(true);
//      }
//      else {
//        MixDev->setRecsrc(false);
//      }
//    }
//    return; /* I'm done */
}


int Mixer_ALSA::readVolumeFromHW( int devnum, int *VolLeft, int *VolRight )
{
    gid = &groups.pgroups[devnum];

    snd_mixer_group_t group;

    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ( snd_mixer_group_read(handle, &group) < 0)
      return Mixer::ERR_READ;

    *VolLeft = group.volume.values[SND_MIXER_CHN_FRONT_LEFT];
    if (group.channels == SND_MIXER_CHN_MASK_MONO) 
      *VolRight = group.volume.values[SND_MIXER_CHN_FRONT_LEFT];
    else
      *VolRight = group.volume.values[SND_MIXER_CHN_FRONT_RIGHT];

    printf("READ - Devnum: %d, Left: %d, Right: %d\n", devnum, *VolLeft, *VolRight );
    
   return 0;
}


int Mixer_ALSA::writeVolumeToHW( int devnum, int volLeft, int volRight )
{
    printf("WRITE - Devnum: %d, Left: %d, Right: %d\n", devnum, volLeft, volRight );

    gid = &groups.pgroups[devnum];

    snd_mixer_group_t group;

    bzero(&group, sizeof(group));
    group.gid = *gid;
    if ( snd_mixer_group_read(handle, &group) < 0)
      return Mixer::ERR_READ;

    group.volume.values[SND_MIXER_CHN_FRONT_LEFT] = volLeft;
    group.volume.values[SND_MIXER_CHN_FRONT_RIGHT] = volRight;

    if ( snd_mixer_group_write(handle, &group) < 0)
      return Mixer::ERR_WRITE;

    return 0;
}

