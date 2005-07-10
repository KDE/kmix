//-*-C++-*-

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
