//-*-C++-*-
#ifndef KMIXER_H
#define KMIXER_H

// undef Above+Below because of Qt <-> X11 collision. Grr, I hate X11 headers
#undef Above
#undef Below
#undef Unsorted

#include <qslider.h>
#include <qstring.h>
#include <qlist.h>
#include <qarray.h>
#include <qlabel.h>

#include <QceStateLED.h>
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
  int		num() const;
  QString	name() const;
  bool		stereo() const;
  bool		recordable() const;
  bool		recsrc() const;		// Is it currently being recorded?
  bool		disabled() const;	// Is slider disabled by user?
  bool		muted() const;		// Is it muted by user?
  bool		stereoLinked() const;

  void		setNum(int);
  void		setName(QString);
  void		setStereo(bool value);
  void		setRecordable(bool value);
  void		setRecsrc(bool value);
  void		setDisabled(bool value);
  void		setMuted(bool value);
  void		setStereoLinked(bool value);

  MixChannel	*Left;			//
  MixChannel	*Right;			//

  Mixer		*mix;

  QLabel	*picLabel;
  QceStateLED  	*i_KLed_state;		/* State LED (recsource = red)	   */

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

  static bool	i_b_HW_update;

  static void HW_update(bool val_b_update_allowed);

public slots:
  void VolChanged( int new_pos );
  void VolChangedI(int new_pos);
};



class Mixer
{
public:
  enum { ERR_PERM=1, ERR_WRITE, ERR_READ, ERR_NODEV, ERR_NOTSUPP, ERR_OPEN, ERR_LASTERR };
  enum { LEFT, RIGHT, BOTH };


  Mixer();
  Mixer(int devnum, int SetNum);
  void init();
  void init(int devnum, int SetNum);
  virtual ~Mixer() {};

  /// Static function. This function must be overloaded by any derived mixer class
  /// to create and return an instance of the derived class.
  static Mixer* getMixer(int devnum, int SetNum);


  /// Tells the number of the mixing devices
  unsigned int size() const;
  /// Returns a pointer to the mix device with the given number
  MixDevice* operator[](int val_i_num);

  /// Grabs (opens) the mixer for further intraction
  virtual int grab();
  /// Releases (closes) the mixer
  virtual int release();
  /// Prints out a translated error text for the given error number on stderr
  void errormsg(int mixer_error);
  /**
     Returns a translated error text for the given error number. Derived classes
     can override this method to produce platform specific error descriptions.
  */
  virtual QString errorText(int mixer_error);
  QString mixerName();

  virtual void updateMixDevice(MixDevice *mixdevice);
  virtual void setBalance(int left, int right);


  /// Write set 0 into the mixer hardware
  virtual void Set0toHW();
  /// Write a given set into set 0
  virtual void Set2Set0(int Source, bool copy_volume);
  virtual void Set0toSet(int Source);

  /// Set the record source(s) according to the given device mask
  /// The default implementation does nothing.
  virtual void setRecsrc(unsigned int newRecsrc);
  /// Gets the currently active record source(s) as a device mask
  /// The default implementation just returns the internal stored value.
  /// This value can be outdated, when another applications change the record
  /// source. You can override this in your derived class
  virtual unsigned int recsrc() const;
  /// Reads the volume of the given device into VolLeft and VolRight.
  /// Abstract method! You must implement it in your dericved class.
  virtual int readVolumeFromHW( int devnum, int *VolLeft, int *VolRight ) = 0;
  /// Writes the given volumes in the given device
  /// Abstract method! You must implement it in your dericved class.
  virtual int writeVolumeToHW( int devnum, int volLeft, int volRight ) = 0;

  void sessionSave(bool sessionConfig);

  ///  The mixing set list
  MixSetList *TheMixSets;



protected:
  virtual int releaseMixer() = 0;
  virtual void setDevNumName_I(int devnum) = 0;
  QString	devname;


protected:
  /// Derived classes MUST implement this to open the mixer. Returns a KMix error
  // code (O=OK).
  virtual int	openMixer() = 0;

  /// User friendly name of the Mixer (e.g. "IRIX Audio Mixer"). If your mixer API
  /// gives you a usable name, use that name.
  QString	i_s_mixer_name;
  bool		isOpen;
  unsigned int	devmask, recmask, i_recsrc, stereodevs;
  int		PercentLeft,PercentRight;

  ///  Maximum volume Level allowed by the Mixer API (OS dependent)
  int		MaxVolume;

private:
  int		devnum;

  void setDevNumName(int devnum);
  int  setupMixer(int devnum, int SetNum);
  void setupStructs(void);
  void updateMixDeviceI(MixDevice *mixdevice);

  // All mix devices of this phyisical device.
  QArray<MixDevice*> i_ql_mixDevices;
};
#endif
