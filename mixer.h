//-*-C++-*-
#ifndef KMIXER_H
#define KMIXER_H

// undef Above+Below because of Qt <-> X11 collision. Grr, I hate X11 headers
#undef Above
#undef Below
#undef Unsorted
#include <qslider.h>
#include <qlist.h>
#include <kurl.h>
#include <kapp.h>
#include <kmsgbox.h>

#define MAX_MIXDEVS 32

#if defined(sun) || defined(__sun__)
#define DEFAULT_MIXER "/dev/audioctl"
#elif sgi
#define DEFAULT_MIXER "SGI Mixer" // no device name on SGI
#define _LANGUAGE_C_PLUS_PLUS
#include <dmedia/audio.h>
#elif defined(ALSA)
#define DEFAULT_MIXER "ALSA Mixer"
#else
// hope that we have a OSS System
#define DEFAULT_MIXER "/dev/mixer"
#endif

// For the crossreferencing between classes, I must declare all
// referenced classes here.
class MixChannel;
class MixSet;
class Mixer;

/****************************************************************************
  The internal device representation of a mixing channel:
  device_num:    The ioctl() device number of the mixer source, as given by
                 the SOUND_MIXER_READ_DEVMASK ioctl().
  is_stereo:     TRUE for a source with stereo capabilities.
  is_recordable: TRUE for mixer devices, which can be recorded.
  is_recsrc:     TRUE for a source, which is currently recording source
  channel:	 Channel descriptor: 0 = Left, 1 = Right.
		 !!! This may be expanded to EVEN = Left, ODD = Right in
		 !!! the future. This interpretation may be useful in
		 !!! conjunction with a multiple channel soundcard as
		 !!! the GUS. Multiple channels may be controlled via a
		 !!! single slider.
  current_value: The current volume of this channel [0...10000]. This is an
		 internal value only and is getting converted in the
		 update_channel() function.
****************************************************************************/
class MixDevice : public QWidget
{
  Q_OBJECT

public slots:
    void MvolSplitCB();
    void MvolMuteCB();
    void MvolRecsrcCB();

signals:
    void relayout();

public:
  MixDevice(int num);
  MixDevice	*Next;			// Pointer to next elment of MixDevice list
  MixChannel	*Left;			//
  MixChannel	*Right;			//
  Mixer		*mix;
  char* name();
  QLabel	*picLabel;
  int		device_num;		// ioctl() device number of mixer
  bool		StereoLink;		// Is this channel linked via the
                                        // left-right-controller?
  bool	 	is_stereo;		// Is it stereo capable?
  bool      	is_recordable;		// Can it be recorded?
  bool		is_recsrc;		// Is it currently being recorded?
  bool		is_disabled;		// Is slider disabled by user?
  bool		is_muted;		// Is it muted by user?
  char		devname[11];		// Ascii channel name (10char max)
};

/***************************************************************************
 * The structure MixChannel is used as hook for user data in callbacks.
 * There are pointers to 2 MixChannel's per MixDevice. If neccesary, this
 * could could be modified, so one could build a MixChannel list.
 *
 * !!! Hmm, I am wondering if there are there Multi-Channel (more than 2) cards
 * out there, which can regulate each channel separately (GUS perhaps?!?).
 ***************************************************************************/
class MixChannel : public QWidget
{
  Q_OBJECT

public:
  MixDevice	*mixDev;
  char		channel;		/* channel number:                 */
					/* Even = Left, Odd = Right        */
  int		volume;			/* Volume of this channel	   */
  QSlider	*slider;		/* Associated slider               */
public slots:
  void	VolChanged( int new_pos );
};



class Mixer
{
public:
  enum { ERR_OPEN=1, ERR_WRITE, ERR_READ, ERR_NODEV, ERR_NOTSUPP, ERR_LASTERR };
  enum { LEFT, RIGHT, BOTH };


  Mixer();
  Mixer(int devnum, int SetNum);
  int grab();
  int release();
  void errormsg(int mixer_error);
  void updateMixDevice(MixDevice *mixdevice);
  void setBalance(int left, int right);
  /// Write set 0 into the mixer hardware
  void Set0toHW();
  /// Write a given set into set 0
  void Set2Set0(int Source, bool copy_volume);
  void Set0toSet(int Source);
  void setRecsrc(unsigned int newRecsrc);
  unsigned int getRecsrc();
  void sessionSave(bool sessionConfig);

  int num_mixdevs;
  MixDevice	*First;
  ///  The mixing set list
  MixSetList *TheMixSets;

private:
  char		*devname;
  int		devnum;

#ifdef sgi
  // IRIX uses ALport stuff
  ALport	m_port;
  ALconfig	m_config;
#else
  // Other platforms use a standard file descriptor
  int		fd;
#endif

  bool		isOpen;

  void setDevNumName(int devnum);
  int  setupMixer(int devnum, int SetNum);
  void setupStructs(void);
  int  openMixer(void);
  void updateMixDeviceI(MixDevice *mixdevice);

  unsigned int devmask, recmask, recsrc, stereodevs;
  int PercentLeft,PercentRight;
  ///  Maximum volume Level allowed by the Mixer API (OS dependent)
  int MaxVolume;
};

#endif
