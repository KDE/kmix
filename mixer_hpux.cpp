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
  l_mixer = new Mixer_HPUX( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}

Mixer_HPUX::Mixer_HPUX() : Mixer() { };
Mixer_HPUX::Mixer_HPUX(int devnum, int SetNum) : Mixer(devnum, SetNum);

int Mixer_HPUX::release_I()
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
