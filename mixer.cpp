/*
 * Copyright by Christian Esken 1996-97
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS `AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
                                                                            
#include "iostream.h"
#include <unistd.h>
#include <string.h>

#include "mixer.h"
#include "mixer.moc"


#if defined(sun) || defined(__sun__)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/audioio.h>
//#include <stropts.h>
                               
#define SUN_MIXER
#endif

#ifdef sgi
#include <sys/fcntl.h>
#define IRIX_MIXER
#endif

#ifdef linux
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include "sys/soundcard.h"  // Here UNIX_SOUND_SYSTEM gets defined
#define OSS_MIXER
#endif

// FreeBSD section, according to Sebestyen Zoltan
#ifdef __FreeBSD__
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include "machine/soundcard.h"  // Here UNIX_SOUND_SYSTEM gets defined
#define OSS_MIXER
#endif

// PORTING: add #ifdef PLATFORM , commands , #endif, add your new mixer below

#if defined(SUN_MIXER) || defined(IRIX_MIXER)  || defined(OSS_MIXER)
// We are happy. Do nothing
#else

// We cannot handle this!
#define NO_MIXER
#endif



char KMixErrors[6][100]=
{
  "kmix: This message should not appear. :-(",
  "kmix: There is no mixer support for your system.",
  "kmix: Could not read from mixer.",
  "kmix: Could not write to mixer.",
  "kmix: Your mixer does not control any devices.",
  "kmix: Mixer does not support your platform. See mixer.cpp for porting hints (PORTING)."
};

  char* MixerDevNames[17]={"Volume" , "Bass"  , "Treble"    , "Synth", "Pcm"  , \
			   "Speaker", "Line"  , "Microphone", "CD"   , "Mix"  , \
			   "Pcm2"   , "RecMon", "IGain"     , "OGain", "Line1", \
			   "Line2"  , "Line3" };

Mixer::Mixer()
{
  setupMixer(DEFAULT_MIXER);
}

Mixer::Mixer(char *devname)
{
  setupMixer(devname);
}

int Mixer::grab()
{
  int ret=0;

  if (!isOpen)
    // Try to open Mixer, if it is not open already.
    ret=setupMixer(this->devname);

  return ret;
}

int Mixer::release()
{
  if(isOpen)
    {
      isOpen=false;
#ifdef IRIX_MIXER
      ALfreeconfig(m_config);
      ALcloseport(m_port);
#else
      close(fd);
#endif
    }

  return 0;
}

int Mixer::setupMixer(char *devname)
{

  bool ReadFromSet=false;  // !!! Sets not implemented yet
  int  SetNumber=0;

  // !!! Please check other elements, too, and if necessary, set them to a
  // !!! sane value
  isOpen = false;
  release();	// To be sure, release mixer before (re-)opening

  devmask = recmask = recsrc = stereodevs = 0;
  PercentLeft = PercentRight=100;
  num_mixdevs = 0;
  First=NULL;

  this->devname = strdup(devname);
  int ret = openMixer();
  if (ret)
      return ret;

  /*
   * Set up the structures for the internal representation of the mixer devs.
   */
  setupStructs();
  /*
   * Read the volumes from the mixer, after that read them  from a given
   * mixing set. Why do I not directly read from the mixing set, if one
   * is specified on the command line?
   * Answer: I want all available (in terms of "realized by OSS") devices
   *  to have some reasonable value in my internal structures, even if some
   *  devices are not specified in the mixing set. BTW: Usually they are
   *  specified, but if the "Load/Save options" does not work properly or
   *  if you pick up a configuration file from a friend with different mixing
   *  hardware, the result could be some serious mess. 
   */
  InternalSetVolumes(-1);
  if ( ReadFromSet == TRUE )
    InternalSetVolumes(SetNumber);

  return 0;
}


/******************************************************************************
 *
 * Function:	setupStructs
 *
 * Task:	Setup the internal data structures associated with a mixer
 *		channel. One structure of type "dt_mixer_T" is filled with
 *		information, two structures of type "dt_mixerCB_T" are
 *		filled for each mixing channel for use in the callbacks.
 *		The two structures are only used so that the callbacks can
 *		distinguish, which slider has been moved.
 *		This function is only called *once* during initialization.
 *		It only fills the structures. It does NOT decide, if all
 *		sliders appear (perhaps the user has disabled some) or if
 *		1 or 2 sliders for each stereo device are shown. The policy
 *		for this is implemented in "InternalSetVolumes", and in
 *		some user callbacks (the latter only in the future).
 *
 * in:		-
 * 
 * out:		-
 *
 *****************************************************************************/
