//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
 * Copyright (C) 1999 by Helge Deller <deller@gmx.de>
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
#ifndef MIXER_HPUX_H
#define MIXER_HPUX_H

#define DEFAULT_MIXER "HP-UX Mixer"
#ifdef HAVE_ALIB_H
#include <Alib.h>
#define HPUX_MIXER
#endif

#include "mixer_backend.h"

class Mixer_HPUX : public Mixer_Backend
{
public:
  Mixer_HPUX(int devnum=0);
  virtual ~Mixer_HPUX();

  virtual QString errorText(int mixer_error);

  virtual int readVolumeFromHW( int devnum, Volume &vol, Volume& );
  virtual int writeVolumeToHW ( int devnum, Volume &vol, Volume& );

  virtual QString getDriverName();

protected:
  virtual bool setRecsrcHW( int devnum, bool on = true );
  virtual bool isRecsrcHW( int devnum );

  virtual int open();
  virtual int close();

  Audio	  *audio;
  unsigned int stereodevs,devmask, recmask, MaxVolume, i_recsrc;
    

};

#endif
