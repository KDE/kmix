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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/audioio.h>

#include "mixer_sun.h"

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_SUN( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}


Mixer_SUN::Mixer_SUN() : Mixer() { };
Mixer_SUN::Mixer_SUN(int devnum, int SetNum) : Mixer(devnum, SetNum);
void Mixer_SUN::setDevNumName_I(int devnum)
{
  devname = "/dev/audioctl";
}

int Mixer_SUN::openMixer()
{
  release();		// To be sure, release mixer before (re-)opening

  if ((fd= open(devname.latin1(), O_RDWR)) < 0) {
    if ( errno == EACCES )
      return Mixer::ERR_PERM;
    else
      return Mixer::ERR_OPEN;
  }
  else {
    // Mixer is open. Now define properties
    devmask	= 1;
    recmask	= 0;
    i_recsrc	= 0;
    stereodevs	= 0;
    MaxVolume	= 255;
    i_b_open	= true;

    i_s_mixer_name = "SUN Audio Mixer"
    return 0;
  }
}

int Mixer_SUN::releaseMixer()
{
  int l_i_ret = close(fd);
  return l_i_ret;
}

QString Mixer_SUN::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n(  "kmix: You have no permission to access the mixer device.\n" \
			  "Ask your system administrator to fix /dev/sndctl to allow the access.");
      break;
    default:
      l_s_errmsg = Mixer::errorText(mixer_error);
    }
  return l_s_errmsg;
}


int Mixer_SUN::readVolumeFromHW( int /*devnum*/, int *VolLeft, int *VolRight )
{
  audio_info_t audioinfo;
  int Volume;

  if (ioctl(fd, AUDIO_GETINFO, &audioinfo) < 0) {
    return(Mixer::ERR_READ);
  }
  else {
    Volume = audioinfo.play.gain;
    *VolLeft  = *VolRight = (Volume & 0x7f);
    return 0;
  }
}

int Mixer_SUN::writeVolumeToHW( int devnum, int volLeft, int volRight )
{
  audio_info_t audioinfo;
  AUDIO_INITINFO(&audioinfo);
  audioinfo.play.gain = volLeft;	// -<- Only left volume (one channel on Sun)

  if (ioctl(fd, AUDIO_SETINFO, &audioinfo) < 0) {
    return(Mixer::ERR_WRITE);
  }
  else {
    return 0;
  }
}