void  Mixer::setupStructs(void)
{
  int         devicework, recwork, recsrcwork, stereowork;
  MixDevice   *MixPtr, *MixNext;

  // Copy the bit mask to scratch registers
  devicework  = devmask;
  recwork     = recmask;
  recsrcwork  = recsrc;
  stereowork  = stereodevs;
  num_mixdevs = 0;


  // Clear old List of devices
  MixPtr = First;
  while ( MixPtr != NULL ) {
    MixNext = MixPtr->Next;
    delete MixPtr;
    MixPtr = MixNext;
  }
    
  MixPtr= NULL;
  num_mixdevs= 0;

  for (int i=0 ; i<=MAX_MIXDEVS; i++) {
    if ( (devicework & 1) == 1 ) {
      // If we come here, the mixer device with num i is supported by
      // sound driver.

      /* !!! Here I can fill in a test, if the mixing device should
       *     be shown. You can create a super small mixer if you only
       *     enable one channel. This is most useful in conjunction with
       *     using different mixing setups. TODO
       */


      // Store old Adress, for setting up its "Next" pointer
      MixDevice *MixOld = MixPtr;

      // Set up a pointer to the general mixing structure
      MixPtr = new MixDevice(i);
      CHECK_PTR(MixPtr);
      // set up old "Next" pointer
      if (MixOld == NULL)
	First = MixPtr;
      else
	MixOld->Next = MixPtr;
      MixPtr->Next = NULL;

      MixPtr->mix  = this;	// keep track, which mixer is used by device i
      MixPtr->Left  = new MixChannel;
      MixPtr->Right = new MixChannel;
      MixPtr->Left->mixDev = MixPtr->Right->mixDev = MixPtr;
      MixPtr->Left->channel  = Mixer::LEFT;
      MixPtr->Right->channel = Mixer::RIGHT;


      // Fill out the device structure for the current mixing device.
      // 1. Remember the device number (for use in the callback).
      MixPtr->device_num    = i;
      MixPtr->is_stereo     = (stereowork & 1);
      MixPtr->is_recsrc     = (recsrcwork & 1);
      MixPtr->is_recordable = (recwork    & 1);
      MixPtr->is_disabled   = false; // !!! sets
      MixPtr->is_muted      = false; // !!! sets
      num_mixdevs++;
    }
    devicework = (devicework>>1);
    recwork    = (recwork   >>1);
    recsrcwork = (recsrcwork>>1);
    stereowork = (stereowork>>1);
  }
}

/******************************************************************************
 *
 * Function:	InternalSetVolumes
 *
 * Task:	Reads volume settings from either the mixer or a mixing set
 *		and fills in the values in the according "dt_mixer_T"
 *		structure.
 *		This function decides on policy, if a slider is created
 *		for a mixing channel, if the "StereoLink" is enabled,
 *		and thus if two sliders will be created. The idea is as
 *		follows: Reading from a set reads in the "StereoLink"
 *		element. Reading from the mixer sets "StereoLink", if
 *		left and right channel use the same volume.
 *		Attention! This function decidedes on policy, but the
 *		sliders are created elsewhere.
 *
 *
 * in:		The source, where the volume settings are read from.
 *		-1  = Read from mixer hardware
 *		 0  = Default mixing set
 *		 1-5= Other mixing sets
 * 
 * out:		-
 *
 *****************************************************************************/
void Mixer::InternalSetVolumes(int Source)
{
  MixDevice	*MixPtr;
  int		VolLeft,VolRight,Volume;

  /*
   * Source can be the number of a mixing set, or -1 for reading from
   * the hardware.
   * -1  = Read from mixer hardware
   *  0  = Default mixing set
   *  1-5= Other mixing sets
   */

  if ( Source < 0 )
    {
      // Read from hardware
      MixPtr = First;
      while (MixPtr)
	{
#ifdef SUN_MIXER
	  audio_info_t audioinfo;

	  if (ioctl(fd, AUDIO_GETINFO, &audioinfo) < 0)
	    errormsg(Mixer::ERR_READ);
	  Volume = audioinfo.play.gain;
	  VolLeft  = VolRight = Volume & 0x7f;
#endif
#ifdef IRIX_MIXER
	  // Get volume
	  long in_buf[4];
	  switch( MixPtr->device_num ) {
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
	    printf("Unknown device %d\n", MixPtr->device_num);
	  }
	  ALgetparams(AL_DEFAULT_DEVICE, in_buf, 4);
	  VolRight = in_buf[1]*100/255;
	  VolLeft  = in_buf[3]*100/255;
#endif
#ifdef OSS_MIXER
	  if (ioctl(fd, MIXER_READ( MixPtr->device_num ), &Volume) == -1)
	    /* Oops, can't read mixer */
	    errormsg(Mixer::ERR_READ);
	  
	  VolLeft  = Volume & 0x7f;
	  VolRight = (Volume>>8) & 0x7f;
// PORTING: add #ifdef PLATFORM , commands , #endif
#endif

	  if ( (VolLeft == VolRight) && (MixPtr->is_stereo == true) )
	    MixPtr->StereoLink = true;
	  else
	    MixPtr->StereoLink = false;

	  MixPtr->Left->volume = VolLeft;
	  if ( MixPtr->StereoLink )
	    MixPtr->Right->volume = MixPtr->Left->volume;
	  else
	    MixPtr->Right->volume = VolRight;

	  MixPtr=MixPtr->Next;
	}
      //StereoSliderAvailable = true;
    } /* if ( Source < 0 ) */

}


