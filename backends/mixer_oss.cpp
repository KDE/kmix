/*
 *              KMix -- KDE's full featured mini mixer
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
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

#include "mixer_oss.h"
#include "core/kmixdevicemanager.h"
#include "core/mixer.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#else
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif
#endif

#include <klazylocalizedstring.h>

#include <qplatformdefs.h>
#include <qtimer.h>
#include <qregularexpression.h>

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

   FreeBSD, on the other hand, has had SOUND_MIXER_NRDEVICES set to 25
   for at least the last 13 years; it doesn't change. In addition,
   deviceNameDevfs() overruns the alphabet and ends up looking for
   silly mixer names like /dev/sound/mixer\ . All the devices above
   SOUND_MIXER_NRDEVICES simply don't work.
 */
#if defined(__FreeBSD__) && defined(SOUND_MIXER_NRDEVICES)
#define MAX_MIXDEVS SOUND_MIXER_NRDEVICES
#else
#define MAX_MIXDEVS 32
#endif

// clang-format off
const KLocalizedString MixerDevNames[MAX_MIXDEVS]={
    kli18n("Volume"),   kli18n("Bass"),       kli18n("Treble"),
    kli18n("Synth"),    kli18n("Pcm"),        kli18n("Speaker"),
    kli18n("Line"),     kli18n("Microphone"), kli18n("CD"),
    kli18n("Mix"),      kli18n("Pcm2"),       kli18n("RecMon"),
    kli18n("IGain"),    kli18n("OGain"),      kli18n("Line1"),
    kli18n("Line2"),    kli18n("Line3"),      kli18n("Digital1"),
    kli18n("Digital2"), kli18n("Digital3"),   kli18n("PhoneIn"),
    kli18n("PhoneOut"), kli18n("Video"),      kli18n("Radio"),
    kli18n("Monitor")
#if MAX_MIXDEVS > 25
    ,
    kli18n("3D-depth"),   kli18n("3D-center"),
    kli18n("unknown"),  kli18n("unknown"),    kli18n("unknown"),
    kli18n("unknown") , kli18n("unused")
#endif
};

const MixDevice::ChannelType MixerChannelTypes[MAX_MIXDEVS] = {
    MixDevice::VOLUME,   MixDevice::BASS,       MixDevice::TREBLE,
    MixDevice::MIDI,     MixDevice::AUDIO,      MixDevice::SPEAKER,
    MixDevice::EXTERNAL, MixDevice::MICROPHONE, MixDevice::CD,
    MixDevice::VOLUME,   MixDevice::AUDIO,      MixDevice::RECMONITOR,
    MixDevice::VOLUME,   MixDevice::RECMONITOR, MixDevice::EXTERNAL,
    MixDevice::EXTERNAL, MixDevice::EXTERNAL,   MixDevice::DIGITAL,
    MixDevice::DIGITAL,  MixDevice::DIGITAL,    MixDevice::EXTERNAL,
    MixDevice::EXTERNAL, MixDevice::VIDEO,      MixDevice::EXTERNAL,
    MixDevice::EXTERNAL
#if MAX_MIXDEVS > 25
    ,
    MixDevice::VOLUME,   MixDevice::VOLUME,
    MixDevice::UNKNOWN,  MixDevice::UNKNOWN,    MixDevice::UNKNOWN,
    MixDevice::UNKNOWN,  MixDevice::UNKNOWN
#endif
};
// clang-format on

MixerBackend *OSS_getMixer(Mixer *mixer, int device)
{
    MixerBackend *l_mixer;
    l_mixer = new Mixer_OSS(mixer, device);
    return l_mixer;
}

Mixer_OSS::Mixer_OSS(Mixer *mixer, int device)
    : MixerBackend(mixer, device)
{
    if (device == -1) {
        m_devnum = 0;
    }
    m_fd = -1; // point to an invalid FD
}

Mixer_OSS::~Mixer_OSS()
{
    close();
}


static QString deviceName(int devnum)
{
    return (QString("/dev/mixer%1").arg(devnum==0 ? "" : QString::number(devnum)));
}


static QString deviceNameDevfs(int devnum)
{
    return (QString("/dev/sound/mixer%1").arg(devnum==0 ? "" : QString::number(devnum)));
}


