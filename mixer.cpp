/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
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
#include <errno.h>

#include "sets.h"
#include "mixer.h"
#include "mixer.moc"

#include <qstring.h>
#include <klocale.h>

#if defined(sun) || defined(__sun__)
#define SUN_MIXER
#endif

#ifdef sgi
#include <sys/fcntl.h>
#define IRIX_MIXER
#endif

#ifdef linux
#ifdef ALSA
#define ALSA_MIXER
#else
#define OSS_MIXER
#endif
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__bsdi__) || defined(_UNIXWARE)
#define OSS_MIXER
#endif

#if defined(hpux) && defined(HAVE_ALIB_H)
#define HPUX_MIXER
#endif

// PORTING: add #ifdef PLATFORM , commands , #endif, add your new mixer below

#if defined(SUN_MIXER)
#include "mixer_sun.cpp"
#elif defined(IRIX_MIXER)
#include "mixer_irix.cpp"
#elif defined(ALSA_MIXER)
#include "mixer_alsa.cpp"
#elif defined(OSS_MIXER)
#include "mixer_oss.cpp"
#elif defined(HPUX_MIXER)
#include "mixer_hpux.cpp"
#else
// We cannot handle this! I install a dummy mixer instead.
#define NO_MIXER
include "mixer_none.cpp"
#endif



const char* MixerDevNames[32]={"Volume"  , "Bass"    , "Treble"    , "Synth"   , "Pcm"  ,    \
			       "Speaker" , "Line"    , "Microphone", "CD"      , "Mix"  ,    \
			       "Pcm2"    , "RecMon"  , "IGain"     , "OGain"   , "Line1",    \
			       "Line2"   , "Line3"   , "Digital1"  , "Digital2", "Digital3", \
			       "PhoneIn" , "PhoneOut", "Video"     , "Radio"   , "Monitor",  \
			       "3D-depth", "3D-center", "unknown"  , "unknown" , "unknown",  \
			       "unknown" , "unused" };


bool MixChannel::i_b_HW_update = true;


Mixer::Mixer()
{
}

Mixer::Mixer(int /*devnum*/, int /*SetNum*/)
{
}

void Mixer::init()
{
  // use default mixer device and no mixing set
  setupMixer(0, -1);
}

void Mixer::init(int devnum, int SetNum)
{
  setupMixer(devnum, SetNum);
}



void Mixer::sessionSave(bool /*sessionConfig*/)
{
  TheMixSets->write();
}



int Mixer::grab()
{
  int ret=0;

  if (!isOpen)
    // Try to open Mixer, if it is not open already.
    ret=openMixer();

  return ret;
}

int Mixer::release()
{
  int l_i_ret = 0;
  if(isOpen) {
    isOpen=false;
    // Call the target system dependent "release device" function
    l_i_ret = releaseMixer();
  }

  return l_i_ret;
}



unsigned int Mixer::size() const
{
  unsigned int l_i_num=0;

  MixDevice *l_mc_device = First;
  while ( l_mc_device != NULL ) {
    l_mc_device = l_mc_device->Next;
    l_i_num++;
  }
  return l_i_num;
}

MixDevice* Mixer::operator[](int val_i_num)
{
  MixDevice *l_mc_device = First;


  if (val_i_num < 1 ) {
    // Index too small (or first) => Return first entry
    debug("Mixer::operator[]: Falscher Index");
  }

  else {
    for ( int l_i = 1 ; l_i < val_i_num ; l_i++) {
      if ( l_mc_device->Next == NULL) {
	// Stop traversing, otherwise we would end up behind the last entry in the list
	break;
      }
      else {
	l_mc_device = l_mc_device->Next;
      }
    }
  }

  return l_mc_device;
}



int Mixer::setupMixer(int devnum, int SetNum)
{
  TheMixSets = new MixSetList;
  TheMixSets->read();  // Read sets from kmixrc

  isOpen = false;
  release();	// To be sure, release mixer before (re-)opening

  devmask = recmask = i_recsrc = stereodevs = 0;
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
  Set2Set0(-1,true);       // Read from hardware
  if ( SetNum >= 0)
    Set2Set0(SetNum,true); // Read (additionaly) from Set, if SetNum >= 0
  Set0toHW();              // Forward to the hardware

  return 0;
}


void Mixer::setDevNumName(int devnum)
{
  QString devname;
  this->devnum  = devnum;
  this->setDevNumName_I(devnum);
}


void Mixer::readVolumeFromHW( int /*devnum*/, int */*VolLeft*/, int */*VolRight*/ )
{
  errormsg(Mixer::ERR_READ);  
}

