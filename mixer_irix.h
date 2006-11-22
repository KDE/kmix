//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef MIXER_IRIX_H
#define MIXER_IRIX_H

#define _LANGUAGE_C_PLUS_PLUS
#include <dmedia/audio.h>

#include "mixer_backend.h"

class Mixer_IRIX : public Mixer_Backend
{
public:
  Mixer_IRIX(int devnum=0);
  virtual ~Mixer_IRIX();

  virtual void setRecsrc(unsigned int newRecsrc);
  virtual int readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );
  virtual int writeVolumeToHW( int devnum, int volLeft, int volRight );

  virtual QString getDriverName();

protected:
  virtual void setDevNumName_I(int devnum);
  virtual int open();
  virtual int close();

  ALport	m_port;
  ALconfig	m_config;
};

#endif