int Mixer_OSS::open()
{
    QString finalDeviceName;
    finalDeviceName = deviceName(m_devnum);
    qCDebug(KMIX_LOG) << "OSS open() " << finalDeviceName;
    if ((m_fd = QT_OPEN(finalDeviceName.toLatin1().data(), O_RDWR)) < 0) {
        if (errno == EACCES)
            return Mixer::ERR_PERM;
        else {
            finalDeviceName = deviceNameDevfs(m_devnum);
            if ((m_fd = QT_OPEN(finalDeviceName.toLatin1().data(), O_RDWR)) < 0) {
                if (errno == EACCES)
                    return Mixer::ERR_PERM;
                else
                    return Mixer::ERR_OPEN;
            }
        }
    }

    // Register the card now, so that it will have an ID set.  The loop
    // below requires it.  If it is not done here then there will be an
    // "Emergency ID created" message from Mixer::dbusPath() which is
    // called from MixDevice::addToPool().
#if defined(SOUND_MIXER_INFO)
    struct mixer_info l_mix_info;
    if (ioctl(m_fd, SOUND_MIXER_INFO, &l_mix_info) != -1) {
        registerCard(l_mix_info.name);
    } else
#endif
    {
        registerCard("OSS Audio Mixer");
    }

    // Mixer is open. Now define its properties.
    int devmask, recmask, i_recsrc, stereodevs;
    if (ioctl(m_fd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
        return Mixer::ERR_READ;
    if (ioctl(m_fd, SOUND_MIXER_READ_RECMASK, &recmask) == -1)
        return Mixer::ERR_READ;
    if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
        return Mixer::ERR_READ;
    if (ioctl(m_fd, SOUND_MIXER_READ_STEREODEVS, &stereodevs) == -1)
        return Mixer::ERR_READ;

    int idx = 0;
    while (devmask && idx < MAX_MIXDEVS) {
        if (devmask & (1 << idx)) // device active?
        {
            Volume playbackVol(100, 0, true, false);
            playbackVol.addVolumeChannel(VolumeChannel(Volume::LEFT));
            if (stereodevs & (1 << idx))
                playbackVol.addVolumeChannel(VolumeChannel(Volume::RIGHT));

            QString id;
            id.setNum(idx);
            MixDevice *md = new MixDevice(_mixer, id, MixerDevNames[idx].toString(), MixerChannelTypes[idx]);
            md->addPlaybackVolume(playbackVol);

            QByteArray dspName = finalDeviceName.toLocal8Bit();
            // The DSP device with the same number as the mixer device.
            dspName.replace("/mixer", "/dsp");
            md->setHardwareId(dspName);

            // Tutorial: Howto add a simple capture switch
            if (recmask & (1 << idx)) {
                // can be captured => add capture volume, with no capture volume
                Volume captureVol(100, 0, true, true);
                md->addCaptureVolume(captureVol);
            }

            m_mixDevices.append(md->addToPool());
        }
        idx++;
    }

    // MixerBackend::registerCard() was originally called here.
    // This is too late, see above.

    m_isOpen = true;
    return 0;
}

int Mixer_OSS::close()
{
    _pollingTimer->stop();
    m_isOpen = false;
    int l_i_ret = ::close(m_fd);
    closeCommon();
    return l_i_ret;
}


QString Mixer_OSS::errorText(int mixer_error)
{
    QString l_s_errmsg;
    switch (mixer_error) {
    case Mixer::ERR_PERM:
        l_s_errmsg = i18n("kmix: You do not have permission to access the mixer device.\n"
                          "Login as root and do a 'chmod a+rw /dev/mixer*' to allow the access.");
        break;
    case Mixer::ERR_OPEN:
        l_s_errmsg = i18n("kmix: Mixer cannot be found.\n"
                          "Please check that the soundcard is installed and the\n"
                          "soundcard driver is loaded.\n"
                          "On Linux you might need to use 'insmod' to load the driver.\n"
                          "Use 'soundon' when using commercial OSS.");
        break;
    default:
        l_s_errmsg = MixerBackend::errorText(mixer_error);
        break;
    }
    return l_s_errmsg;
}

void print_recsrc(int recsrc)
{
    int i;

    QString msg;
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
        if ((1 << i) & recsrc)
            msg += '+';
        else
            msg += '.';
    }
    qCDebug(KMIX_LOG) << msg;
}

int Mixer_OSS::setRecsrcToOSS(const QString &id, bool on)
{
    int i_recsrc; //, oldrecsrc;
    int devnum = id2num(id);
    if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1) {
        errormsg(Mixer::ERR_READ);
        return Mixer::ERR_READ;
    }

    //    oldrecsrc = i_recsrc = on ?
    //             (i_recsrc | (1 << devnum )) :
    //             (i_recsrc & ~(1 << devnum ));

    // Change status of record source(s)
    if (ioctl(m_fd, SOUND_MIXER_WRITE_RECSRC, &i_recsrc) == -1) {
        errormsg(Mixer::ERR_WRITE);
        // don't return here. It is much better to re-read the capture switch states.
    }

    /* The following if {} patch was submitted by Tim McCormick <tim@pcbsd.org>. */
    /*   Comment (cesken): This patch fixes an issue with mutual exclusive recording sources.
         Actually the kernel soundcard driver *could* "do the right thing" by examining the change
         (old-recsrc XOR new-recsrc), and knowing which sources are mutual exclusive.
         The OSS v3 API docs indicate that the behaviour is undefined for this case, and it is not
         clearly documented how and whether SOUND_MIXER_CAP_EXCL_INPUT is evaluated in the OSS driver.
         Evaluating that in the application (KMix) could help, but the patch will work independent
         on whether SOUND_MIXER_CAP_EXCL_INPUT is set or not.

         In any case this patch is a superb workaround for a shortcoming of the OSS v3 API.
     */
    // If the record source is supposed to be on, but wasn't set, explicitly
    // set the record source. Not all cards support multiple record sources.
    // As a result, we also need to do the read & write again.
    if (((i_recsrc & (1 << devnum)) == 0) && on) {
        // Setting the new device failed => Try to enable it *exclusively*

        //       oldrecsrc = i_recsrc = 1 << devnum;

        if (ioctl(m_fd, SOUND_MIXER_WRITE_RECSRC, &i_recsrc) == -1)
            errormsg(Mixer::ERR_WRITE);
        if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &i_recsrc) == -1)
            errormsg(Mixer::ERR_READ);
    }

    // Re-read status of record source(s). Just in case the hardware/driver has
    // some limitation (like exclusive switches)
    int recsrcMask;
    if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &recsrcMask) == -1)
        errormsg(Mixer::ERR_READ);
    else {
        for (int i = 0; i < m_mixDevices.count(); i++) {
            shared_ptr<MixDevice> md = m_mixDevices[i];
            bool isRecsrc = ((recsrcMask & (1 << devnum)) != 0);
            md->setRecSource(isRecsrc);
        } // for all controls
    } // reading newrecsrcmask is OK

    return Mixer::OK;
}

