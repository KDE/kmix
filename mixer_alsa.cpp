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

#include <sys/soundlib.h>
void *devhandle;
int ret;

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_ALSA( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}



Mixer_ALSA::Mixer_ALSA() : Mixer() { };
Mixer_ALSA::Mixer_ALSA(int devnum, int SetNum) : Mixer(devnum, SetNum);

int Mixer_ALSA::openMixer()
{
  release();		// To be sure, release mixer before (re-)opening

  int ret = snd_mixer_open( &devhandle, 0, 0 ); /* card 0 mixer 0 */
  if ( ret ) {
    return Mixer::ERR_OPEN;
  }
  else {
    // Mixer is open. Now define properties
    snd_mixer_channel_info_t chinfo;
    snd_mixer_channel_t data;
    int num, i;
    devmask=recmask=i_recsrc=stereodevs=0;
    MaxVolume = 100;
    
    num = snd_mixer_channels( devhandle );
    if ( num < 0 )
      return Mixer::ERR_NODEV;
    for( i=0;i<=num; i++ ) {
      ret = snd_mixer_channel_info(devhandle, i, &chinfo);
      if ( !ret ) {
	if ( chinfo.caps & SND_MIXER_CINFO_CAP_STEREO )
	  stereodevs |= 1 << i;
	if ( chinfo.caps & SND_MIXER_CINFO_CAP_RECORD )
	  recmask |= 1 << i;
	devmask |= 1 << i;
	ret = snd_mixer_channel_read( devhandle, i, &data );
	if ( !ret ) {
	  if ( data.flags & SND_MIXER_FLG_RECORD )
	    i_recsrc |= 1 << i;
	}
      }
    }    
    if ( !devmask ) {
      return Mixer::ERR_NODEV;
    }

    i_s_mixer_name = "ALSA Audio Mixer"

    isOpen = true;
    return 0;
  }
}

int Mixer_ALSA::releaseMixer()
{
  ret = snd_mixer_close(devhandle);
  return ret;
}

void Mixer_ALSA::setDevNumName_I(int devnum)
{
  devname = "ALSA";
}

void Mixer_ALSA::readVolumeFromHW( int devnum, int *VolLeft, int *VolRight )
{
  snd_mixer_channel_t data;
  ret = snd_mixer_channel_read( devhandle, devnum(), &data );
  if ( !ret ) {
    *VolLeft = data.left;
    *VolRight = data.right;
  }
  else 
    errormsg(Mixer::ERR_READ);
}
