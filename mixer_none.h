#ifndef MIXER_NONE_H
#define MIXER_NONE_H

class Mixer_None : public Mixer
{
public:
  Mixer_None();
  Mixer_None(int devnum, int SetNum);
  virtual ~Mixer_None() {};

  virtual int readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );
  virtual int writeVolumeToHW( int devnum, int volLeft, int volRight );

protected:
  virtual int openMixer();
  virtual int releaseMixer();
};

#endif
