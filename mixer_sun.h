#ifndef MIXER_SUN_H
#define MIXER_SUN_H


class Mixer_SUN : public Mixer
{
public:
  Mixer_SUN();
  Mixer_SUN(int devnum, int SetNum);
  virtual ~Mixer_SUN() {};

  virtual QString errorText(int mixer_error);
  virtual void readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );

protected:
  virtual void setDevNumName_I(int devnum);
  virtual int openMixer();
  virtual int releaseMixer();

  int fd;
};

#endif 
