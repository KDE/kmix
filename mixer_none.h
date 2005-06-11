#ifndef MIXER_NONE_H
#define MIXER_NONE_H

#include "mixer_backend.h"

class Mixer_None : public Mixer_Backend
{
public:
  Mixer_None(int devnum);
  virtual ~Mixer_None();

  virtual int readVolumeFromHW( int devnum, Volume& vol );
  virtual int writeVolumeToHW( int devnum, Volume& vol );
  virtual bool setRecsrcHW( int devnum, bool on);
  virtual bool isRecsrcHW( int devnum );

protected:
  virtual int open();
  virtual int close();
};

#endif
