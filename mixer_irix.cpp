
Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_IRIX( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}





Mixer_IRIX::Mixer_IRIX() : Mixer() { };
Mixer_IRIX::Mixer_IRIX(int devnum, int SetNum) : Mixer(devnum, SetNum);

int Mixer_IRIX::release_I()
{
    ALfreeconfig(m_config);
    ALcloseport(m_port);
return 0;
}
