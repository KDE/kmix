//-*-C++-*-

#ifndef MIXER_OSS_H
#define MIXER_OSS_H

#include <qstring.h>

class Mixer_OSS : public Mixer
{
public:
  Mixer_OSS(int device = -1, int card = -1 );
  virtual ~Mixer_OSS() {};

  virtual QString errorText(int mixer_error);
  virtual int readVolumeFromHW( int devnum, Volume &vol );
  virtual int writeVolumeToHW( int devnum, Volume vol );


protected:
  virtual bool setRecsrcHW( int devnum, bool on = true );
  virtual bool isRecsrcHW( int devnum );

  virtual int openMixer();
  virtual int releaseMixer();

  virtual QString deviceName( int );
  int     m_fd;
  QString m_deviceName;
};

#endif
