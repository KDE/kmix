/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-98 Christian Esken
 *                        esken@kde.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
                                                                            
#include "iostream.h"
#include <unistd.h>
#include <string.h>

#include "sets.h"
#include "mixer.h"
#include "mixer.moc"

#include <qstring.h>

#if defined(sun) || defined(__sun__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/audioio.h>
                               
#define SUN_MIXER
#endif

#ifdef sgi
#include <sys/fcntl.h>
#define IRIX_MIXER
#endif

#ifdef linux
#ifdef ALSA
  #include <sys/soundlib.h>
  void *devhandle;
  int ret;
#else
 #include <fcntl.h>
 #include <sys/ioctl.h>
 #include <sys/types.h>
 #include <sys/soundcard.h>
 #define OSS_MIXER
#endif
#endif

// FreeBSD section, according to Sebestyen Zoltan
#ifdef __FreeBSD__
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include "machine/soundcard.h"
#define OSS_MIXER
#endif

// NetBSD section, according to  Lennart Augustsson <augustss@cs.chalmers.se>
#ifdef __NetBSD__
#include <fcntl.h>
#include "sys/ioctl.h"
#include <sys/types.h>
#include <soundcard.h>
#define OSS_MIXER
#endif

// BSDI section, according to <tom@foo.toetag.com>
#ifdef __bsdi__ 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/soundcard.h> 
#define OSS_MIXER 
#endif 


// PORTING: add #ifdef PLATFORM , commands , #endif, add your new mixer below

#if defined(SUN_MIXER) || defined(IRIX_MIXER)  || defined(OSS_MIXER) || defined(ALSA)
// We are happy. Do nothing
#else

// We need to include always fcntl.h for the open syscall
#include <fcntl.h>
// We cannot handle this!
#define NO_MIXER
#endif


// If you change the definition here, make sure to fix it in kmix.cpp, too.
char KMixErrors[6][200]=
{
  "kmix: This message should not appear. :-(",
#ifdef OSS_MIXER
  "kmix: Could not open mixer.\nPerhaps you have no permission to access the mixer device.\n" \
  "Login as root and do a 'chmod a+rw /dev/mixer*' to allow the access.",
#elif defined (SUN_MIXER)
  "kmix: Could not open mixer.\nPerhaps you have no permission to access the mixer device.\n" \
  "Ask your system administrator to fix /dev/sndctl to allow the access.",
#else
  "kmix: Could not open mixer.\nPerhaps you have no permission to access the mixer device.\n" \
  "Please check your operating systems manual to allow the access.",
#endif
  "kmix: Could not read from mixer.",
  "kmix: Could not write to mixer.",
  "kmix: Your mixer does not control any devices.",
  "kmix: Mixer does not support your platform. See mixer.cpp for porting hints (PORTING)."
};

char* MixerDevNames[17]={"Volume" , "Bass"  , "Treble"    , "Synth", "Pcm"  , \
			 "Speaker", "Line"  , "Microphone", "CD"   , "Mix"  , \
			 "Pcm2"   , "RecMon", "IGain"     , "OGain", "Line1", \
			 "Line2"  , "Line3" };

/// The mixing set list

Mixer::Mixer()
{
  // use default mixer device
  setupMixer(0);
}

Mixer::Mixer(int devnum)
{
  setupMixer(devnum);
}

void Mixer::sessionSave()
{
  TheMixSets->write();
}



int Mixer::grab()
{
  int ret=0;

  if (!isOpen)
    // Try to open Mixer, if it is not open already.
    ret=setupMixer(this->devnum);

  return ret;
}

int Mixer::release()
{
  if(isOpen) {
    isOpen=false;
#ifdef IRIX_MIXER
    ALfreeconfig(m_config);
    ALcloseport(m_port);
#elif defined(ALSA)
    ret = snd_mixer_close(devhandle);
    return ret;
#else
    close(fd);
#endif
  }
  return 0;
}


int Mixer::setupMixer(int devnum)
{
  TheMixSets = new MixSetList;
  TheMixSets->read();  // Read sets from kmixrc

  isOpen = false;
  release();	// To be sure, release mixer before (re-)opening

  devmask = recmask = recsrc = stereodevs = 0;
  PercentLeft = PercentRight=100;
  num_mixdevs = 0;
  First=NULL;

  // set the mixer device name from a given device id
  setDevNumName(devnum);


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
  Set2Set0(-1,true);
  Set0toHW();

  return 0;
}


