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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef MIXER_SUN_H
#define MIXER_SUN_H

#include <qstring.h>

class Mixer_SUN : public Mixer
{
public:
  Mixer_SUN();
  Mixer_SUN(int devnum, int SetNum);
  virtual ~Mixer_SUN() {};

  virtual QString errorText(int mixer_error);
  virtual int readVolumeFromHW( int devnum, Volume& volume );
  virtual int writeVolumeToHW( int devnum, Volume volume );
  bool setRecsrcHW( int devnum, bool on );
  bool isRecsrcHW( int devnum );

protected:
  virtual int openMixer();
  virtual int releaseMixer();

  void VolumeToGainBalance( Volume& volume, uint_t& gain, uchar_t& balance );
  void GainBalanceToVolume( uint_t& gain, uchar_t& balance, Volume& volume );

  int fd;
};

#endif 
