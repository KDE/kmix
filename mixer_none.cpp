// We need to include always fcntl.h for the open syscall
#include <fcntl.h>

Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}

