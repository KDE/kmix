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
class Mixer_Backend;
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
			ERR_OPEN, ERR_LASTERR, ERR_NOMEM, ERR_INCOMPATIBLESET, ERR_MIXEROPEN };

      Mixer( int driver, int device );
      virtual ~Mixer();

      static int numDrivers();
      /// Static function. This function must be overloaded by any derived mixer class
      /// to create and return an instance of the derived class.
      //static Mixer* getMixer( int driver, int device = 0 );
      //static Mixer* getMixer( int driver, MixSet set,int device = 0 );

      void volumeSave( KConfig *config );
      void volumeLoad( KConfig *config );

       /// Tells the number of the mixing devices
      unsigned int size() const;
      
      bool isValid();
      
      /// Returns a pointer to the mix device with the given number
      MixDevice* operator[](int val_i_num);

      /// Returns a pointer to the mix device whose type matches the value
      /// given by the parameter and the array MixerDevNames given in
      /// mixer_oss.cpp (0 is Volume, 4 is PCM, etc.)
      MixDevice *mixDeviceByType( int deviceidx );

      /// Open/grab the mixer for further intraction
      virtual int open();
      /// Close/release the mixer
      virtual int close();

      /// Returns a detailed state message after errors. Only for diagnostic purposes, no i18n.
      QString& stateMessage() const;

      virtual QString mixerName();

      // Returns the name of the driver, e.g. "OSS" or "ALSA0.9"
      QString driverName();
      static QString driverName(int num);

      /// Returns an unique ID of the Mixer. It currently looks like "<soundcard_descr>:<hw_number>"
      QString& id();

      /// get the actual MixSet
      MixSet getMixSet();

      /// Returns the id of the master volume device
      int masterDevice();
      /// Sets the id of the master volume device
      void setMasterDevice(int);

      /// DCOP oriented methods (look at mixerIface.h for the descriptions)
      virtual void setVolume( int deviceidx, int percentage );
      virtual void setAbsoluteVolume( int deviceidx, long absoluteVolume );
      virtual void setMasterVolume( int percentage );

      virtual void increaseVolume( int deviceidx );
      virtual void decreaseVolume( int deviceidx );

      virtual long absoluteVolume( int deviceidx );
      virtual long absoluteVolumeMin( int deviceidx );
      virtual long absoluteVolumeMax( int deviceidx );
      virtual int volume( int deviceidx );
      virtual int masterVolume();

      virtual void setMute( int deviceidx, bool on );
      virtual bool mute( int deviceidx );
      virtual void toggleMute( int deviceidx );
      virtual bool isRecordSource( int deviceidx );

      virtual bool isAvailableDevice( int deviceidx );

      void commitVolumeChange( MixDevice* md );

   public slots:
      virtual void readSetFromHW();
      void readSetFromHWforceUpdate() const;
      virtual void setRecordSource( int deviceidx, bool on );

      virtual void setBalance(int balance); // sets the m_balance (see there)

   signals:
      void newBalance( Volume& );
      void newRecsrc( void );
      void newVolumeLevels(void);

   protected:
      QTimer* _pollingTimer;

      int m_balance; // from -100 (just left) to 100 (just right)

      QPtrList<MixSet> m_profiles;
      static QPtrList<Mixer> s_mixers;

   public:
      int setupMixer( MixSet set );
      static QPtrList<Mixer>& mixers();

   private:
     Mixer_Backend *_mixerBackend;
      mutable bool _readSetFromHWforceUpdate;
      static int _dcopID;
      QString _id;
};

#endif
