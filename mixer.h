//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * 1996-2000 Christian Esken <esken@kde.org>
 * Sven Fischer <herpes@kawo2.rwth-aachen.de>
 * 2002 - Helio Chissini de Castro <helio@conectiva.com.br>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef MIXER_H
#define MIXER_H

#include <qstring.h>
#include <qtimer.h>
#include <qobject.h>
#include <qintdict.h>
#include <qptrlist.h>

#include "volume.h"
#include "mixerIface.h"
#include "mixset.h"
#include "mixdevice.h"

class Volume;
class KConfig;

class Mixer : public QObject, virtual public MixerIface
{
      Q_OBJECT

   public:
      enum MixerError { ERR_PERM=1, ERR_WRITE, ERR_READ, ERR_NODEV, ERR_NOTSUPP,
			ERR_OPEN, ERR_LASTERR, ERR_NOMEM, ERR_INCOMPATIBLESET };

      Mixer( int device = -1, int card = -1 );
      virtual ~Mixer() {};

      /// Static function. This function must be overloaded by any derived mixer class
      /// to create and return an instance of the derived class.
      static int getDriverNum();
      static Mixer* getMixer( int driver, int device = 0, int card = 0 );
      static Mixer* getMixer( int driver, MixSet set,int device = 0, int card = 0 );

      void volumeSave( KConfig *config );
      void volumeLoad( KConfig *config );

       /// Tells the number of the mixing devices
      unsigned int size() const;
      /// Returns a pointer to the mix device with the given number
      MixDevice* operator[](int val_i_num);

      /// Returns a pointer to the mix device whose type matches the value
      /// given by the parameter and the array MixerDevNames given in
      /// mixer_oss.cpp (0 is Volume, 4 is PCM, etc.)
      MixDevice *mixDeviceByType( int deviceidx );

      /// Grabs (opens) the mixer for further intraction
      virtual int grab();
      /// Releases (closes) the mixer
      virtual int release();
      /// Prints out a translated error text for the given error number on stderr
      void errormsg(int mixer_error);
      /// Returns a translated error text for the given error number.
      /// Derived classes can override this method to produce platform
      /// specific error descriptions.
      virtual QString errorText(int mixer_error);
      virtual QString mixerName();

      // Returns the name of the driver, e.g. "OSS" or "ALSA0.9"
      QString driverName();
      static QString driverName(int num);

      /// set/get mixer number used to identify mixers with equal names
      void setMixerNum( int num );
      int mixerNum();

      /// get the actual MixSet
      virtual MixSet getMixSet() { return m_mixDevices; };

      /// Set the record source(s) according to the given device mask
      /// The default implementation does nothing.

      /// Gets the currently active record source(s) as a device mask
      /// The default implementation just returns the internal stored value.
      /// This value can be outdated, when another applications change the record
      /// source. You can override this in your derived class
      //  virtual unsigned int recsrc() const;

      /// Returns the number of the master volume device */
      int masterDevice() { return m_masterDevice; };

      /// Reads the volume of the given device into VolLeft and VolRight.
      /// Abstract method! You must implement it in your dericved class.
      virtual int readVolumeFromHW( int devnum, Volume &vol ) = 0;


      /// DCOP oriented methods (look at mixerIface.h for the descriptions)
      virtual void setVolume( int channeltype, int percentage );
      virtual void setMasterVolume( int percentage );

      virtual void increaseVolume( int channeltype );
      virtual void decreaseVolume( int channeltype );

      virtual int volume( int channeltype );
      virtual int masterVolume();

      virtual void setMute( int channeltype, bool on );
      virtual bool mute( int channeltype );
      virtual bool isRecordSource( int deviceidx );

      virtual bool isAvailableDevice( int deviceidx );

      void commitVolumeChange( MixDevice* md );

      virtual bool hasBrokenRecSourceHandling();

   public slots:
      /// Writes the given volumes in the given device
      /// Abstract method! You must implement it in your dericved class.
      virtual int writeVolumeToHW( int devnum, Volume &volume ) = 0;
      virtual void readSetFromHW();
      virtual void setRecordSource( int deviceidx, bool on );

      virtual void setBalance(int balance); // sets the m_balance (see there)

   signals:
      void newBalance( Volume& );
      void newRecsrc( void );
      void newVolumeLevels(void);

   protected:
      int m_devnum;
      int m_cardnum;
      int m_masterDevice; // device num for master volume
      /// Derived classes MUST implement this to open the mixer. Returns a KMix error
      // code (O=OK).
      virtual int openMixer() = 0;
      virtual int releaseMixer() = 0;

      virtual bool setRecsrcHW( int devnum, bool on) = 0;
      virtual bool isRecsrcHW( int devnum ) = 0;

      /// User friendly name of the Mixer (e.g. "IRIX Audio Mixer"). If your mixer API
      /// gives you a usable name, use that name.
      QString m_mixerName;

      QTimer* _pollingTimer;

      // mixer number to identify mixers with equal name correctly (set by the client)
      int m_mixerNum;

      bool m_isOpen;
      int m_balance; // from -100 (just left) to 100 (just right)

      // All mix devices of this phyisical device.
      MixSet m_mixDevices;

      QPtrList<MixSet> m_profiles;

   public:
      int setupMixer() { return setupMixer( m_mixDevices ); };
      int setupMixer( MixSet set );
};

#endif
