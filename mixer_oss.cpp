
Mixer* Mixer::getMixer(int devnum, int SetNum)
{
  Mixer *l_mixer;
  l_mixer = new Mixer_OSS( devnum, SetNum);
  l_mixer->init(devnum, SetNum);
  return l_mixer;
}


Mixer_OSS::Mixer_OSS() : Mixer() { }
Mixer_OSS::Mixer_OSS(int devnum, int SetNum) : Mixer(devnum, SetNum) { }

void Mixer_OSS::setDevNumName_I(int devnum)
{
  switch (devnum) {
  case 0:
  case 1:
    devname = "/dev/mixer";
    break;
  default:
    devname = "/dev/mixer";
    devname += ('0'+devnum-1);
    break;
  }
}

QString Mixer_OSS::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n("kmix: You have no permission to access the mixer device.\n" \
			"Login as root and do a 'chmod a+rw /dev/mixer*' to allow the access.");
      break;
    case ERR_OPEN:
      l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
			"Please check that the soundcard is installed and the\n" \
			"soundcard driver is loaded.\n" \
			"On Linux you might need to use 'insmod' to load the driver.\n" \
			"Use 'soundon' when using commercial OSS.");
      break;
    default:
      l_s_errmsg = Mixer::errorText(mixer_error);
    }
  return l_s_errmsg;
}
