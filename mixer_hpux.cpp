
Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_HPUX( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}

Mixer_HPUX::Mixer_HPUX() : Mixer() { };
Mixer_HPUX::Mixer_HPUX(int devnum, int SetNum) : Mixer(devnum, SetNum);

int Mixer_HPUX::release_I()
{
  ACloseAudio(hpux_audio,0);
}


void Mixer_HPUX::setDevNumName_I(int devnum)
{
  devname = "HP-UX Mixer";
}