void Mixer::setBalance(int left, int right)
{
  PercentLeft  = left;
  PercentRight = right;

  /* The change affects all devices, so do now update all devices */
  updateMixDevice(NULL);
}

/* Open mixer device */
int Mixer::openMixer(void)
{
#ifdef NO_MIXER
  return Mixer::ERR_NOTSUPP;
#endif

  release();		// To be sure, release mixer before (re-)opening

#ifdef IRIX_MIXER
  // Create config
  m_config = ALnewconfig();
  if (m_config == (ALconfig)0)
    {
      cerr << "OpenAudioDevice(): ALnewconfig() failed\n";
      return Mixer::ERR_OPEN;
    }
  // Open audio device
  m_port = ALopenport("XVDOPlayer", "w", m_config);
  if (m_port == (ALport)0)
    return Mixer::ERR_OPEN;


#else
  if ((fd= open(devname, O_RDWR)) < 0)
    return Mixer::ERR_OPEN;
#endif

#ifdef SUN_MIXER

  devmask   =1;
  recmask   =0;
  recsrc    =0;
  stereodevs=0;
  MaxVolume =255;
#endif
#ifdef IRIX_MIXER

  devmask   =1+128+2048;
  recmask   =128;
  recsrc    =128;
  stereodevs=1+128+2048;
  MaxVolume =255; 
#endif
#ifdef OSS_MIXER
  if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
    return Mixer::ERR_READ;
  if (ioctl(fd, SOUND_MIXER_READ_RECMASK, &recmask) == -1)
    return Mixer::ERR_READ;
  if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1)
    return Mixer::ERR_READ;
  if (ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stereodevs) == -1)
    return Mixer::ERR_READ;
  if (!devmask)
    return Mixer::ERR_NODEV;
  MaxVolume =100;
#endif

// PORTING: add #ifdef PLATFORM , commands , #endif

  isOpen = true;
  return 0;
} 


void Mixer::errormsg(int mixer_error)
{
  if (mixer_error < Mixer::ERR_LASTERR )
    cerr << KMixErrors[mixer_error] << '\n';
  else
    cerr << "Mixer: Unknown error.\n";
}


void Mixer::updateMixDevice(MixDevice *mixdevice)
{
  if (mixdevice == NULL)
    {
      /* Argument NULL => Update all devices */
      for (MixDevice *i = First; i!=NULL ; i=i->Next)
	  updateMixDeviceI(i);
    }
  else
    updateMixDeviceI(mixdevice);
}

