
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

void Mixer_IRIX::readVolumeFromHW( int devnum, int *VolLeft, int *VolRight )
{
       long in_buf[4];
      switch( devnum() ) {
      case 0:       // Speaker Output
	in_buf[0] = AL_RIGHT_SPEAKER_GAIN;
	in_buf[2] = AL_LEFT_SPEAKER_GAIN;
	break;
      case 7:       // Microphone Input (actually selectable).
	in_buf[0] = AL_RIGHT_INPUT_ATTEN;
	in_buf[2] = AL_LEFT_INPUT_ATTEN;
	break;
      case 11:      // Record monitor
	in_buf[0] = AL_RIGHT_MONITOR_ATTEN;
	in_buf[2] = AL_LEFT_MONITOR_ATTEN;
	break;
      default:
	printf("Unknown device %d\n", MixPtr->num() );
      }
      ALgetparams(AL_DEFAULT_DEVICE, in_buf, 4);
      *VolRight = in_buf[1]*100/255;
      *VolLeft  = in_buf[3]*100/255; 
}
