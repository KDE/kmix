// We need to include always fcntl.h for the open syscall
#include <fcntl.h>

// This static method must be implemented (as fallback)
Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_None( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}