/* Internal function */
void Mixer::updateMixDeviceI(MixDevice *mixdevice)
{
  int Volume,volLeft,volRight;


  if ( mixdevice->is_muted )
    {
      // The mute is on! Thats really easy then
      volLeft = volRight = 0;
    }
  else
    {
      // Not muted. Hmm, lets ask left and right channel
      volLeft  = mixdevice->Left->volume;
      volRight = mixdevice->Right->volume;

      /* Now multiply the value with the percentage given by the stereo
       * slider. Multiply with the maximum allowed value (100 with OSS,
       * 255 with SGI) Divide everything by 100.
       * This give a value in the interval [0..100] or [0..255].
       */
      volLeft  = (volLeft *PercentLeft *MaxVolume) / 10000;
      volRight = (volRight*PercentRight*MaxVolume) / 10000;
    }


  Volume = volLeft + ((volRight)<<8);

#ifdef SUN_MIXER
  audio_info_t audioinfo;
  AUDIO_INITINFO(&audioinfo);
  audioinfo.play.gain = volLeft;	// -<- Only left volume (one channel on Sun)

  if (ioctl(fd, AUDIO_SETINFO, &audioinfo) < 0)
    errormsg(Mixer::ERR_WRITE);
#endif
#ifdef IRIX_MIXER
  // Set volume (right&left)
  long out_buf[4] =
  {
    0, volRight,
    0, volLeft
  };
  switch( mixdevice->device_num ) {
  case 0:      // Speaker
    out_buf[0] = AL_RIGHT_SPEAKER_GAIN;
    out_buf[2] = AL_LEFT_SPEAKER_GAIN;
    break;
  case 7:      // Microphone (Input)
    out_buf[0] = AL_RIGHT_INPUT_ATTEN;
    out_buf[2] = AL_LEFT_INPUT_ATTEN;
    break;
  case 11:     // Record monitor
    out_buf[0] = AL_RIGHT_MONITOR_ATTEN;
    out_buf[2] = AL_LEFT_MONITOR_ATTEN;
    break;
  }
  ALsetparams(AL_DEFAULT_DEVICE, out_buf, 4);
#endif
#ifdef OSS_MIXER
  if (ioctl(fd, MIXER_WRITE( mixdevice->device_num ), &Volume) == -1)
    errormsg(Mixer::ERR_WRITE);
// PORTING: add #ifdef PLATFORM , commands , #endif
#endif
}



void MixChannel::VolChanged(int new_pos)
{
  MixDevice *mixdevice=mixDev;

  volume = 100-new_pos; // The slider is reversed
  /* Set right channel volume to the same amount as left channel, if right
   * is linked to left
   */
  if (mixdevice->StereoLink == TRUE)
    mixdevice->Right->volume = volume;

  mixdevice->mix->updateMixDevice(mixdevice);
}


void MixDevice::MvolSplitCB()
{
  StereoLink = !StereoLink;
  Right->volume = Left->volume = (Right->volume + Left->volume)/2;

  /* This really should be a member of Mixer and not MixDevice. But I
   * do not want every darn Class to be a derivation of QWidget. So I
   * do a bit magic in here.
   */
  mix->updateMixDevice(NULL);

  emit relayout();
}


void MixDevice::MvolMuteCB()
{
  is_muted = ! is_muted;
  // Again, this glorious line :-(
  mix->updateMixDevice(NULL);

  emit relayout();
}

void MixDevice::MvolRecsrcCB()
{
  // Remember old state of recsrc, so we see in the end if anything has
  // changed at all.
  unsigned int old_recsrc = mix->getRecsrc();
  // Determine the Bit in the record source mask, which must be changed
  unsigned int bit_in_recsrc =  ( 1 << device_num );
  // And calculate the new wanted record source mask.
  unsigned int new_recsrc = old_recsrc ^ bit_in_recsrc;

  mix->setRecsrc(new_recsrc);

  // If something has changed, tell the UI to da a relayout.
  // OK! In future, I will give relayout hints, so the UI will only
  // relayout certain parts, e.g. the Record source "bullets". TODO !!!
  if (mix->getRecsrc() != old_recsrc)
    emit relayout();
}

unsigned int Mixer::getRecsrc()
{
  return recsrc;
}

void Mixer::setRecsrc(unsigned int newRecsrc)
{
#ifdef OSS_MIXER
  // Change status of record source(s)
  if (ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &newRecsrc) == -1)
    errormsg (Mixer::ERR_WRITE);
  // Re-read status of record source(s). Just in case, OSS does not like
  // my settings. And with this line mix->recsrc gets its new value. :-)
  if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1)
    errormsg(Mixer::ERR_READ);
#else
  KMsgBox::message(0, "Porting required.", "Please port this feature :-)", KMsgBox::INFORMATION, "OK" );
  // PORTING: Hint: Do not forget to set recsrc to the new valid
  //                record source mask.
#endif

  /* Traverse through the mixer devices and set the "is_recsrc" flags
   * This is especially necessary for mixer devices that sometimes do
   * not obey blindly (because of hardware limitations)
   */
  MixDevice *MixDev = First;
  unsigned int recsrcwork = recsrc;
  while (MixDev) {
    if (recsrcwork & (1 << (MixDev->device_num) ) )
      MixDev->is_recsrc = true;
    else
      MixDev->is_recsrc = false;

    MixDev            = MixDev->Next;
  }
}


MixDevice::MixDevice(int num)
{
  device_num=num;
  strncpy(devname, MixerDevNames[num],11);
  devname[10]=0;
};

char* MixDevice::name()
{
  return devname;
}
