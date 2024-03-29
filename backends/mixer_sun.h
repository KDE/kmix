/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken <esken@kde.org>
 *                            2000 Brian Hanson <bhanson@hotmail.com>
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

#ifndef MIXER_SUN_H
#define MIXER_SUN_H

#include <QString>

#include "mixerbackend.h"

class Mixer_SUN : public MixerBackend
{
public:
  Mixer_SUN(Mixer *mixer, int devnum);
  virtual ~Mixer_SUN();

  virtual QString errorText(int mixer_error);
  virtual int readVolumeFromHW( const QString& id, shared_ptr<MixDevice> );
  virtual int writeVolumeToHW ( const QString& id, shared_ptr<MixDevice> );

  virtual QString getDriverName();

protected:
  virtual int open();
  virtual int close();

  void VolumeToGainBalance( Volume& volume, uint_t& gain, uchar_t& balance );
  void GainBalanceToVolume( uint_t& gain, uchar_t& balance, Volume& volume );

  int fd;

private:
  int id2num(const QString& id);
};

#endif 
