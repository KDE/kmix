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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "mixer_none.h"

// This static method must be implemented (as fallback)
Mixer_Backend* Mixer::getMixer(int devnum)
{
  Mixer_Backend *l_mixer;
  l_mixer = new Mixer_None( devnum);
  return l_mixer;
}

Mixer_None::Mixer_None(int devnum) : Mixer_Backend( device )
{
}

Mixer_None::~Mixer_None()
{
  close();
}

int Mixer_None::open()
{
   //i_s_mixer_name = "No mixer";
   return Mixer::ERR_NOTSUPP;
}

int Mixer_None::close()
{
  m_isOpen = false;
  m_mixDevices.clear();
  return Mixer::ERR_NOTSUPP;
}

int Mixer_None::readVolumeFromHW( int , Volume &vol )
{
  return Mixer::ERR_NOTSUPP;
}

int Mixer_None::writeVolumeToHW( int , Volume &vol )
{
  return Mixer::ERR_NOTSUPP;
}

bool Mixer_None::setRecsrcHW( int devnum, bool on)
{
    return false;
}

bool Mixer_None::isRecsrcHW( int devnum )
{
    return false;
}

QString NONE_getDriverName() {
        return "None";
}

