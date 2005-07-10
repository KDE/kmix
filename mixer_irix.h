#ifndef MIXER_IRIX_H
#define MIXER_IRIX_H

#define _LANGUAGE_C_PLUS_PLUS
#include <dmedia/audio.h>

#include "mixer_backend.h"

class Mixer_IRIX : public Mixer_Backend
{
public:
  Mixer_IRIX(int devnum);
  virtual ~Mixer_IRIX();

  virtual void setRecsrc(unsigned int newRecsrc);
  virtual int readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );
  virtual int writeVolumeToHW( int devnum, int volLeft, int volRight );

protected:
  virtual void setDevNumName_I(int devnum);
  virtual int open();
  virtual int close();

  ALport	m_port;
  ALconfig	m_config;
};

#endif
