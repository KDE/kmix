//-*- C++ -*-
#ifndef SETS_H
#define SETS_H
/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-98 Christian Esken
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

#include <qobject.h>
#include <qlist.h>
#include <qstring.h>


/**
  The MixSetEntry class represents the state of one mixer device.
  It contains device states as well as GUI aspects of the device */
class MixSetEntry
{
public:
  MixSetEntry();
  ~MixSetEntry();
  void read(int set,int devnum);
  void write(int set,int devnum);

  /// internal device number
  char		devnum;
  /// saved volume of left channel
  int		volumeL;
  /// saved volume of right channel
  int		volumeR;
  /// Is the device to be shown?
  bool		is_disabled;		// Is slider disabled by user?
  /// Is the device muted?
  bool		is_muted;		// Is it muted by user?
  /// Are the sliders linked?
  bool		StereoLink;
  /// Device Name. When using STL, this will be a String, that is
  /// shared between all the same channels in the list of MixSets
  QString       name;
};




class MixSet : public QList<MixSetEntry>
{

public:
  MixSet();
  ~MixSet();
  void read(int set);
  void write(int set);
  QString&      name() { return SetName; };
private:
  QString	SetName;
};


class MixSetList : public QList<MixSet>
{

public:
  MixSetList();
  ~MixSetList();
  void read();
  void write();
  int NumSets;
};

#endif // SETS_H
