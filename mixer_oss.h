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

#ifndef MIXER_OSS_H
#define MIXER_OSS_H

#include <qstring.h>

#include "mixer_backend.h"

class Mixer_OSS : public Mixer_Backend
{
public:
  Mixer_OSS(int device = -1);
  virtual ~Mixer_OSS();

  virtual QString errorText(int mixer_error);
  virtual int readVolumeFromHW( int devnum, Volume &vol );
  virtual int writeVolumeToHW( int devnum, Volume &vol );

  virtual QString getDriverName();

protected:
  virtual bool setRecsrcHW( int devnum, bool on = true );
  virtual bool isRecsrcHW( int devnum );

  virtual int open();
  virtual int close();

  virtual QString deviceName( int );
  virtual QString deviceNameDevfs( int );
  int     m_fd;
  QString m_deviceName;
};

#endif
