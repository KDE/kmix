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

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_IRIX( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}





Mixer_IRIX::Mixer_IRIX() : Mixer() { };
Mixer_IRIX::Mixer_IRIX(int devnum, int SetNum) : Mixer(devnum, SetNum);

int Mixer_IRIX::release_I()
{
    ALfreeconfig(m_config);
    ALcloseport(m_port);
return 0;
}

void Mixer_IRIX::readVolumeFromHW( int devnum, int *VolLeft, int *VolRight )
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
}
