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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mixer_irix.h"

#error "The IRIX port is not maintained anymore, an no official part of KMix / KDE at this point of time! Please contact the current KMix maintainer if you would like to maintain the port."

Mixer_Backend* IRIX_getMixer(int devnum)
{
  Mixer_Backend *l_mixer;
  l_mixer = new Mixer_IRIX( devnum);
  l_mixer->init(devnum);
  return l_mixer;
}



Mixer_IRIX::Mixer_IRIX(int devnum) : Mixer_Backend(devnum)
{
  close();
}

int Mixer_IRIX::open()
{
  // Create config
  m_config = ALnewconfig();
  if (m_config == (ALconfig)0) {
    cerr << "OpenAudioDevice(): ALnewconfig() failed\n";
    return Mixer::ERR_OPEN;
  }
  // Open audio device
  m_port = ALopenport("XVDOPlayer", "w", m_config);
  if (m_port == (ALport)0) {
    return Mixer::ERR_OPEN;
  }
  else {
    // Mixer is open. Now define properties
    devmask	= 1+128+2048;
    recmask	= 128;
    i_recsrc    = 128;
    stereodevs	= 1+128+2048;
    MaxVolume	= 255;

    i_s_mixer_name = "HPUX Audio Mixer";

    isOpen	= true;
    return 0;
  }
}

int Mixer_IRIX::close()
{
  m_isOpen = false;
  ALfreeconfig(m_config);
  ALcloseport(m_port);
  m_mixDevices.clear();
  return 0;
}

int Mixer_IRIX::readVolumeFromHW( int devnumi, Volume &vol, Volume & )
{
  long in_buf[4];
  switch( devnum() ) {
  case 0:       // Speaker Output
    in_buf[0] = AL_RIGHT_SPEAKER_GAIN;
    in_buf[2] = AL_LEFT_SPEAKER_GAIN;
    break;
  case 7:       // Microphone Input (actually selectable).
    in_buf[0] = AL_RIGHT_INPUT_ATTEN;
    in_buf[2] = AL_LEFT_INPUT_ATTEN;
    break;
  case 11:      // Record monitor
    in_buf[0] = AL_RIGHT_MONITOR_ATTEN;
    in_buf[2] = AL_LEFT_MONITOR_ATTEN;
    break;
  default:
    printf("Unknown device %d\n", MixPtr->num() );
  }
  ALgetparams(AL_DEFAULT_DEVICE, in_buf, 4);
  *VolRight = in_buf[1]*100/255;
  *VolLeft  = in_buf[3]*100/255;

  return 0;
}

int Mixer_IRIX::writeVolumeToHW( int devnum, Volume &vol, Volume & )
{
  // Set volume (right&left)
  long out_buf[4] =
  {
    0, volRight,
    0, volLeft
  };
  switch( mixdevice->num() ) {
  case 0:      // Speaker
    out_buf[0] = AL_RIGHT_SPEAKER_GAIN;
    out_buf[2] = AL_LEFT_SPEAKER_GAIN;
    break;
  case 7:      // Microphone (Input)
    out_buf[0] = AL_RIGHT_INPUT_ATTEN;
    out_buf[2] = AL_LEFT_INPUT_ATTEN;
    break;
  case 11:     // Record monitor
    out_buf[0] = AL_RIGHT_MONITOR_ATTEN;
    out_buf[2] = AL_LEFT_MONITOR_ATTEN;
    break;
  }
  ALsetparams(AL_DEFAULT_DEVICE, out_buf, 4);

  return 0;
}

QString IRIX_getDriverName() {
        return "IRIX";
}

QString Mixer_IRIX::getDriverName()
{
        return "IRIX";
}

