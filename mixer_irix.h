#ifndef MIXER_IRIX_H
#define MIXER_IRIX_H

#define _LANGUAGE_C_PLUS_PLUS
#include <dmedia/audio.h>

class Mixer_IRIX : public Mixer
{
public:
  Mixer_IRIX();
  Mixer_IRIX(int devnum, int SetNum);
  virtual ~Mixer_IRIX() {};

  virtual void readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );

protected:
  virtual void setDevNumName_I(int devnum);
  virtual int openMixer();
  virtual int releaseMixer();

  ALport	m_port;
  ALconfig	m_config;
};

#endif
