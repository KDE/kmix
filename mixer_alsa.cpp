
Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_ALSA( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}



Mixer_ALSA::Mixer_ALSA() : Mixer() { };
Mixer_ALSA::Mixer_ALSA(int devnum, int SetNum) : Mixer(devnum, SetNum);

int Mixer_ALSA::release_I()
{
  ret = snd_mixer_close(devhandle);
  return ret;
}

void Mixer_ALSA::setDevNumName_I(int devnum)
{
  devname = "ALSA";
}
