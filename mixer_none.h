#ifndef MIXER_NONE_H
#define MIXER_NONE_H

class Mixer_None : public Mixer
{
public:
  Mixer_None();
  Mixer_None(int devnum, int SetNum);
  virtual ~Mixer_None() {};

protected:
  virtual int openMixer();
  virtual int releaseMixer();
};

#endif