void Mixer::setDevNumName(int devnum)
{
  QString devname;
  this->devnum  = devnum;

#ifdef OSS_MIXER
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
#endif
#ifdef SUN_MIXER
  devname = "/dev/audioctl";
#endif
#ifdef ALSA
  devname = "ALSA";
#endif
#ifdef NO_MIXER
  devname = "Mixer";
#endif

  // Using a (const char*) cast. There seem to be some broken platforms out
  // there, so I had to add it.
  this->devname = strdup((const char *)devname);
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

  // Copy the bit mask to scratch registers. I will do bit shiftings on these.
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
      MixPtr->is_disabled   = false;
      MixPtr->is_muted      = false;
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
 *		This function defines the policy, if a slider is created
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
 *		-1    = Read from mixer hardware
 *		 0    = Default mixing set
 *		 1-...= Other mixing sets
 * 
 * out:		-
 *
 *****************************************************************************/



/*****************************************************************************
 * This functions writes mixing set 0 to the Hardware
 *****************************************************************************/
void Mixer::Set0toHW()
{
  MixSet *SrcSet = TheMixSets->first();
  MixDevice *MixPtr = First;

  while ( MixPtr) {
    // Find mix set entry for current MixPtr
    MixSetEntry *mse;
    for (mse = SrcSet->first();
	 (mse != NULL) && (mse->devnum != MixPtr->device_num);
	 mse=SrcSet->next() );

    if (mse == NULL)
      continue;  // entry not found

    MixPtr->StereoLink  = mse->StereoLink;
    MixPtr->is_muted    = mse->is_muted;
    MixPtr->is_disabled = mse->is_disabled;
    if (! MixPtr->is_disabled) {
      // Write volume (when channel is enabled)
      MixPtr->Left->volume  =  mse->volumeL;
      MixPtr->Right->volume =  mse->volumeR;
    }

    MixPtr=MixPtr->Next;
  }
}

/*****************************************************************************
 * This functions writes a mixing set with number "Source" into mixing set 0
 *****************************************************************************/
void Mixer::Set2Set0(int Source, bool copy_volume)
{
  MixDevice	*MixPtr;
  int		VolLeft,VolRight,Volume;

  MixSet *DestSet = TheMixSets->first();

  /*
   * Source can be the number of a mixing set, or -1 for reading from
   * the hardware.
   * -1    = Read from mixer hardware
   *  0    = Default mixing set (request gets ignored)
   *  1-...= Other mixing sets
   *
   * Destination set is always set 0
   */

  if ( Source < 0 ) {
    // Read from hardware
    MixPtr = First;
    while (MixPtr) {

      // Find mix set entry for current MixPtr
      MixSetEntry *mse = DestSet->findDev(MixPtr->device_num);
      if (mse == NULL)
	continue;  // entry not found


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
#ifdef ALSA
      snd_mixer_channel_t data;
      ret = snd_mixer_channel_read( devhandle, MixPtr->device_num, &data );
      if ( !ret ) {
	VolLeft = data.left;
	VolRight = data.right;
      }
      else 
 	errormsg(Mixer::ERR_READ);
#endif

      if ( (VolLeft == VolRight) && (MixPtr->is_stereo == true) )
	mse->StereoLink = true;
      else
	mse->StereoLink = false;

      mse->volumeL = VolLeft;
      if ( MixPtr->StereoLink )
	mse->volumeR = VolLeft;
      else
	mse->volumeR = VolRight;

      MixPtr=MixPtr->Next;
    }
  } /* if ( Source < 0 ) */
  else {
    // Read from a set (means copy set data to MixSet 0)
    MixSet *SrcSet = TheMixSets->first();
    for (int ssn=0; ssn<Source; ssn++) {
      SrcSet = TheMixSets->next();
      if (SrcSet == 0)
	SrcSet = TheMixSets->addSet(); // automatically create any "missing" sets
    }
    if (SrcSet == DestSet)
      return; // nothing to do (means: Set 0)

    MixSet::clone( SrcSet, DestSet, copy_volume );
  }
}

void Mixer::Set0toSet(int Source)
{
  MixSet *SrcSet  = TheMixSets->first();
  MixSet *DestSet = SrcSet;
  for (int ssn=0; ssn<Source; ssn++) {
    DestSet = TheMixSets->next();
    if (DestSet == 0)
      DestSet = TheMixSets->addSet(); // automatically create any "missing" sets
  }
  if (SrcSet == DestSet)
    return; // nothing to do (means: Set 0)

  MixSet::clone( SrcSet, DestSet, true );
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
#elif defined(ALSA)
  ret = snd_mixer_open( &devhandle, 0, 0 ); /* card 0 mixer 0 */
  if ( ret )
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
#ifdef ALSA
  snd_mixer_channel_info_t chinfo;
  snd_mixer_channel_t data;
  int num, i;
  devmask=recmask=recsrc=stereodevs=0;
  MaxVolume = 100;
  num = snd_mixer_channels( devhandle );
  if ( num < 0 )
    return Mixer::ERR_NODEV;
  for( i=0;i<=num; i++ ) {
    ret = snd_mixer_channel_info(devhandle, i, &chinfo);
    if ( !ret ) {
      if ( chinfo.caps & SND_MIXER_CINFO_CAP_STEREO )
        stereodevs |= 1 << i;
      if ( chinfo.caps & SND_MIXER_CINFO_CAP_RECORD )
        recmask |= 1 << i;
      devmask |= 1 << i;
      ret = snd_mixer_channel_read( devhandle, i, &data );
      if ( !ret ) {
        if ( data.flags & SND_MIXER_FLG_RECORD )
        recsrc |= 1 << i;
      }
    }
  }    
  if ( !devmask )
    return Mixer::ERR_NODEV;
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


  if ( mixdevice->is_muted ) {
    // The mute is on! Thats really easy then
    volLeft = volRight = 0;
  }
  else {
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
#ifdef ALSA
  snd_mixer_channel_t data;
  ret = snd_mixer_channel_read( devhandle, mixdevice->device_num, &data );
  if ( !ret ) {
    data.left = volLeft;
    data.right = volRight;
    data.channel = mixdevice->device_num;
    ret = snd_mixer_channel_write( devhandle, mixdevice->device_num, &data );
    if ( ret )
      errormsg(Mixer::ERR_WRITE);
  }
  else
    errormsg(Mixer::ERR_READ);
#endif
}



void MixChannel::VolChanged(int new_pos)
{
  MixDevice *mixdevice=mixDev;

  volume = 100-new_pos; // The panning slider is reversed

  /* Set right channel volume to the same amount as left channel, if right
   * is linked to left
   */
  if (mixdevice->StereoLink == TRUE)
    mixdevice->Right->volume = volume;

  MixSet *Set0  = mixDev->mix->TheMixSets->first();
  MixSetEntry *mse = Set0->findDev(mixdevice->device_num);
  if (mse) {
    mse->volumeL =  mixdevice->Left->volume;
    mse->volumeR =  mixdevice->Right->volume;
  }
  else {
    cerr << "MixChannel::VolChanged(): no such mix set entry\n";
  }
  mixdevice->mix->updateMixDevice(mixdevice);
}


void MixDevice::MvolSplitCB()
{
  StereoLink = !StereoLink;
  Right->volume = Left->volume = (Right->volume + Left->volume)/2;

  MixSet *Set0  = mix->TheMixSets->first();
  MixSetEntry *mse = Set0->findDev(device_num);
  if (mse) {
    mse->StereoLink =  StereoLink;
  }
  else {
    cerr << "MixChannel::MvolSplitCB(): no such mix set entry\n";
  }
  /* This really should better be a member of Mixer and not MixDevice.
   * But I do not want every darn Class to be a derivation of QWidget.
   * So I do a bit magic in here.
   */
  mix->updateMixDevice(NULL);

  emit relayout();
}


void MixDevice::MvolMuteCB()
{
  is_muted = ! is_muted;
  MixSet *Set0  = mix->TheMixSets->first();
  MixSetEntry *mse = Set0->findDev(device_num);
  if (mse) {
    mse->is_muted =  is_muted;
  }
  else {
    cerr << "MixChannel::MvolMuteCB(): no such mix set entry\n";
  }
  // Again, this glorious line ;-)
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
MixDevice *MixDev = First; /* moved up for use with ALSA */
#ifdef OSS_MIXER
  // Change status of record source(s)
  if (ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &newRecsrc) == -1)
    errormsg (Mixer::ERR_WRITE);
  // Re-read status of record source(s). Just in case, OSS does not like
  // my settings. And with this line mix->recsrc gets its new value. :-)
  if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1)
    errormsg(Mixer::ERR_READ);
#elif defined(ALSA)
  snd_mixer_channel_t data;
  recsrc = 0;
  while (MixDev) {
    ret = snd_mixer_channel_read( devhandle, MixDev->device_num, &data ); /* get */
    if ( ret )
      errormsg(Mixer::ERR_READ);
    if ( newRecsrc & ( 1 << MixDev->device_num ) )
      data.flags |= SND_MIXER_FLG_RECORD;
    else
      data.flags &= ~SND_MIXER_FLG_RECORD;
    ret = snd_mixer_channel_write( devhandle, MixDev->device_num, &data ); /* set */
    if ( ret )
      errormsg(Mixer::ERR_WRITE);
    ret = snd_mixer_channel_read( devhandle, MixDev->device_num, &data ); /* check */
    if ( ret )
      errormsg(Mixer::ERR_READ);
    if ( ( data.flags & SND_MIXER_FLG_RECORD ) && /* if it's set and stuck */
         ( newRecsrc & ( 1 << MixDev->device_num ) ) ) {
      recsrc |= 1 << MixDev->device_num;
      MixDev->is_recsrc = true;
    } else
      MixDev->is_recsrc = false;
    MixDev = MixDev->Next;
  }
  return; /* I'm done */ 
#else
  KMsgBox::message(0, "Porting required.", "Please port this feature :-)", KMsgBox::INFORMATION, "OK" );
  // PORTING: Hint: Do not forget to set recsrc to the new valid
  //                record source mask.
#endif

  /* Traverse through the mixer devices and set the "is_recsrc" flags
   * This is especially necessary for mixer devices that sometimes do
   * not obey blindly (because of hardware limitations)
   */
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
#ifdef ALSA
    snd_mixer_channel_info_t chinfo;
    ret = snd_mixer_channel_info( devhandle, num, &chinfo );
    if ( ret )
      devname = "unknown   ";
    else
      strncpy( devname, (const char *)chinfo.name, 11 );
#else
    strncpy(devname, MixerDevNames[num],11);
#endif
  devname[10]=0;
};

char* MixDevice::name()
{
  return devname;
}
