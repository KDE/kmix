#ifndef MIXER_NONE_H
#define MIXER_NONE_H

class Mixer_None : public Mixer
{
public:
  Mixer_None();
  Mixer_None(int devnum, int SetNum);
  virtual ~Mixer_None();

  virtual int readVolumeFromHW( int devnum, Volume& vol );
  virtual int writeVolumeToHW( int devnum, Volume vol );
  virtual bool setRecsrcHW( int devnum, bool on);
  virtual bool isRecsrcHW( int devnum );

protected:
  virtual int openMixer();
  virtual int releaseMixer();
};

#endif
