/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken - esken@kde.org
 *                    2002 Helio Chissini de Castro - helio@conectiva.com.br
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

#include "core/mixer.h"

#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>

#include "backends/mixer_backend.h"
#include "backends/kmix-backends.cpp"
#include "core/volume.h"

/**
 * Some general design hints. Hierachy is Mixer->MixDevice->Volume
 */

QList<Mixer *> Mixer::s_mixers;
MasterControl Mixer::m_globalMasterCurrent;
MasterControl Mixer::m_globalMasterPreferred;
float Mixer::VOLUME_STEP_DIVISOR = 20;
float Mixer::VOLUME_PAGESTEP_DIVISOR = 10;
bool Mixer::m_beepOnVolumeChange = false;

int Mixer::numDrivers()
{
    MixerFactory *factory = g_mixerFactories;
    int num = 0;
    while (factory->getMixer) {
        num ++;
        factory ++;
    }

    return num;
}

/*
 * Returns a reference of the current mixer list.
 */
QList<Mixer *>& Mixer::mixers()
{
    return s_mixers;
}

Mixer::Mixer(QString& ref_driverName, int device)
    : m_balance(0)
    , m_mixerBackend(0L)
    , m_dynamic(false)
{
    m_cardInstance = 0;
    m_mixerBackend = 0;
    int driverCount = numDrivers();
    for (int driver = 0; driver < driverCount; driver++) {
        QString driverName = Mixer::driverName(driver);
        if ( driverName == ref_driverName ) {
            // driver found => retrieve Mixer factory for that driver
            getMixerFunc *f = g_mixerFactories[driver].getMixer;
            if (f) {
                m_mixerBackend = f(this, device);
                readSetFromHWforceUpdate();  // enforce an initial update on first readSetFromHW()
            }
            break;
        }
    }
}


Mixer::~Mixer()
{
    // Close the mixer. This might also free memory, depending on the called backend method
    close();
    delete m_mixerBackend;
}


/*
 * Find a Mixer. If there is no mixer with the given id, 0 is returned
 */
Mixer* Mixer::findMixer(const QString& mixer_id)
{
    Mixer *mixer = 0;
    int mixerCount = Mixer::mixers().count();
    for (int i = 0; i < mixerCount; ++i) {
        if (((Mixer::mixers())[i])->id() == mixer_id) {
            mixer = (Mixer::mixers())[i];
            break;
        }
    }
    return mixer;
}


/**
 * Set the card instance. Usually this will be 1, but if there is
 * more than one card with the same name install, then you need
 * to use 2, 3, ...
 */
void Mixer::setCardInstance(int cardInstance)
{
    m_cardInstance = cardInstance;
    recreateId();
}

int Mixer::getCardInstance() const
{
     return m_cardInstance;
}

void Mixer::recreateId()
{
    /* As we use "::" and ":" as separators, the parts %1,%2 and %3 may not
     * contain it.
     * %1, the driver name is from the KMix backends, it does not contain colons.
     * %2, the mixer name, is typically coming from an OS driver. It could contain colons.
     * %3, the mixer number, is a number: it does not contain colons.
     */
    QString mixerName = getBaseName();
    mixerName.replace(':','_');
    QString primaryKeyOfMixer = QString("%1::%2:%3")
            .arg(getDriverName())
            .arg(mixerName)
            .arg(m_cardInstance);
    // The following 3 replaces are for not messing up the config file
    primaryKeyOfMixer.replace(']','_');
    primaryKeyOfMixer.replace('[','_'); // not strictly necessary, but lets play safe
    primaryKeyOfMixer.replace(' ','_');
    primaryKeyOfMixer.replace('=','_');
    m_id = primaryKeyOfMixer;
}

const QString Mixer::dbusPath()
{
    return "/Mixers/" + QString::number(m_mixerBackend->m_devnum);
}

void Mixer::volumeSave(KConfig *config)
{
    m_mixerBackend->readSetFromHW();
    QString grp("Mixer");
    grp.append(id());
    m_mixerBackend->m_mixDevices.write(config, grp);
}