int Mixer_OSS::id2num(const QString &id)
{
    return id.toInt();
}

/**
 * Prints out a translated error text for the given error number on stderr
 */
void Mixer_OSS::errormsg(int mixer_error)
{
    QString l_s_errText;
    l_s_errText = errorText(mixer_error);
    qCCritical(KMIX_LOG) << l_s_errText << "\n";
}

int Mixer_OSS::readVolumeFromHW(const QString &id, shared_ptr<MixDevice> md)
{
    int ret = 0;

    // --- VOLUME ---
    Volume &vol = md->playbackVolume();
    int devnum = id2num(id);

    bool controlChanged = false;

    if (vol.hasVolume()) {
        int volume;
        if (ioctl(m_fd, MIXER_READ(devnum), &volume) == -1) {
            /* Oops, can't read mixer */
            errormsg(Mixer::ERR_READ);
            ret = Mixer::ERR_READ;
        } else {

            int volLeft = (volume & 0x7f);
            int volRight = ((volume >> 8) & 0x7f);
            //
            //			if ( md->id() == "0" )
            //				qCDebug(KMIX_LOG) << md->id() << ": " << "volLeft=" << volLeft << ", volRight" << volRight;

            bool isMuted = volLeft == 0 && (vol.count() < 2 || volRight == 0); // muted is "left and right muted" or "left muted when mono"
            md->setMuted(isMuted);
            if (!isMuted) {
                // Muted is represented in OSS by value 0. We don't want to write the value 0 as a volume,
                // but instead we only mark it muted (see setMuted() above).

                for (const VolumeChannel &vc : std::as_const(vol.getVolumes()))
                {
                    long volOld = 0;
                    long volNew = 0;
                    switch (vc.chid) {
                    case Volume::LEFT:
                        volOld = vol.getVolume(Volume::LEFT);
                        volNew = volLeft;
                        vol.setVolume(Volume::LEFT, volNew);
                        break;
                    case Volume::RIGHT:
                        volOld = vol.getVolume(Volume::RIGHT);
                        volNew = volRight;
                        vol.setVolume(Volume::RIGHT, volNew);
                        break;
                    default:
                        // not supported by OSSv3
                        break;
                    }

                    if (volOld != volNew) {
                        controlChanged = true;
                        // if ( md->id() == "0" ) qCDebug(KMIX_LOG) << "changed";
                    }
                } // for
            } // muted
        }
    }

    // --- RECORD SWITCH ---
    // Volume& captureVol = md->captureVolume();
    int recsrcMask;
    if (ioctl(m_fd, SOUND_MIXER_READ_RECSRC, &recsrcMask) == -1)
        ret = Mixer::ERR_READ;
    else {
        bool isRecsrcOld = md->isRecSource();
        // test if device bit is set in record bit mask
        bool isRecsrc = ((recsrcMask & (1 << devnum)) != 0);
        md->setRecSource(isRecsrc);
        if (isRecsrcOld != isRecsrc)
            controlChanged = true;
    }

    if (ret == 0) {
        if (controlChanged) {
            // qCDebug(KMIX_LOG) << "FINE! " << ret;
            return Mixer::OK;
        } else {
            return Mixer::OK_UNCHANGED;
        }
    } else {
        // qCDebug(KMIX_LOG) << "SHIT! " << ret;
        return ret;
    }
}

