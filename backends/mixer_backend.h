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
 
#ifndef MIXER_BACKEND_H
#define MIXER_BACKEND_H

#include <QString>
#include <QTime>
#include <QTimer>
#include "core/mixdevice.h"
#include "core/mixset.h"
#include "kmix_debug.h"

class Mixer;


class Mixer_Backend : public QObject
{
      Q_OBJECT

friend class Mixer;

// The Mixer Backend's may only be accessed from the Mixer class.
protected:
  Mixer_Backend(Mixer *mixer, int devnum);
  virtual ~Mixer_Backend();

  /**
   * Derived classes MUST implement this to open the mixer.
   *
   * @return a KMix error code (O=OK).
   */
  virtual int open() = 0;
  /**
   * Derived classes MUST implement this to close the mixer. Do not call this directly, but use shutdown() instead.
   * The method cannot be made pure virtual, as we use close() in the destructor, and C++ does not allow this.
   * https://stackoverflow.com/questions/99552/where-do-pure-virtual-function-call-crashes-come-from?lq=1
   *
   * @return a KMix error code (O=OK).
   */
  virtual int close(); // Not pure virtual. See comment!

  /**
   * Shutdown deinitializes this MixerBackend, freeing resources
   */
  void closeCommon();

  /**
   * Returns the driver name, e.g. "ALSA" or "OSS". This virtual method is for looking up the
   * driver name on instantiated objects.
   *
   * Please note, that there is also a static implementation of the driverName
   * (Because there is no "virtual static" in C++, I need the method twice).
   * The static implementation is for the Mixer Factory (who needs it *before* instantiating an object).
   * While it is not a member function, its implementation can still be found in the corresponding
   * Backend implementation. For example in mixer_oss.cpp there is a global function called OSS_getDriverName().
   */
  virtual QString getDriverName() = 0;

  /**
   * Opens the mixer, if it constitutes a valid Device. You should return "false", when
   * the Mixer with the devnum given in the constructor is not supported by the Backend. The two
   * typical cases are:
   * (1) No such hardware installed
   * (2) The hardware exists, but has no mixer support (e.g. external soundcard with only mechanical volume knobs)
   * The implementation calls open(), checks the return code and whether the number of
   * supported channels is > 0. The device remains opened if it is valid, otherwise a close() is done.
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

  /// Returns translated WhatsThis messages for a control.Translates from
  virtual QString translateKernelToWhatsthis(const QString &kernelName);

   // Return an Universal Device Identification (suitable for the OS, especially for Hotplug and Unplug events)
   virtual QString& udi() { return _udi; };

  int m_devnum;
  /**
   * User friendly name of the Mixer (e.g. "USB 7.1 Surround System"). If your mixer API gives you a usable name, use that name.
   */
  virtual QString getName() const;
  virtual QString getId() const;
  virtual int getCardInstance() const      {   return _cardInstance;      }

  // All controls of this card
  MixSet m_mixDevices;

  /******************************************************************************************
   * Please don't access the next vars from the Mixer class (even though Mixer is a friend).
   * There are proper access methods for them.
   ******************************************************************************************/
  bool m_isOpen;
  // The MixDevice that would qualify best as MasterDevice (according to the taste of the Backend developer)
  shared_ptr<MixDevice> m_recommendedMaster;
   // The Mixer is stored her only for one reason: The backend creates the MixDevice's, and it has shown
   // that it is helpful if the MixDevice's know their corresponding Mixer. KMix lived 10 years without that,
   // but just believe me. It's *really* better, for example, you can put controls of different soundcards in
   // one View. That is very cool! Also the MDW doesn't need to store the Mixer any longer (MDW is a GUI element,
   // so that was 'wrong' anyhow
  Mixer* _mixer;
  QTimer* _pollingTimer;
  QString _udi;  // Universal Device Identification

  mutable bool _readSetFromHWforceUpdate;

signals:
  void controlChanged( void ); // TODO remove?

public slots:
/**
 * Re-initialize. Currently only implemented by PulseAudio backend, and this slot might get moved there
 */
  virtual void reinit() {};

protected:
  void freeMixDevices();

  QMap<QString,int> s_mixerNums;

	/**
	 * Registers the card for this Backend and sets the card discriminator for the given card name.
	 * You MUST call this before creating the first MixDevice. Reason is, that each MixDevice instance register a
	 * DBUS name that includes the mixer ID (and this means also the _cardInstance).
	 *
	 * The discriminator should always be 1, unless a second card with
	 * the same name of a registered card was already registered. Default implementation will return 2, 3 and so on
	 * for more cards. Subclasses can override this and return arbitrary ID's, but any ID that is not 1 will be
	 * displayed to the user everywhere where a mixer name is shown, like in the tab name.
	 *
	 * For the background please see BKO-327471 and read the following info:
	 *   "Count mixer nums for every mixer name to identify mixers with equal names.
	 *    This is for creating persistent (reusable) primary keys, which can safely
	 *    be referenced (especially for config file access, so it is meant to be persistent!)."
	 *
	 *
	 *
	 * @param cardBaseName
	 */
  void registerCard(QString cardBaseName)
  {
		m_mixerName = cardBaseName;
		int cardDiscriminator = 1 + s_mixerNums[cardBaseName];
		qCDebug(KMIX_LOG) << "cardBaseName=" << cardBaseName << ", cardDiscriminator=" << cardDiscriminator;
		_cardInstance = cardDiscriminator;
		_cardRegistered = true;
  }

  /**
   * Unregisters the card of this Backend. The cardDiscriminator counter for this card name is reduced by 1.
   * See #registerCard() for more info.
   *
   * TODO This is not entirely correct. Example: If the first card (cardDiscrimiator == 1) is unpluggged, then
   *   s_mixerNums["cardName"] is changed from 2 to 1. The next plug of registerCard("cardName") will use
   *   cardDiscriminator == 2, but the card with that discriminator was not unplugged => BANG!!!
   *
   * @param cardBaseName
   */
  void unregisterCard(QString cardBaseName)
  {
	  QMap<QString,int>::const_iterator it = s_mixerNums.constFind(cardBaseName);
	  if (it != s_mixerNums.constEnd())
	  {
		  int beforeValue = it.value();
		  int afterValue = beforeValue-1;
		  if (beforeValue > 0)
			  s_mixerNums[cardBaseName] = afterValue;
		  qCDebug(KMIX_LOG) << "beforeValue=" << beforeValue << ", afterValue" << afterValue;
	  }
  }

  int    _cardInstance;
  bool _cardRegistered;


protected slots:
  virtual void readSetFromHW();
private:
  QTime _fastPollingEndsAt;
  QString m_mixerName;
};

typedef Mixer_Backend *getMixerFunc( Mixer* mixer, int device );
typedef QString getDriverNameFunc( );

struct MixerFactory
{
    getMixerFunc *getMixer;
    getDriverNameFunc *getDriverName;
};


#endif
