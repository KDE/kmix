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

#include "mixer_hpux.h"

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_HPUX( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}

Mixer_HPUX::Mixer_HPUX() : Mixer() { };
Mixer_HPUX::Mixer_HPUX(int devnum, int SetNum) : Mixer(devnum, SetNum);

int Mixer_HPUX::openMixer()
{
  release();		// To be sure, release mixer before (re-)opening

  char ServerName[50];
  ServerName[0] = 0;
  hpux_audio = AOpenAudio(ServerName,NULL);
  if (hpux_audio==0) {
    return Mixer::ERR_OPEN;
  }
  else {
     // Mixer is open. Now define properties
    devmask = 1 + 16; // volume+pcm
    if (AOutputDestinations(hpux_audio) & AMonoIntSpeakerMask) 	devmask |= 32; // Speaker
    if (AOutputDestinations(hpux_audio) & AMonoLineOutMask)	devmask |= 64; // Line
    if (AInputSources(hpux_audio) & AMonoMicrophoneMask)		devmask |= 128;// Microphone
    if (AOutputDestinations(hpux_audio) & AMonoJackMask)		devmask |= (1<<14); // Line1
    if (AOutputDestinations(hpux_audio) & AMonoHeadphoneMask)	devmask |= (1<<15); // Line2
    if (AInputSources(hpux_audio) & AMonoAuxiliaryMask)		devmask |= (1<<20); // PhoneIn
    MaxVolume = aMaxOutputGain(hpux_audio) - aMinOutputGain(hpux_audio);
    recmask = stereodevs = devmask; 
    i_recsrc = 0;   
    isOpen = true;

    i_s_mixer_name = "HPUX Audio Mixer"
    return 0;
  }
}

int Mixer_HPUX::releaseMixer()
{
  ACloseAudio(hpux_audio,0);
}


void Mixer_HPUX::setDevNumName_I(int devnum)
{
  devname = "HP-UX Mixer";
}


void Mixer_::readVolumeFromHW( int /*devnum*/, int *VolLeft, int *VolRight )
{
  *VolRight = 100;
  *VolLeft  = 100;
}
