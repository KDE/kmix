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

// Linux stuff, by Christian Esken
#if defined(linux)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>
// FreeBSD section, according to Sebestyen Zoltan
#elif defined(__FreeBSD__)
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include "machine/soundcard.h"
// NetBSD section, according to  Lennart Augustsson <augustss@cs.chalmers.se>
#elif defined(__NetBSD__)
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include <soundcard.h>
// BSDI section, according to <tom@foo.toetag.com>
#elif defined()
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/soundcard.h> 
// UnixWare includes
#elif defined(_UNIXWARE)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>
#endif

#include "mixer_oss.h"

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_OSS( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}


Mixer_OSS::Mixer_OSS() : Mixer() { }
Mixer_OSS::Mixer_OSS(int devnum, int SetNum) : Mixer(devnum, SetNum) { }

int Mixer_OSS::openMixer()
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
    if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
      return Mixer::ERR_READ;
    if (ioctl(fd, SOUND_MIXER_READ_RECMASK, &recmask) == -1)
      return Mixer::ERR_READ;
    if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
      return Mixer::ERR_READ;
    if (ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stereodevs) == -1)
      return Mixer::ERR_READ;
    if (!devmask)
      return Mixer::ERR_NODEV;
    MaxVolume =100;

#ifndef __NetBSD__
    struct mixer_info l_mix_info;
    if (ioctl(fd, SOUND_MIXER_INFO, &l_mix_info) != -1) {
      i_s_mixer_name = l_mix_info.name;
    }
    else
#endif /* !__NetBSD__ */
      i_s_mixer_name = "OSS Audio Mixer";

    isOpen = true;
    return 0;
  }
}

int Mixer_OSS::releaseMixer()
{
  int l_i_ret = close(fd);
  return l_i_ret;
}


void Mixer_OSS::setDevNumName_I(int devnum)
{
  switch (devnum) {
  case 0:
  case 1:
    devname = "/dev/mixer";
    break;
  default:
    devname = "/dev/mixer";
    devname += ('0'+devnum-1);
    break;
  }
}

QString Mixer_OSS::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n("kmix: You have no permission to access the mixer device.\n" \
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


void Mixer_OSS::setRecsrc(unsigned int newRecsrc)
{
  // Change status of record source(s)
  if (ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &newRecsrc) == -1)
    errormsg (Mixer::ERR_WRITE);
  // Re-read status of record source(s). Just in case, OSS does not like
  // my settings. And with this line mix->recsrc gets its new value. :-)
  if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
    errormsg(Mixer::ERR_READ);

  // PORTING: Hint: Do not forget to set i_recsrc to the new valid
  //                record source mask.

  /* Traverse through the mixer devices and set the record source flags
   * This is especially necessary for mixer devices that sometimes do
   * not obey blindly (because of hardware limitations)
   */
  unsigned int recsrcwork = i_recsrc;
  MixDevice *MixPtr;
  for ( unsigned int l_i_mixDevice = 1; l_i_mixDevice <= size(); l_i_mixDevice++) {
    MixPtr = operator[](l_i_mixDevice);

    if (recsrcwork & (1 << (MixPtr->num()) ) ) {
      MixPtr->setRecsrc(true);
    }
    else {
      MixPtr->setRecsrc(false);
    }
  }
}



int Mixer_OSS::readVolumeFromHW( int devnum, int *VolLeft, int *VolRight )
{
  int Volume;
  if (ioctl(fd, MIXER_READ( devnum ), &Volume) == -1) {
    /* Oops, can't read mixer */
    return(Mixer::ERR_READ);
  }
  else {
    *VolLeft  = (Volume & 0x7f);
    *VolRight = ((Volume>>8) & 0x7f);
    return 0;
  }
}



int Mixer_OSS::writeVolumeToHW( int devnum, int volLeft, int volRight )
{
  int Volume = volLeft + ((volRight)<<8);

  if (ioctl(fd, MIXER_WRITE( devnum ), &Volume) == -1) {
    return Mixer::ERR_WRITE;
  }
  else {
    return 0;
  }
}
