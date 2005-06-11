#ifndef MIXER_HPUX_H
#define MIXER_HPUX_H

#define DEFAULT_MIXER "HP-UX Mixer"
#ifdef HAVE_ALIB_H
#include <Alib.h>
#define HPUX_MIXER
#endif

#include "mixer_backend.h"

class Mixer_HPUX : public Mixer_Backend
{
public:
  Mixer_HPUX(int devnum);
  virtual ~Mixer_HPUX();

  virtual QString errorText(int mixer_error);

  virtual int readVolumeFromHW( int devnum, Volume &vol );
  virtual int writeVolumeToHW( int devnum, Volume &vol );

protected:
  virtual bool setRecsrcHW( int devnum, bool on = true );
  virtual bool isRecsrcHW( int devnum );

  virtual int open();
  virtual int close();

  Audio	  *audio;
  unsigned int stereodevs,devmask, recmask, MaxVolume, i_recsrc;
    

};

#endif
