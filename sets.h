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


class MixSetEntry
{
  char		channel;
  int		volumeL;
  int		volumeR;
  bool		is_disabled;		// Is slider disabled by user?
  bool		is_muted;		// Is it muted by user?
};


class MixSet : public QList<MixSetEntry>
{
  Q_OBJECT

public:
  MixSet(int num, const QString &name);
  ~MixSet() {};
  bool readSet();
  bool writeSet();

  int		getNum() { return num; };
  QString&      getName() { return name; };

private:
  int		num;
  QString	name;
};

#endif // SETS_H
