/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 2008 Helio Chissini de Castro <helio@kde.org>
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

#ifndef MIXER_PULSE_H
#define MIXER_PULSE_H

#include <QString>

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <pulse/ext-stream-restore.h>

#include "mixer_backend.h"

class Mixer_PULSE : public Mixer_Backend
{
public:
  Mixer_PULSE(Mixer *mixer, int devnum);
  virtual ~Mixer_PULSE();

  virtual int readVolumeFromHW( const QString& id, MixDevice *md  );
  virtual int writeVolumeToHW ( const QString& id, MixDevice *md  );
  void setRecsrcHW              ( const QString& id, bool on );
  bool isRecsrcHW               ( const QString& id );

  virtual QString getDriverName();

protected:
  virtual int open();
  virtual int close();

  int fd;
};

#endif 