/*
void Mixer::setDevNumName_I(int devnum)
{
  devname = "Unknown Mixer";
}
*/


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
  recsrcwork  = i_recsrc;
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
      MixPtr->setNum(i);
      MixPtr->setStereo(stereowork & 1);
      MixPtr->setRecsrc(recsrcwork & 1);
      MixPtr->setRecordable(recwork & 1);
      MixPtr->setDisabled(false);
      MixPtr->setMuted(false);
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
	 (mse != NULL) && (mse->devnum != MixPtr->num() );
	 mse=SrcSet->next() );

    if (mse == NULL)
      continue;  // entry not found

    MixPtr->setStereoLinked	(mse->StereoLink);
    MixPtr->setMuted		(mse->is_muted  );
    MixPtr->setDisabled		(mse->is_disabled);
    if (! MixPtr->disabled() ) {
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
  int		VolLeft,VolRight;

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
      MixSetEntry *mse = DestSet->findDev(MixPtr->num());
      if (mse == NULL)
	continue;  // entry not found

      readVolumeFromHW( MixPtr->num(), &VolLeft, &VolRight);

      if ( (VolLeft == VolRight) && (MixPtr->stereo() ) )
	mse->StereoLink = true;
      else
	mse->StereoLink = false;

      mse->volumeL = VolLeft;
      if ( MixPtr->stereoLinked() )
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


QString Mixer::mixerName()
{
  return i_s_mixer_name;
}




void Mixer::errormsg(int mixer_error)
{
  QString l_s_errText;
  l_s_errText = errorText(mixer_error);
  cerr << l_s_errText << "\n";
}

QString Mixer::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
    {
    case ERR_PERM:
      l_s_errmsg = i18n("kmix:You have no permission to access the mixer device.\n" \
			"Please check your operating systems manual to allow the access.");
      break;
    case ERR_WRITE:
      l_s_errmsg = i18n("kmix: Could not write to mixer.");
      break;
    case ERR_READ:
      l_s_errmsg = i18n("kmix: Could not read from mixer.");
      break;
    case ERR_NODEV:
      l_s_errmsg = i18n("kmix: Your mixer does not control any devices.");
      break;
    case  ERR_NOTSUPP: 
      l_s_errmsg = i18n("kmix: Mixer does not support your platform. See mixer.cpp for porting hints (PORTING).");
      break;
    case ERR_OPEN: 
      l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
			"Please check that the soundcard is installed and the\n" \
			"soundcard driver is loaded\n");
      break;
    default:
      l_s_errmsg = i18n("kmix: Unknown error. Please report, how you produced this error.");
      break;
    }
  return l_s_errmsg;
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


  if ( mixdevice->muted() ) {
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
  switch( mixdevice->num() ) {
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
  writeVolumeToHW(mixdevice->num(), volLeft, volRight);
// PORTING: add #ifdef PLATFORM , commands , #endif
#endif
#ifdef ALSA
  snd_mixer_channel_t data;
  ret = snd_mixer_channel_read( devhandle, mixdevice->num(), &data );
  if ( !ret ) {
    data.left = volLeft;
    data.right = volRight;
    data.channel = mixdevice->num();
    ret = snd_mixer_channel_write( devhandle, mixdevice->num(), &data );
    if ( ret )
      errormsg(Mixer::ERR_WRITE);
  }
  else
    errormsg(Mixer::ERR_READ);
#endif
#ifdef HPUX_MIXER
  // Set volume (right&left)
/*  long out_buf[4] =
  {
    0, volRight,
    0, volLeft
  };
  switch( mixdevice->num() ) {
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
  ALsetparams(AL_DEFAULT_DEVICE, out_buf, 4); */
#endif
// PORTING: add #ifdef PLATFORM , commands , #endif
}


void MixChannel::VolChanged(int new_pos)
{
  this->VolChangedI(100-new_pos); // The slider is reversed
}


void MixChannel::VolChangedI(int new_pos)
{
  MixDevice *mixdevice=mixDev;
  volume = new_pos;

  /* Set right channel volume to the same amount as left channel, if right
   * is linked to left
   */
  if (mixdevice->stereoLinked() )
    mixdevice->Right->volume = volume;

  MixSet *Set0  = mixDev->mix->TheMixSets->first();
  MixSetEntry *mse = Set0->findDev(mixdevice->num() );
  if (mse) {
    mse->volumeL =  mixdevice->Left->volume;
    mse->volumeR =  mixdevice->Right->volume;
  }
  else {
    cerr << "MixChannel::VolChanged(): no such mix set entry\n";
  }
  if ( i_b_HW_update ) {
    mixdevice->mix->updateMixDevice(mixdevice);
  }
}

void MixChannel::HW_update(bool val_b_update_allowed)
{
  i_b_HW_update = val_b_update_allowed;
}




void MixDevice::MvolSplitCB()
{
  StereoLink = !StereoLink;
  Right->volume = Left->volume = (Right->volume + Left->volume)/2;

  MixSet *Set0  = mix->TheMixSets->first();
  MixSetEntry *mse = Set0->findDev( num() );
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
  MixSetEntry *mse = Set0->findDev( num() );
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
  unsigned int old_recsrc = mix->recsrc();
  // Determine the Bit in the record source mask, which must be changed
  unsigned int bit_in_recsrc =  ( 1 << num() );
  // And calculate the new wanted record source mask.
  unsigned int new_recsrc = old_recsrc ^ bit_in_recsrc;

  mix->setRecsrc(new_recsrc);

  // If something has changed, tell the UI to a relayout.
  // OK! In future, I will give relayout hints, so the UI will only
  // relayout certain parts, e.g. the Record source "bullets". TODO !!!
  if (mix->recsrc() != old_recsrc)
    emit relayout();
}

unsigned int Mixer::recsrc() const
{
  return i_recsrc;
}

void Mixer::setRecsrc(unsigned int newRecsrc)
{
#if 1
#warning Christian: Have to port over setRecsrc() to derived classes
#else
MixDevice *MixDev = First; /* moved up for use with ALSA */
#ifdef OSS_MIXER
  // Change status of record source(s)
  if (ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &newRecsrc) == -1)
    errormsg (Mixer::ERR_WRITE);
  // Re-read status of record source(s). Just in case, OSS does not like
  // my settings. And with this line mix->recsrc gets its new value. :-)
  if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
    errormsg(Mixer::ERR_READ);
#elif defined(ALSA)
  snd_mixer_channel_t data;
  i_recsrc = 0;
  while (MixDev) {
    ret = snd_mixer_channel_read( devhandle, MixDev->num(), &data ); /* get */
    if ( ret )
      errormsg(Mixer::ERR_READ);
    if ( newRecsrc & ( 1 << MixDev->num() ) )
      data.flags |= SND_MIXER_FLG_RECORD;
    else
      data.flags &= ~SND_MIXER_FLG_RECORD;
    ret = snd_mixer_channel_write( devhandle, MixDev->num(), &data ); /* set */
    if ( ret )
      errormsg(Mixer::ERR_WRITE);
    ret = snd_mixer_channel_read( devhandle, MixDev->num(), &data ); /* check */
    if ( ret )
      errormsg(Mixer::ERR_READ);
    if ( ( data.flags & SND_MIXER_FLG_RECORD ) && /* if it's set and stuck */
         ( newRecsrc & ( 1 << MixDev->num() ) ) ) {
      i_recsrc |= 1 << MixDev->num();
      MixDev->setRecsrc(true);
    }
    else {
      MixDev->setRecsrc(false);
    }
    MixDev = MixDev->Next;
  }
  return; /* I'm done */ 
#elif defined(HPUX_MIXER)
  i_recsrc = newRecsrc;
  // nothing else (yet!)
#else
  KMsgBox::message(0, "Porting required.", "Please port this feature :-)", KMsgBox::INFORMATION, "OK" );
  // PORTING: Hint: Do not forget to set i_recsrc to the new valid
  //                record source mask.
#endif

  /* Traverse through the mixer devices and set the record source flags
   * This is especially necessary for mixer devices that sometimes do
   * not obey blindly (because of hardware limitations)
   */
  unsigned int recsrcwork = i_recsrc;
  while (MixDev) {
    if (recsrcwork & (1 << (MixDev->num()) ) )
      MixDev->setRecsrc(true);
    else
      MixDev->setRecsrc(false);

    MixDev            = MixDev->Next;
  }
#endif
}