int Mixer_OSS::writeVolumeToHW(const QString &id, shared_ptr<MixDevice> md)
{
    int volume;
    int devnum = id2num(id);

    Volume &vol = md->playbackVolume();
    if (md->isMuted())
        volume = 0;
    else {
        if (vol.getVolumes().count() > 1)
            volume = (vol.getVolume(Volume::LEFT) + (vol.getVolume(Volume::RIGHT) << 8));
        else
            volume = vol.getVolume(Volume::LEFT);
    }

    if (ioctl(m_fd, MIXER_WRITE(devnum), &volume) == -1)
        return Mixer::ERR_WRITE;

    setRecsrcToOSS(id, md->isRecSource());

    return 0;
}


const char *OSS_driverName = "OSS";

QString Mixer_OSS::getDriverName()
{
    return (OSS_driverName);
}


int OSS_acceptsHotplugId(const QString &id)
{
    // The Solid device UDIs for a plugged OSS sound card are of the form:
    //
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:12.1/usb4/4-2/4-2:1.0/sound/card4
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:12.1/usb4/4-2/4-2:1.0/sound/card4/mixer4
    //   /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:12.1/usb4/4-2/4-2:1.0/sound/card4/pcm4
    //
    // The "mixer" one of these is taken as the canonical form.  The "card" and
    // "mixer" numbers are assumed to be the same, but this is not checked.

    const QRegularExpression rx("/card(\\d+)/mixer(\\d+)$");
    const QRegularExpressionMatch match = rx.match(id);	// match sound card mixer device
    if (!match.hasCaptured(2)) return (-1);		// UDI not recognised
    return (match.captured(2).toInt());			// assume conversion succeeds
}


bool OSS_acceptsDeviceNode(const QString &blkdev, int devnum)
{
    // The primary OSS device is "/dev/mixer[N]" which corresponds to
    // the "mixer" UDI as above.
    return (blkdev==deviceName(devnum) || blkdev==deviceNameDevfs(devnum));
}
