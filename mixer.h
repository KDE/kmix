//-*-C++-*-
#ifndef KMIXER_H
#define KMIXER_H

// undef Above+Below because of Qt <-> X11 collision. Grr, I hate X11 headers
#undef Above
#undef Below
#undef Unsorted

#include <qslider.h>
#include <qlist.h>
#include <qlabel.h>

#include <kurl.h>
#include <kapp.h>


/*
   I am using a fixed MAX_MIXDEVS #define here.
   People might argue, that I should rather use the SOUND_MIXER_NRDEVICES
   #define used by OSS. But using this #define is not good, because it is
   evaluated during compile time. Compiling on one platform and running
   on another with another version of OSS with a different value of
   SOUND_MIXER_NRDEVICES is very bad. Because of this, usage of
   SOUND_MIXER_NRDEVICES should be discouraged.

   The #define below is only there for internal reasons.
   In other words: Don't play around with this value
 */
#define MAX_MIXDEVS 32

#if defined(sun) || defined(__sun__)
#define DEFAULT_MIXER "/dev/audioctl"
#elif sgi
#define DEFAULT_MIXER "SGI Mixer" // no device name on SGI
#define _LANGUAGE_C_PLUS_PLUS
#include <dmedia/audio.h>
#elif defined(ALSA)
#define DEFAULT_MIXER "ALSA Mixer"
#elif defined(hpux)
# define DEFAULT_MIXER "HP-UX Mixer"
# ifdef HAVE_ALIB_H
#  include <Alib.h>
#  define HPUX_MIXER
# endif
#else
// hope that we have a OSS System
# define DEFAULT_MIXER "/dev/mixer"
#endif

// For the crossreferencing between classes, I must declare all
// referenced classes here.
class MixChannel;
class MixSet;
class Mixer;

/****************************************************************************
  The internal device representation of a mixing channel:
  Sorry. This class is no nice, shiny, encapsulating class, which hides
  implementation details. This stuff still stems from old times, when kmix
  was called dmix and was built on top of Motif.
  Yes! I should rework this some day.

  device_num:    The ioctl() device number of the mixer source, as given by
                 the SOUND_MIXER_READ_DEVMASK ioctl().
  is_stereo:     TRUE for a source with stereo capabilities.
  is_recordable: TRUE for mixer devices, which can be recorded.
  is_recsrc:     TRUE for a source, which is currently recording source
  channel:	 Channel descriptor: 0 = Left, 1 = Right.
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
  int		num();
  void		setNum(int);
  QString	name();
  bool		stereo();
  bool		recordable();
  bool		recsrc();		// Is it currently being recorded?
  bool		disabled();		// Is slider disabled by user?
  bool		muted();		// Is it muted by user?
  bool		stereoLinked();

  void		setName(QString);
  void		setStereo(bool value);
  void		setRecordable(bool value);
  void		setRecsrc(bool value);
  void		setDisabled(bool value);
  void		setMuted(bool value);
  void		setStereoLinked(bool value);


  MixDevice	*Next;			// Pointer to next elment of MixDevice list
  MixChannel	*Left;			//
  MixChannel	*Right;			//
  Mixer		*mix;

  QLabel	*picLabel;

private:
  int		dev_num;		// ioctl() device number of mixer
  bool		StereoLink;		// Is this channel linked via the
                                        // left-right-controller?
  bool	 	is_stereo;		// Is it stereo capable?
  bool      	is_recordable;		// Can it be recorded?
  bool		is_recsrc;		// Is it currently being recorded?
  bool		is_disabled;		// Is slider disabled by user?
  bool		is_muted;		// Is it muted by user?
  QString	dev_name;		// Ascii channel name
};

/***************************************************************************
 * The structure MixChannel is used as hook for user data in the slots.
 * There are pointers to 2 MixChannel's per MixDevice. If neccesary, this
 * could could be modified, so one could build a MixChannel list.
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
  void VolChangedI(int new_pos);
};



class Mixer
{
public:
  enum { ERR_OPEN=1, ERR_WRITE, ERR_READ, ERR_NODEV, ERR_NOTSUPP, ERR_LASTERR };
  enum { LEFT, RIGHT, BOTH };


  Mixer();
  Mixer(int devnum, int SetNum);
  void init();
  void init(int devnum, int SetNum);
  virtual ~Mixer() {};

  static Mixer* getMixer(int devnum, int SetNum);

  virtual int grab();
  virtual int release();
  void errormsg(int mixer_error);
  virtual void updateMixDevice(MixDevice *mixdevice);
  virtual void setBalance(int left, int right);
  /// Write set 0 into the mixer hardware
  virtual void Set0toHW();
  /// Write a given set into set 0
  virtual void Set2Set0(int Source, bool copy_volume);
  virtual void Set0toSet(int Source);
  virtual void setRecsrc(unsigned int newRecsrc);
  unsigned int getRecsrc();



  void sessionSave(bool sessionConfig);

  int num_mixdevs;
  MixDevice	*First;
  ///  The mixing set list
  MixSetList *TheMixSets;



protected:
  virtual int release_I();
  virtual void setDevNumName_I(int devnum) = 0;

  QString	devname;


private:
  int		devnum;

#ifdef sgi
  // IRIX uses ALport stuff
  ALport	m_port;
  ALconfig	m_config;
#elif defined(HPUX_MIXER)
  // HP-UX uses Alib stuff
  Audio		*hpux_audio;
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


class Mixer_OSS : public Mixer
{
public:
  Mixer_OSS();
  Mixer_OSS(int devnum, int SetNum);
  virtual ~Mixer_OSS() {};

protected:
  virtual void setDevNumName_I(int devnum);
};

class Mixer_ALSA : public Mixer
{
public:
  Mixer_ALSA();
  Mixer_ALSA(int devnum, int SetNum);
  virtual ~Mixer_ALSA() {};

protected:
  virtual void setDevNumName_I(int devnum);
};

class Mixer_SUN : public Mixer
{
public:
  Mixer_SUN();
  Mixer_SUN(int devnum, int SetNum);
  virtual ~Mixer_SUN() {};

protected:
  virtual void setDevNumName_I(int devnum);
};

class Mixer_IRIX : public Mixer
{
public:
  Mixer_IRIX();
  Mixer_IRIX(int devnum, int SetNum);
  virtual ~Mixer_IRIX() {};

protected:
  virtual void setDevNumName_I(int devnum);
};

class Mixer_HPUX : public Mixer
{
public:
  Mixer_HPUX();
  Mixer_HPUX(int devnum, int SetNum);
  virtual ~Mixer_HPUX() {};

protected:
  virtual int release_I();
  virtual void setDevNumName_I(int devnum);
};


#endif