void Mixer::volumeLoad(KConfig *config)
{
    QString grp("Mixer");
    grp.append(id());
    if (!config->hasGroup(grp)) {
        // no such group. Volumes (of this mixer) were never saved beforehand.
        // Thus don't restore anything (also see Bug #69320 for understanding the real reason)
        return; // make sure to bail out immediately
    }

    // else restore the volumes
    if (!m_mixerBackend->m_mixDevices.read(config, grp)) {
        // Some mixer backends don't support reading the volume into config
        // files, so bail out early if that's the case.
        return;
    }

    // set new settings
    for (int i = 0; i < m_mixerBackend->m_mixDevices.count(); i++) {
        shared_ptr<MixDevice> md = m_mixerBackend->m_mixDevices[i];
        if (!md.get())
            continue;

        m_mixerBackend->writeVolumeToHW(md->id(), md);
        if (md->isEnum())
            m_mixerBackend->setEnumIdHW(md->id(), md->enumId());
    }
}


/**
 * Opens the mixer.
 * Also, starts the polling timer, for polling the Volumes from the Mixer.
 *
 * @return 0, if OK. An Mixer::ERR_ error code otherwise
 */
bool Mixer::openIfValid() {
    bool ok = m_mixerBackend->openIfValid();
    if (ok) {
        recreateId(); // TODO NOW : We actually cannot postpone it to here, due to ControlPool. Move to Mixer !!!. Actually recreateId() is supposed to be called later again, via setCardInstance()
        shared_ptr<MixDevice> recommendedMaster = m_mixerBackend->recommendedMaster();
        if (recommendedMaster.get()) {
            QString recommendedMasterStr = recommendedMaster->id();
            setLocalMasterMD(recommendedMasterStr);
            kDebug(67100) << "Mixer::open() detected master: " << recommendedMaster->id();
        } else {
            if (!m_dynamic)
                kError(67100) << "Mixer::open() no master detected." << endl;
            QString noMaster = "---no-master-detected---";
            setLocalMasterMD(noMaster); // no master
        }
        connect(m_mixerBackend, SIGNAL(controlChanged()), SIGNAL(controlChanged()));
        connect(m_mixerBackend, SIGNAL(controlsReconfigured(QString)), SIGNAL(controlsReconfigured(QString)));

        new DBusMixerWrapper(this, dbusPath());
    }

    return ok;
}

/**
 * Closes the mixer.
 * Also, stops the polling timer.
 *
 * @return 0 (always)
 */
int Mixer::close()
{
    return m_mixerBackend->close();
}


/* ------- WRAPPER METHODS. START ------------------------------ */
unsigned int Mixer::size() const
{
    return m_mixerBackend->m_mixDevices.count();
}

shared_ptr<MixDevice> Mixer::operator[](int num)
{
    shared_ptr<MixDevice> md =  m_mixerBackend->m_mixDevices.at(num);
    return md;
}

MixSet& Mixer::getMixSet()
{
    return m_mixerBackend->m_mixDevices;
}


/**
 * Returns the driver name, that handles this Mixer.
 */
QString Mixer::getDriverName()
{
    QString driverName = m_mixerBackend->getDriverName();
    return driverName;
}

bool Mixer::isOpen() const
{
    if (!m_mixerBackend)
        return false;
    else
        return m_mixerBackend->isOpen();
}

void Mixer::readSetFromHWforceUpdate() const
{
    m_mixerBackend->readSetFromHWforceUpdate();
}

  /// Returns translated WhatsThis messages for a control.Translates from 
QString Mixer::translateKernelToWhatsthis(const QString &kernelName)
{
    return m_mixerBackend->translateKernelToWhatsthis(kernelName);
}

/* ------- WRAPPER METHODS. END -------------------------------- */

void Mixer::setBeepOnVolumeChange(bool beepOnVolumeChange)
{
    m_beepOnVolumeChange = beepOnVolumeChange;
}

bool Mixer::getBeepOnVolumeChange()
{
     return m_beepOnVolumeChange;
}

int Mixer::balance() const {
    return m_balance;
}

void Mixer::setBalance(int balance)
{
    if( balance == m_balance ) {
        // balance unchanged => return
        return;
    }

    m_balance = balance;

    shared_ptr<MixDevice> master = getLocalMasterMD();
    if (!master.get()) {
        // no master device available => return
        return;
    }

    Volume& volP = master->playbackVolume();
    setBalanceInternal(volP);
    Volume& volC = master->captureVolume();
    setBalanceInternal(volC);

    m_mixerBackend->writeVolumeToHW(master->id(), master);
    emit newBalance(volP);
}

