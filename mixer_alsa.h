#ifndef MIXER_ALSA_H
#define MIXER_ALSA_H

class Mixer_ALSA : public Mixer
{
public:
  Mixer_ALSA();
  ~Mixer_ALSA();
  Mixer_ALSA(int devnum, int SetNum);

  virtual void setRecsrc(unsigned int newRecsrc);
  virtual int readVolumeFromHW( int devnum, int *VolLeft, int *VolRight );
  virtual int writeVolumeToHW( int devnum, int volLeft, int volRight );

protected:
  virtual void setDevNumName_I(int devnum);
  virtual int openMixer();
  virtual int releaseMixer();
  virtual MixDevice* createNewMixDevice(int num);

private:
  snd_mixer_t        *handle;
  snd_mixer_groups_t  groups;
  snd_mixer_gid_t    *gid;
};

#endif
