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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MIXER_H
#define MIXER_H

#include <QString>
#include <QTimer>
#include <QObject>
#include <qlist.h>

#include "volume.h"
class Mixer_Backend;
#include "mixset.h"
#include "mixdevice.h"

class Volume;
class KConfig;

class Mixer : public QObject
{
      Q_OBJECT

   public:
      enum MixerError { ERR_PERM=1, ERR_WRITE, ERR_READ,
			ERR_OPEN, ERR_LASTERR };

      Mixer( int driver, int device );
      virtual ~Mixer();

      static int numDrivers();
      QString getDriverName();

      MixDevice* find(const QString& devPK);

      void volumeSave( KConfig *config );
      void volumeLoad( KConfig *config );

       /// Tells the number of the mixing devices
      unsigned int size() const;


      /// Returns a pointer to the mix device with the given number
      MixDevice* operator[](int val_i_num);

      /// Returns a pointer to the mix device whose type matches the value
      /// given by the parameter and the array MixerDevNames given in
      /// mixer_oss.cpp (0 is Volume, 4 is PCM, etc.)
      MixDevice *getMixdeviceById( const QString& deviceID );

      /// Open/grab the mixer for further intraction
      bool openIfValid();

      /// Close/release the mixer
      virtual int close();

      /// Returns a detailed state message after errors. Only for diagnostic purposes, no i18n.
      QString& stateMessage() const;

      /// Returns the name of the card/chip/hardware, as given by the driver. The name is NOT instance specific,
      /// so if you install two identical soundcards, two of them will deliver the same mixerName().
      /// Use this method if you need an instance-UNspecific name, e.g. for finding an appropriate
      /// mixer layout for this card, or as a prefix for constructing instance specific ID's like in id().
      virtual QString baseName();

      /// Return the name of the card/chip/hardware, which is suitable for humans
      virtual QString readableName();

      // Returns the name of the driver, e.g. "OSS" or "ALSA0.9"
      static QString driverName(int num);

      /// Returns an unique ID of the Mixer. It currently looks like "<soundcard_descr>:<hw_number>@<driver>"
      QString& id();
      /// The owner/creator of the Mixer can set an unique name here. This key should never displayed to
      /// the user, but can be used for referencing configuration items and such.
      void setID(QString& ref_id);

      /// The KMix global master card. Please note that KMix and KMixPanelApplet can have a
      /// different MasterCard's at the moment (but actually KMixPanelApplet does not read/save this yet).
      /// At the moment it is only used for selecting the Mixer to use in KMix's DockIcon.
      static void setGlobalMaster(QString& ref_card, QString& ref_control);
      static Mixer* masterCard();
      /// The global Master Device inside the current MasterCard (as returned by masterCard()).
      static MixDevice* masterCardDevice();


      /// get the actual MixSet
      MixSet getMixSet();

      /// Returns the master volume device (doesn't work out :-(. See masterCard() and masterCardDevice() instead)
      MixDevice* masterDevice();
      /// Sets the master volume device (doesn't work out :-(. See setMasterCard() and setMasterCardDevice() instead)
      void setMasterDevice(QString&);

      /// DCOP oriented methods (look at mixerIface.h for the descriptions)
      virtual void setVolume( const QString& mixdeviceID, int percentage );
      virtual void setAbsoluteVolume( const QString& mixdeviceID, long absoluteVolume );
      virtual void setMasterVolume( int percentage );

      virtual void increaseVolume( const QString& mixdeviceID );
      virtual void decreaseVolume( const QString& mixdeviceID );

      virtual long absoluteVolume( const QString& mixdeviceID );
      virtual long absoluteVolumeMin( const QString& mixdeviceID );
      virtual long absoluteVolumeMax( const QString& mixdeviceID );
      virtual int volume( const QString& mixdeviceID );
      virtual int masterVolume();

      virtual void setMute( const QString& mixdeviceID, bool on );
      virtual bool mute( const QString& mixdeviceID );
      virtual void toggleMute( const QString& mixdeviceID );
      virtual bool isRecordSource( const QString& mixdeviceID );

      virtual bool isAvailableDevice( const QString& mixdeviceID );

      void commitVolumeChange( MixDevice* md );

   public slots:
      virtual void readSetFromHW();
      void readSetFromHWforceUpdate() const;
      virtual void setRecordSource( const QString& controlID, bool on );

      virtual void setBalance(int balance); // sets the m_balance (see there)

   signals:
      void newBalance( Volume& );
      void newRecsrc( void );
      void newVolumeLevels(void);

   protected:
      QTimer* _pollingTimer;

      int m_balance; // from -100 (just left) to 100 (just right)

      static QList<Mixer *> s_mixers;

   public:
      static QList<Mixer *>& mixers();

   private:
     Mixer_Backend *_mixerBackend;
      mutable bool _readSetFromHWforceUpdate;
      static int _dcopID;
      QString _id;
      QString _masterDevicePK;
      static QString _masterCard;
      static QString _masterCardDevice;
};

#endif
