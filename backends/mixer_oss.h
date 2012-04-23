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

#include <QString>

#include "mixer_backend.h"

class Mixer_OSS : public Mixer_Backend
{
public:
  Mixer_OSS(Mixer *mixer, int device);
  virtual ~Mixer_OSS();

  virtual QString errorText(int mixer_error);
  virtual int readVolumeFromHW( const QString& id, shared_ptr<MixDevice> );
  virtual int writeVolumeToHW ( const QString& id, shared_ptr<MixDevice> );

  virtual QString getDriverName();

protected:

  virtual int open();
  virtual int close();

  virtual QString deviceName( int );
  virtual QString deviceNameDevfs( int );

private:
  int     m_fd;
  QString m_deviceName;

  int setRecsrcToOSS( const QString& id, bool on );

};

#endif
