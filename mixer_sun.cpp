#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/audioio.h>


Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_SUN( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}


Mixer_SUN::Mixer_SUN() : Mixer() { };
Mixer_SUN::Mixer_SUN(int devnum, int SetNum) : Mixer(devnum, SetNum);
void Mixer_SUN::setDevNumName_I(int devnum)
{
  devname = "/dev/audioctl";
}

QString Mixer_SUN::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n(  "kmix: You have no permission to access the mixer device.\n" \
			  "Ask your system administrator to fix /dev/sndctl to allow the access.");
      break;
    default:
      l_s_errmsg = Mixer::errorText(mixer_error);
    }
  return l_s_errmsg;
}


void Mixer_SUN::readVolumeFromHW( int /*devnum*/, int *VolLeft, int *VolRight )
{
  audio_info_t audioinfo;
  int Volume;

  if (ioctl(fd, AUDIO_GETINFO, &audioinfo) < 0)
    errormsg(Mixer::ERR_READ);
  Volume = audioinfo.play.gain;
  *VolLeft  = *VolRight = (Volume & 0x7f);
}