void Mixer::setBalanceInternal(Volume& vol)
{
    //_mixerBackend->readVolumeFromHW( master->id(), master );

    int left = vol.getVolume(Volume::LEFT);
    int right = vol.getVolume(Volume::RIGHT);
    int refvol = left > right ? left : right;
    if (m_balance < 0) { // balance left
        vol.setVolume(Volume::LEFT, refvol);
        vol.setVolume(Volume::RIGHT, (m_balance * refvol) / 100 + refvol);
    } else {
        vol.setVolume(Volume::LEFT, -(m_balance * refvol) / 100 + refvol);
        vol.setVolume(Volume::RIGHT, refvol);
    }
}

// should return a name suitable for a human user to read (on a label, ...)
QString Mixer::readableName()
{
    if (m_mixerBackend->m_mixerName.endsWith(":0")) {
        QString finalName = m_mixerBackend->m_mixerName.left(m_mixerBackend->m_mixerName.length() - 2);
        finalName = finalName.append(' ').arg(getCardInstance());
        return finalName;
    } else
        return m_mixerBackend->m_mixerName;
}


QString Mixer::getBaseName()
{
    return m_mixerBackend->m_mixerName;
}

/**
 * Queries the Driver Factory for a driver.
 * @par driver Index number. 0 <= driver < numDrivers()
 */
QString Mixer::driverName(int driver)
{
    getDriverNameFunc *f = g_mixerFactories[driver].getDriverName;
    if (f)
        return f();
    else
        return "unknown";
}

/* obsoleted by setInstance()
void Mixer::setID(QString& ref_id)
{
  _id = ref_id;
}
*/

QString& Mixer::id()
{
    return m_id;
}

QString& Mixer::udi()
{
    return m_mixerBackend->udi();
}

/**
 * Set the global master, which is shown in the dock area and which is accesible via the
 * DBUS masterVolume() method.
 *
 * The parameters are taken over as-is, this means without checking for validity.
 * This allows the User to define a master card that is not always available
 * (e.g. it is an USB hotplugging device). Also you can set the master at any time you
 * like, e.g. after reading the KMix configuration file and before actually constructing
 * the Mixer instances (hint: this method is static!).
 *
 * @param ref_card The card id
 * @param ref_control The control id. The corresponding control must be present in the card.
 * @param preferred Whether this is the preferred master (auto-selected on coldplug and hotplug).
 */
void Mixer::setGlobalMaster(QString ref_card, QString ref_control, bool preferred)
{
    kDebug(67100) << "ref_card =" << ref_card << ", ref_control =" << ref_control << ", preferred =" << preferred;
    m_globalMasterCurrent.set(ref_card, ref_control);
    if (preferred)
        m_globalMasterPreferred.set(ref_card, ref_control);
    kDebug(67100) << "Mixer::setGlobalMaster() card =" <<ref_card<< " control =" << ref_control;
}

Mixer* Mixer::getGlobalMasterMixerNoFalback()
{
    foreach (Mixer* mixer, Mixer::mixers()) {
        if (mixer && mixer->id() == m_globalMasterCurrent.getCard())
            return mixer;
    }
    return 0;
}

Mixer* Mixer::getGlobalMasterMixer()
{
    Mixer *mixer = getGlobalMasterMixerNoFalback();
    if (!mixer && Mixer::mixers().count() > 0) {
        mixer = Mixer::mixers()[0];       // produce fallback
    }
    return mixer;
}


/**
 * Return the preferred global master.
 * If there is no preferred global master, returns the current master instead.
 */
MasterControl& Mixer::getGlobalMasterPreferred()
{
    if ( m_globalMasterPreferred.isValid()) {
        kDebug(67100) << "Returning preferred master";
        return m_globalMasterPreferred;
    } else {
        kDebug(67100) << "Returning current master";
        return m_globalMasterCurrent;
    }
}


shared_ptr<MixDevice> Mixer::getGlobalMasterMD()
{
    return getGlobalMasterMD(true);
}


shared_ptr<MixDevice> Mixer::getGlobalMasterMD(bool fallbackAllowed)
{
    shared_ptr<MixDevice> mdRet;
    Mixer *mixer = fallbackAllowed ?
            Mixer::getGlobalMasterMixer() : Mixer::getGlobalMasterMixerNoFalback();

    if (!mixer)
        return mdRet;

    foreach (shared_ptr<MixDevice> md, mixer->m_mixerBackend->m_mixDevices) {
        if (!md.get())
            continue; // invalid
        if (md->id() == m_globalMasterCurrent.getControl()) {
            mdRet = md;
            break; // found
        }
    }
    if (!mdRet.get())
        kDebug(67100) << "Mixer::masterCardDevice() returns 0 (no globalMaster)";

    return mdRet;
}


