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


#include "kmix-platforms.cpp"



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
    MixChannel::i_b_HW_update = true;
}

Mixer::Mixer(int /*devnum*/, int /*SetNum*/)
{
    MixChannel::i_b_HW_update = true;
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
  i_set_allMixSets->write();
}



int Mixer::grab()
{
  int ret=0;

  if (!i_b_open)
    // Try to open Mixer, if it is not open already.
    ret=openMixer();

  return ret;
}

int Mixer::release()
{
  int l_i_ret = 0;
  if(i_b_open) {
    i_b_open=false;
    // Call the target system dependent "release device" function
    l_i_ret = releaseMixer();
  }

  return l_i_ret;
}



unsigned int Mixer::size() const
{
  return i_ql_mixDevices.size();
}

MixDevice& Mixer::operator[](int val_i_num)
{
  MixDevice *l_mc_device;

  if ( (val_i_num < 0) || (val_i_num >= (int)i_ql_mixDevices.size()) ) {
    // Index wrong => Return 0
    debug("Mixer::operator[]: Wrong Index");
    l_mc_device = 0;
  }
  else {
    l_mc_device =  i_ql_mixDevices[val_i_num];
  }

  return *l_mc_device;
}



int Mixer::setupMixer(int devnum, int SetNum)
{
  i_set_allMixSets = new MixSetList;
  i_set_allMixSets->read();  // Read sets from kmixrc

  i_b_open = false;
  release();	// To be sure, release mixer before (re-)opening

  devmask = recmask = i_recsrc = stereodevs = 0;
  PercentLeft = PercentRight=100;

  // set the mixer device name from a given device id
  setDevNumName(devnum);


  int ret = openMixer();
  if (ret != 0) {
    return ret;
  }

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
  Set2HW(-1,true);       // Read from hardware
  if ( SetNum >= 0) {
    Set2HW(SetNum,true); // Read (additionaly) from Set, if SetNum >= 0
  }
  return 0;
}


void Mixer::setDevNumName(int devnum)
{
  QString devname;
  this->devnum  = devnum;
  this->setDevNumName_I(devnum);
}

MixDevice* Mixer::createNewMixDevice(int num)
{ 
  return new MixDevice(num); 
};


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

  // Copy the bit mask to scratch registers. I will do bit shiftings on these.
  devicework  = devmask;
  recwork     = recmask;
  recsrcwork  = i_recsrc;
  stereowork  = stereodevs;
  int l_i_numMixdevs = 0;

  for (int i=0 ; i<MAX_MIXDEVS; i++) {
    if ( (devicework & 1) == 1 ) {
      // If we come here, the mixer device with num i is supported by sound driver.

      // 1) Create the new MixDevice
      i_ql_mixDevices.resize(l_i_numMixdevs+1);
      MixDevice *MixPtr = new MixDevice(i);
      i_ql_mixDevices[l_i_numMixdevs] = MixPtr;

#warning Check, if it is a good idea to keep createNewMixDevice()
      //createNewMixDevice(i);

      // 2) Fill the MixDevice
      MixPtr->mix  = this;		// keep track, which mixer is used by device i
      // 2a) Fill the two MixChannel's
      MixPtr->Left  = new MixChannel;
      MixPtr->Right = new MixChannel;
      MixPtr->Left->mixDev = MixPtr->Right->mixDev = MixPtr;
#if 0
      MixPtr->Left->channel  = Mixer::LEFT;
      MixPtr->Right->channel = Mixer::RIGHT;
#endif
      // 2b) Fill out the device structure
      MixPtr->setNum(i);
      MixPtr->setStereo(stereowork & 1);
      MixPtr->setRecsrc(recsrcwork & 1);
      MixPtr->setRecordable(recwork & 1);
      MixPtr->setDisabled(false);
      MixPtr->setMuted(false);

      // 4) Next device
      l_i_numMixdevs++;
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
 * This functions writes a mixing set with number "Source" into the "Hardware"
 *****************************************************************************/
void Mixer::Set2HW(int Source, bool copyVolume)
{
  int		VolLeft,VolRight;

  MixSet	*SrcSet;

  if ( Source >= 0 ) {
    // Read from a set (means copy set data to current MixSet)
    SrcSet = ((*i_set_allMixSets)[Source]);
  }

  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < size(); l_i_mixDevice++) {
    // Get a pointer to the given mix device 
    MixDevice &MixPtr =  this->operator[](l_i_mixDevice);

    if ( Source < 0 ) {
      // Read from hardware
      readVolumeFromHW( MixPtr.num(), &VolLeft, &VolRight);
      if (! MixPtr.disabled() ) {
	MixPtr.setVolume(0, int(100 * VolLeft  / (float)MaxVolume) );
	MixPtr.setVolume(1, int(100 * VolRight / (float)MaxVolume) );

	// MixPtr.setStereoLinked(true); -<- Nicht dran rummanipulieren !
      }
    }
    else {
      // Read from a set (means copy set data to current MixSet)
      MixSetEntry &l_mse  = *((*SrcSet)[l_i_mixDevice]);
      // if (! MixPtr.disabled() ) { }
      MixPtr.setVolume(0, l_mse.volumeL);
      MixPtr.setVolume(1, l_mse.volumeR);
      MixPtr.setStereoLinked (l_mse.StereoLink);
      MixPtr.setMuted	      (l_mse.is_muted  );
      MixPtr.setDisabled     (l_mse.is_disabled);
    }
  }
}



void Mixer::HW2Set(int Source)
{
#warning Have to devise a method to create "missing" sets

  MixSet	&DestSet = *((*i_set_allMixSets)[Source]);
  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < size(); l_i_mixDevice++) {
    // Get a pointer to the given mix device 
    MixDevice &MixPtr = this->operator[](l_i_mixDevice);
    // Get a pointer to the mix set entry
    MixSetEntry &l_mse = *(DestSet[l_i_mixDevice]);

    l_mse.StereoLink	= MixPtr.stereoLinked();
    l_mse.is_muted	= MixPtr.muted();
    l_mse.is_disabled	= MixPtr.disabled();
    l_mse.volumeL	= MixPtr.volume(0);
    l_mse.volumeR	= MixPtr.volume(1);
  }
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
    case  ERR_NOMEM:
      l_s_errmsg = i18n("kmix: Not enough memory.");
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
  if (mixdevice == 0 )
    {
      /* Argument 0 => Update all devices */
      for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < size(); l_i_mixDevice++) {
	MixDevice &MixPtr = operator[](l_i_mixDevice);
	  updateMixDeviceI(&MixPtr);
      }
    }
  else {
    updateMixDeviceI(mixdevice);
  }
}

