#ifndef MIXER_OSS_H
#define MIXER_OSS_H

#include <qstring.h>

class Mixer_OSS : public Mixer
{
public:
  Mixer_OSS();
  Mixer_OSS(int devnum, int SetNum);
  virtual ~Mixer_OSS() {};

  virtual QString errorText(int mixer_error);
  virtual void readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );
  virtual void writeVolumeToHW( int devnum, int volLeft, int volRight );

protected:
  virtual void setDevNumName_I(int devnum);
  virtual int openMixer();
  virtual int releaseMixer();

  int fd;
};

#endif
