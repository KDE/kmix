/*
 *              KMix -- KDE's full featured mini mixer
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MIXER_OSS4_H
#define MIXER_OSS4_H

#include "mixer_backend.h"
#include <sys/soundcard.h>

class Mixer_OSS4 : public Mixer_Backend
{
public:
  Mixer_OSS4(Mixer* mixer, int device);
  virtual ~Mixer_OSS4();

  virtual QString errorText(int mixer_error);
  virtual QString getDriverName();
  virtual bool CheckCapture(oss_mixext *ext);
  virtual bool hasChangedControls();
  virtual int readVolumeFromHW(const QString& id, shared_ptr<MixDevice> md);
  virtual int writeVolumeToHW(const QString& id, shared_ptr<MixDevice> md );
  virtual void setEnumIdHW(const QString& id, unsigned int idx);
  virtual unsigned int enumIdHW(const QString& id);

protected:

  MixDevice::ChannelType classifyAndRename(QString &name, int flags);

  int wrapIoctl(int ioctlRet);

  void reinitialize() { open(); close(); }
  virtual int open();
  virtual int close();

  int	  m_ossVersion;
  int     m_fd;
  int	  m_numMixers;
  int	  m_numExtensions;
  int	  m_modifyCounter;
  QString m_deviceName;

private:
  int id2num(const QString& id);
};
#endif
