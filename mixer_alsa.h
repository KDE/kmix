//-*-C++-*-

#ifndef MIXER_ALSA_H
#define MIXER_ALSA_H

class Mixer_ALSA : public Mixer
{
public:
  Mixer_ALSA( int device = -1, int card = -1 );
  ~Mixer_ALSA();

  virtual int  readVolumeFromHW( int devnum, Volume &vol );
  virtual int  writeVolumeToHW( int devnum, Volume vol );

  virtual bool setRecsrcHW( int devnum, bool on);
  virtual bool isRecsrcHW( int devnum );

protected:
  virtual int openMixer();
  virtual int releaseMixer();

private:
  int         identify( int, const char* id );
  int         numChannels( int mask );
  snd_mixer_t        *handle;
  snd_mixer_groups_t  groups;
  snd_mixer_gid_t    *gid;
};

#endif