/* Internal function */
void Mixer::updateMixDeviceI(MixDevice *mixdevice)
{
  int volLeft,volRight;


  if ( mixdevice->muted() ) {
    // The mute is on! Thats really easy then
    volLeft = volRight = 0;
  }
  else {
    // Not muted. Hmm, lets ask left and right channel
    volLeft  = mixdevice->volume(0);
    volRight = mixdevice->volume(1);

    /* Now multiply the value with the percentage given by the stereo
     * slider. Multiply with the maximum allowed value (100 with OSS,
     * 255 with SGI) Divide everything by 100.
     * This give a value in the interval [0..100] or [0..255].
     */
    volLeft  = (volLeft *PercentLeft *MaxVolume) / 10000;
    volRight = (volRight*PercentRight*MaxVolume) / 10000;
  }

  int l_i_err = writeVolumeToHW(mixdevice->num(), volLeft, volRight);
  if (l_i_err != 0) {
    errormsg(l_i_err);
  }
}


void MixChannel::VolChanged(int new_pos)
{
  this->VolChangedI(100-new_pos); // The slider is reversed
}


void MixChannel::VolChangedI(int volume)
{
  MixDevice *mixdevice=mixDev;
  mixdevice->setVolume(0,volume);

  /* Set right channel volume to the same amount as left channel, if right
   * is linked to left
   */
  if (mixdevice->stereoLinked() ) {
    mixdevice->setVolume(1,volume);
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
  setStereoLinked( !stereoLinked() );
  setVolume(0, (volume(1) + volume(0))/2);
  setVolume(1, (volume(1) + volume(0))/2);

  /* This really should better be a member of Mixer and not MixDevice.
   * But I do not want every darn Class to be a derivation of QWidget.
   * So I do a bit magic in here.
   */
  mix->updateMixDevice(NULL);

  emit relayout();
}


void MixDevice::MvolMuteCB()
{
  setMuted( ! muted() );

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

void Mixer::setRecsrc(unsigned int /* newRecsrc */)
{
}



MixDevice::MixDevice(int num, const char* name)
{
  i_mse = new MixSetEntry;
  setNum(num);
  if( name == 0 ) 
    setName(MixerDevNames[num]);
  else
    setName( name );
};



QString MixDevice::name() const
{
  return i_mse->name;
}




void MixDevice::setNum(int num)
{
  i_mse->devnum = num;
}

void MixDevice::setName(QString name)
{
  i_mse->name = name;
}
int MixDevice::num() const		{ return i_mse->devnum; }
bool MixDevice::stereo() const		{ return i_mse->is_stereo; }
bool MixDevice::recordable() const	{ return i_mse->is_recordable; }
bool MixDevice::recsrc() const		{ return i_mse->is_recsrc; }
bool MixDevice::disabled() const	{ return i_mse->is_disabled; }
bool MixDevice::muted() const		{ return i_mse->is_muted; }
bool MixDevice::stereoLinked() const	{ return i_mse->StereoLink; }
int MixDevice::volume(char channel)
{
  if (channel == 0) {
    return i_mse->volumeL;
  }
  else if (channel == 1) {
    return i_mse->volumeR;
  }
  else {
    return 0;
  }
}

void MixDevice::setStereo(bool value)	{ i_mse->is_stereo = value; }
void MixDevice::setRecordable(bool value)	{ i_mse->is_recordable = value; }
void MixDevice::setRecsrc(bool value)	{ i_mse->is_recsrc = value; }
void MixDevice::setDisabled(bool value)	{ i_mse->is_disabled = value; }
void MixDevice::setMuted(bool value)		{ i_mse->is_muted = value; }
void MixDevice::setStereoLinked(bool value)	{ i_mse->StereoLink = value; }
void MixDevice::setVolume(char channel, int volume)
{
#warning Change this to a list
  if (channel == 0) {
    i_mse->volumeL = volume;
  }
  else if (channel == 1) {
    i_mse->volumeR = volume;
  }
}
