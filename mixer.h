//-*-C++-*-

#ifndef MIXER_H
#define MIXER_H

//  #include <qslider.h>
#include <qstring.h>
#include <qobject.h>
#include <qintdict.h>
#include <qlist.h>

#include "volume.h"
#include "mixdevice.h"

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

class MixDevice;
class Volume;

typedef QList<MixDevice> MixSet;

class Mixer : public QObject
{
  Q_OBJECT

public:
  enum MixerError { ERR_PERM=1, ERR_WRITE, ERR_READ, ERR_NODEV, ERR_NOTSUPP,
                    ERR_OPEN, ERR_LASTERR, ERR_NOMEM, ERR_INCOMPATIBLESET };

  Mixer( int device = -1, int card = -1 );
  virtual ~Mixer() {};

  /// Static function. This function must be overloaded by any derived mixer class
  /// to create and return an instance of the derived class.
  static Mixer* getMixer( int device = 0, int card = 0 );
  static Mixer* getMixer( MixSet set,int device = 0, int card = 0 );


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

  void sessionSave(bool sessionConfig);

  /// get the actual MixSet
  virtual MixSet getMixSet() { return m_mixDevices; };
  /// Write a given MixSet to hardware
  virtual void writeMixSet( MixSet set );

  /// Set the record source(s) according to the given device mask
  /// The default implementation does nothing.

  /// Gets the currently active record source(s) as a device mask
  /// The default implementation just returns the internal stored value.
  /// This value can be outdated, when another applications change the record
  /// source. You can override this in your derived class
  //  virtual unsigned int recsrc() const;

  /*!
    Returns the number of the master volume device */
  int masterDevice() { return m_masterDevice; };

  /// Reads the volume of the given device into VolLeft and VolRight.
  /// Abstract method! You must implement it in your dericved class.
  virtual int readVolumeFromHW( int devnum, Volume &vol ) = 0;


public slots:
  /// Writes the given volumes in the given device
  /// Abstract method! You must implement it in your dericved class.
  virtual int writeVolumeToHW( int devnum, Volume volume ) = 0;
  virtual void readSetFromHW();

  virtual void setBalance(int balance); // sets the m_balance (see there)
  virtual void setRecsrc( int devnum, bool on = true);

signals:
  void newBalance( Volume );
  void newRecsrc( void );

protected:
  int		m_devnum;
  int		m_cardnum;
  int           m_masterDevice; // device num for master volume
  /// Derived classes MUST implement this to open the mixer. Returns a KMix error
  // code (O=OK).
  virtual int   openMixer() = 0;
  virtual int   releaseMixer() = 0;

  virtual bool  setRecsrcHW( int devnum, bool on) = 0;
  virtual bool  isRecsrcHW( int devnum ) = 0;

  /// User friendly name of the Mixer (e.g. "IRIX Audio Mixer"). If your mixer API
  /// gives you a usable name, use that name.
  QString       m_mixerName;


  bool		m_isOpen;
  int		m_balance; // from -100 (just left) to 100 (just right)

  // All mix devices of this phyisical device.
  MixSet        m_mixDevices;


private:

//    void setDevNumName(int devnum);
  int  setupMixer()
  { return setupMixer( m_mixDevices ); };
  int  setupMixer( MixSet set );
//    void updateMixDeviceI(MixDevice *mixdevice);

};

#endif