shared_ptr<MixDevice> Mixer::getLocalMasterMD()
{
    return find(m_masterDevicePK);
}

void Mixer::setLocalMasterMD(QString &devPK)
{
    m_masterDevicePK = devPK;
}


shared_ptr<MixDevice> Mixer::find(const QString& mixdeviceID)
{
    shared_ptr<MixDevice> mdRet;

    foreach (shared_ptr<MixDevice> md, m_mixerBackend->m_mixDevices) {
        if (!md.get())
            continue; // invalid
        if (md->id() == mixdeviceID) {
            mdRet = md;
            break; // found
        }
    }

    return mdRet;
}


shared_ptr<MixDevice> Mixer::getMixdeviceById(const QString& mixdeviceID)
{
    shared_ptr<MixDevice> md;
    int num = m_mixerBackend->id2num(mixdeviceID);
    if (num != -1 && num < (int)size()) {
        md = (*this)[num];
    }
    return md;
}

/**
   Call this if you have a *reference* to a Volume object and have modified that locally.
   Pass the MixDevice associated to that Volume to this method for writing back
   the changed value to the mixer.
   Hint: Why do we do it this way?
   - It is fast               (no copying of Volume objects required)
   - It is easy to understand ( read - modify - commit )
*/
void Mixer::commitVolumeChange(shared_ptr<MixDevice> md)
{
    m_mixerBackend->writeVolumeToHW(md->id(), md);
    if (md->isEnum())
        m_mixerBackend->setEnumIdHW(md->id(), md->enumId() );
    if (md->captureVolume().hasSwitch()) {
        // Make sure to re-read the hardware, because seting capture might have failed.
        // This is due to exclusive capture groups.
        // If we wouldn't do this, KMix might show a Capture Switch disabled, but
        // in reality the capture switch is still on.
        //
        // We also cannot rely on a notification from the driver (SocketNotifier), because
        // nothing has changed, and so there s nothing to notify.
        m_mixerBackend->readSetFromHWforceUpdate();
        m_mixerBackend->readSetFromHW();
    }
}

// @dbus, used also in kmix app
void Mixer::increaseVolume(const QString& mixdeviceID)
{
    increaseOrDecreaseVolume(mixdeviceID, false);
}

// @dbus
void Mixer::decreaseVolume(const QString& mixdeviceID)
{
    increaseOrDecreaseVolume(mixdeviceID, true);
}

void Mixer::increaseOrDecreaseVolume(const QString& mixdeviceID, bool decrease)
{
    shared_ptr<MixDevice> md = getMixdeviceById(mixdeviceID);
    if (md.get()) {
        Volume& volP = md->playbackVolume();
        if (volP.hasVolume()) {
            int volSpan = volP.volumeSpan();
            double step = volSpan / Mixer::VOLUME_STEP_DIVISOR;
            if (step < 1) step = 1;
            if (decrease) step = -step;
            volP.changeAllVolumes(step);
        }

        Volume& volC = md->captureVolume();
        if (volC.hasVolume()) {
            int volSpan = volC.volumeSpan();
            double step = volSpan / Mixer::VOLUME_STEP_DIVISOR;
            if (step < 1) step = 1;
            if (decrease) step = -step;
            volC.changeAllVolumes(step);
        }

        m_mixerBackend->writeVolumeToHW(mixdeviceID, md);
    }

    /************************************************************
        It is important, not to implement this method like this:
    int vol=volume(mixdeviceID);
    setVolume(mixdeviceID, vol-5);
        It creates too big rounding errors. If you don't believe me, then
        do a decreaseVolume() and increaseVolume() with "vol.maxVolume() == 31".
    ***********************************************************/
}

void Mixer::setDynamic(bool dynamic)
{
    m_dynamic = dynamic;
}

bool Mixer::isDynamic()
{
    return m_dynamic;
}

bool Mixer::moveStream(const QString id, const QString& destId)
{
    // We should really check that id is within our md's....
    return m_mixerBackend->moveStream(id, destId);
}

int Mixer::mediaPlay(QString id)
{
     return m_mixerBackend->mediaPlay(id);
}

int Mixer::mediaPrev(QString id)
{
     return m_mixerBackend->mediaPrev(id);
}

int Mixer::mediaNext(QString id)
{
     return m_mixerBackend->mediaNext(id);
}

#include "mixer.moc"
