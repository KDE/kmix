#ifndef MIXER_HPUX_H
#define MIXER_HPUX_H

#define DEFAULT_MIXER "HP-UX Mixer"
#ifdef HAVE_ALIB_H
#include <Alib.h>
#define HPUX_MIXER
#endif

class Mixer_HPUX : public Mixer
{
public:
  Mixer_HPUX();
  Mixer_HPUX(int devnum, int SetNum);
  virtual ~Mixer_HPUX();

  virtual QString errorText(int mixer_error);
  virtual void setRecsrc(unsigned int newRecsrc);
  virtual int readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );
  virtual int writeVolumeToHW( int devnum, int volLeft, int volRight );

protected:
  virtual int openMixer();
  virtual int releaseMixer();
  virtual void setDevNumName_I(int devnum);

  Audio	  *audio;
};

#endif