MixDevice::MixDevice(int num)
{
  setNum(num);
#ifdef ALSA
    snd_mixer_channel_info_t chinfo;
    ret = snd_mixer_channel_info( devhandle, num, &chinfo );
    if ( ret )
      setName("unknown   ");
    else
      setName(chinfo.name);
#else
    setName(MixerDevNames[num]);
#endif
};



QString MixDevice::name() const
{
  return dev_name;
}




void MixDevice::setNum(int num)
{
  dev_num = num;
}

void MixDevice::setName(QString name)
{
  dev_name = name;
}
int MixDevice::num() const		{ return dev_num; }
bool MixDevice::stereo() const		{ return is_stereo; }
bool MixDevice::recordable() const	{ return is_recordable; }
bool MixDevice::recsrc() const		{ return is_recsrc; }
bool MixDevice::disabled() const	{ return is_disabled; }
bool MixDevice::muted() const		{ return is_muted; }
bool MixDevice::stereoLinked() const	{ return StereoLink; }


void MixDevice::setStereo(bool value)	{ is_stereo = value; }
void MixDevice::setRecordable(bool value)	{ is_recordable = value; }
void MixDevice::setRecsrc(bool value)	{ is_recsrc = value; }
void MixDevice::setDisabled(bool value)	{ is_disabled = value; }
void MixDevice::setMuted(bool value)		{ is_muted = value; }
void MixDevice::setStereoLinked(bool value)	{ StereoLink = value; }
