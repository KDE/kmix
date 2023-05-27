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

#ifndef RANDOMPREFIX_MIXER_H
#define RANDOMPREFIX_MIXER_H

#include <QList>
#include <QObject>
#include <QString>

#include "core/volume.h"
#include "backends/mixerbackend.h"
#include "core/MasterControl.h"
#include "mixset.h"
#include "core/mixdevice.h"
#include "dbus/dbusmixerwrapper.h"
#include "kmixcore_export.h"

class Volume;
class KConfig;


/**
  * This class manages a single mixer only, it should not include any
  * static functions or global data.  The global list of mixers and
  * backends is managed in MixerToolBox.
*/

class KMIXCORE_EXPORT Mixer : public QObject
{
      Q_OBJECT

public:
	/**
	 * Status for Mixer operations.
	 * 
	 * OK_UNCHANGED is a special variant of OK. It must be implemented by
	 * backends that use needsPolling() == true. See Mixer_OSS.cpp for an
	 * example. Rationale is that we need a proper change check: Otherwise
	 * the DBUS Session Bus is massively spammed. Also quite likely the Mixer
	 * GUI might get updated all the time.
	 */
    enum MixerError { OK=0, ERR_PERM=1, ERR_WRITE, ERR_READ,
        ERR_OPEN, OK_UNCHANGED };

    Mixer(const QString &driverName, int deviceIndex);
    virtual ~Mixer();

    QString getDriverName() const		{ return (_mixerBackend->getDriverName()); }

    shared_ptr<MixDevice> find(const QString &devPK) const;

    void volumeSave(KConfig *config) const;
    void volumeLoad(const KConfig *config);

    /// How many mixer backend devices
    // TODO: rename to numDevices, belongs with mixDevices below
    unsigned int size() const			{ return (_mixerBackend->m_mixDevices.count()); }

    /// Returns a pointer to the mix device whose type matches the value
    /// given by the parameter and the array MixerDevNames given in
    /// mixer_oss.cpp (0 is Volume, 4 is PCM, etc.)
    shared_ptr<MixDevice> getMixdeviceById( const QString& deviceID ) const;

    /// Open/grab the mixer for further interaction
    bool openIfValid();

    /// Returns whether the card is open/operational
    bool isOpen() const;

    /// Close/release the mixer
    virtual void close();

    /// Reads balance
    int balance() const;

    /// Returns a detailed state message after errors. Only for diagnostic purposes, no i18n.
    QString& stateMessage() const;

    /**
     * Returns the name of the card/chip/hardware, as given by the driver. The name is NOT instance specific,
     * so if you install two identical soundcards, two of them will deliver the same mixerName().
     * Use this method if you need an instance-UNspecific name, e.g. for finding an appropriate
     * mixer layout for this card, or as a prefix for constructing instance specific ID's like in id().
     */
    virtual QString getBaseName() const;

    /// Wrapper to MixerBackend
    QString translateKernelToWhatsthis(const QString &kernelName) const;

    /**
      * Get a name suitable for a human user to read, possibly with quoted ampersand.
      * The latter is required by some GUI elements like QRadioButton or when used as a
      * tab label, as '&' introduces an accelerator there.
      *
      * @param ampersandQuoted @c true if '&' characters are to be quoted
      * @return the readable device name
      */
    QString readableName(bool ampersandQuoted = false) const;

    /**
     * Returns an unique ID of the Mixer. It currently looks like "<soundcard_descr>::<hw_number>:<driver>"
     */
    const QString &id() const			{ return (_id); }

    int getCardInstance() const      		{ return _mixerBackend->getCardInstance(); }
    shared_ptr<MixDevice> recommendedMaster()	{ return (_mixerBackend->recommendedMaster()); }

    /// Returns an Universal Device Identification of the Mixer. This is an ID that relates to the underlying operating system.
    // For OSS and ALSA this is taken from Solid (actually HAL). For Solaris this is just the device name.
    // Examples:
    // ALSA: /org/freedesktop/Hal/devices/usb_device_d8c_1_noserial_if0_sound_card_0_2_alsa_control__1
    // OSS: /org/freedesktop/Hal/devices/usb_device_d8c_1_noserial_if0_sound_card_0_2_oss_mixer__1
    // Solaris: /dev/audio
    const QString &udi() const			{ return _mixerBackend->udi(); }

    // Returns a DBus path for this mixer
    // Used also by MixDevice to bind to this path
    const QString dbusPath();

    QString getRecommendedDeviceId() const;

    /******************************************
    The recommended master of this Mixer.
    ******************************************/
    shared_ptr<MixDevice> getLocalMasterMD() const;
    void setLocalMasterMD(const QString &devPK);

    /**
     * An icon for the mixer's master channel
     */
    QString iconName() const;

    /// get the actual MixSet
    // TODO: rename to mixDevices()
    MixSet &getMixSet() const			{ return (_mixerBackend->m_mixDevices); }

    /// DBUS oriented methods
    virtual void increaseVolume( const QString& mixdeviceID );
    virtual void decreaseVolume( const QString& mixdeviceID );

    /// Says if we are dynamic (e.g. widgets can come and go)
    virtual void setDynamic(bool dynamic = true)	{ m_dynamic = dynamic; }
    virtual bool isDynamic() const			{ return (m_dynamic); }

    virtual bool moveStream(const QString &id, const QString &destId);
    virtual QString currentStreamDevice(const QString &id) const;

    virtual int mediaPlay(QString id)		{ return _mixerBackend->mediaPlay(id); }
    virtual int mediaPrev(QString id)		{ return _mixerBackend->mediaPrev(id); }
    virtual int mediaNext(QString id)		{ return _mixerBackend->mediaNext(id); }

    void commitVolumeChange(shared_ptr<MixDevice> md);

public Q_SLOTS:
    void readSetFromHWforceUpdate() const;

    virtual void setBalance(int balance); // sets the m_balance (see there)
    
Q_SIGNALS:
    void newBalance(Volume& );
    void controlChanged(void); // TODO remove?

private:
    void setBalanceInternal(Volume& vol);
    void recreateId();
    void increaseOrDecreaseVolume( const QString& mixdeviceID, bool decrease );

    MixerBackend *_mixerBackend;
    QString _id;
    QString _masterDevicePK;
    int m_balance; // from -100 (just left) to 100 (just right)
    bool m_dynamic;
};

#endif
