//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken <esken@kde.org>
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
 
#ifndef MIXERBACKEND_H
#define MIXERBACKEND_H

#include <QString>
#include <QTime>
#include <QTimer>
#include "core/mixdevice.h"
#include "core/mixset.h"
#include "kmix_debug.h"

class Mixer;


class MixerBackend : public QObject
{
  Q_OBJECT

  // The MixerBackend may only be accessed via the Mixer class.
  friend class Mixer;

protected:
  MixerBackend(Mixer *mixer, int devnum);
  virtual ~MixerBackend();

  /**
   * Derived classes MUST implement this to open the mixer.
   *
   * @return a KMix error code (O=OK).
   */
  virtual int open() = 0;

  /**
   * Derived classes MUST implement this to close the mixer. Do not call this directly,
   * but use closeCommon() instead.  The method cannot be pure virtual as we use close()
   * in the destructor, and C++ does not allow this.
   * https://stackoverflow.com/questions/99552
   *
   * @return a KMix error code (O=OK).
   */
  virtual int close();

  /**
   * Deinitialize this backend, freeing resources.
   */
  void closeCommon();

  /**
   * Returns the driver name, e.g. "ALSA" or "OSS". This virtual method is for
   * looking up the driver name on instantiated objects.
   *
   * Please note, that there is also a global character string variable for
   * the driver name.  It and this function should return the same string value.
   * (Because there is no "virtual static" in C++, I need the method twice).
   * The global value is for the mixer factory, which needs it *before* instantiating
   * an object.   While it is not a member function, its implementation
   * can still be found in the corresponding backend implementation.  For example,
   * in mixer_oss.cpp there is a global variable called OSS_driverName.
   */
  virtual QString getDriverName() = 0;

  /**
   * Opens the mixer, if it constitutes a valid device.
   *
   * @return @c true if the open succeeded, or @false if the mixer could not
   * be opened or is not supported by the backend.  The two typical cases are:
   * (1) No such hardware installed
   * (2) The hardware exists, but has no mixer support (e.g. is an external sound card
   * with only mechanical volume knobs).
   *
   * The implementation calls open(), checks the return code and whether the number
   * of supported channels is>0.  The device remains opened if it is valid,
   * otherwise close() is done.
   */
  bool openIfValid();

  /** @return true, if the Mixer is open (and thus can be operated) */
  bool isOpen();

  virtual bool hasChangedControls();
  void readSetFromHWforceUpdate() const;

  /// Volume Read
  virtual int readVolumeFromHW( const QString& id, shared_ptr<MixDevice> ) = 0;
  /// Volume Write
  virtual int writeVolumeToHW( const QString& id, shared_ptr<MixDevice> ) = 0;

  /// Enums
  virtual void setEnumIdHW(const QString& id, unsigned int);
  virtual unsigned int enumIdHW(const QString& id);

  virtual bool moveStream(const QString &id, const QString &destId);
  virtual QString currentStreamDevice(const QString &id) const;

  // Future directions: Move media*() methods to MediaController class
  virtual int mediaPlay(QString ) { return 0; }; // implement in the backend if it supports it
  virtual int mediaPrev(QString ) { return 0; }; // implement in the backend if it supports it
  virtual int mediaNext(QString ) { return 0;}; // implement in the backend if it supports it

  /// Overwrite in the backend if the backend can see changes without polling
  virtual bool needsPolling() { return true; }

  shared_ptr<MixDevice> recommendedMaster();

  /**
   * Return a translated error text for the given error number.
   * Subclasses can override this method to produce platform
   * specific error descriptions.
   */
  virtual QString errorText(int mixer_error);

  /**
   * Return a translated WhatsThis message for a control.
   * Subclasses can override this method to produce backend
   * specific control descriptions.
   */
  virtual QString translateKernelToWhatsthis(const QString &kernelName) const;

  /**
   * The user friendly name of the Mixer (e.g. "USB 7.1 Surround System").
   * If your mixer API gives you a usable name, use that name.
   */
  virtual QString getName() const;
  virtual QString getId() const;
  virtual int getCardInstance() const      {   return _cardInstance;      }

  void freeMixDevices();

  /**
   * Registers the card for this backend.
   */
  void registerCard(const QString &cardBaseName);

  /**
   * Unregisters the card of this backend.
   */
  void unregisterCard(const QString &cardBaseName);

protected:
  int m_devnum;

  // All controls of this card
  MixSet m_mixDevices;

  /******************************************************************************************
   * Please don't access the next vars from the Mixer class (even though Mixer is a friend).
   * There are proper access methods for them.
   ******************************************************************************************/

  bool m_isOpen;
  // The MixDevice that would qualify best as MasterDevice (according to the taste of
  // the backend developer)
  shared_ptr<MixDevice> m_recommendedMaster;
  // The Mixer is stored her only for one reason:  the backend creates the MixDevice's,
  // and it has shown that it is helpful if the MixDevice's know their corresponding Mixer.
  // KMix lived 10 years without that, but just believe me. It's *really* better, for
  // example, you can put controls of different soundcards in one View. That is very cool!
  // Also the MDW doesn't need to store the Mixer any longer (MDW is a GUI element, so
  // that was 'wrong' anyhow
  Mixer* _mixer;

  QTimer* _pollingTimer;

protected slots:
  virtual void readSetFromHW();

private:
  QTime _fastPollingEndsAt;
  QString m_mixerName;
  int    _cardInstance;
  bool _cardRegistered;

  mutable bool _readSetFromHWforceUpdate;

  QMap<QString,int> m_mixerNums;
};

#endif
