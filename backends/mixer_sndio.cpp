/*
 * SPDX-FileCopyrightText: 2024 Rafael Sadowski <rafael@rsadowski.de>
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "mixer_sndio.h"

#include "core/mixer.h"

#include <QLoggingCategory>
#include <klocalizedstring.h>

#include <algorithm>
#include <memory>

#include <poll.h>

const char *Sndio_driverName = "Sndio";

static struct sioctl_hdl *s_hdl;
static int m_nfds;
static std::unique_ptr<struct pollfd[]> m_pollfd;

static QMap<int, MixerSndio *> s_mixers;
static devices serverDevices;
static devices captureDevices;
static devices outputDevices;
static devices captureApps;
static devices outputApps;

MixerBackend *Sndio_getMixer(Mixer *mixer, int devnum)
{
    return new MixerSndio(mixer, devnum);
}

MixerSndio::MixerSndio(Mixer *mixer, int devnum)
    : MixerBackend(mixer, devnum)
    , m_devname(QLatin1String(SIO_DEVANY))
{
    if (devnum == -1)
        m_devnum = 0;
    setupSndio(m_devname);
    s_mixers[m_devnum] = this;
}

MixerSndio::~MixerSndio()
{
    s_mixers.remove(m_devnum);
    m_polling = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
}

QString MixerSndio::getDriverName()
{
    return QString(Sndio_driverName);
}

static std::optional<devices> getDervicesForInstance(int type)
{
    switch (static_cast<SndioDevTyp>(type)) {
    case SndioDevTyp::SERVER:
        return serverDevices;
    case SndioDevTyp::PLAYBACK:
        return outputDevices;
    case SndioDevTyp::CAPTURE:
        return captureDevices;
    case SndioDevTyp::APP_PLAYBACK:
        return outputApps;
    default:
        break;
    }
    Q_ASSERT(0);
    return std::nullopt;
}

/**
 * Helper for performing an operation on the device specified by @p id.
 * Search for the device by name in the specified @p devices map, perform
 * the function @c func on it, and return the result of that.  If the
 * device is not found then do nothing.
 */
static int doForDevice(const QString &id, const devices &devices, shared_ptr<MixDevice> md, int (*func)(const DevInfo &, shared_ptr<MixDevice>))
{
    for (auto &dev : devices) {
        if (QString::number(dev.index) == id)
            return ((*func)(dev, md));
    }

    return Mixer::OK;
}

void MixerSndio::setupSndio(const QString &devname)
{
    if (s_hdl)
        return;

    s_hdl = sioctl_open(devname.toLocal8Bit().data(), SIOCTL_READ | SIOCTL_WRITE, 0);
    if (!s_hdl) {
        qDebug(KMIX_LOG) << devname << ": can't open control device\n";
        return;
    }

    if (!sioctl_ondesc(s_hdl, ondesc, nullptr)) {
        qDebug(KMIX_LOG) << devname << ": can't get device description\n";
        s_hdl = nullptr;
        return;
    }

    if (!sioctl_onval(s_hdl, onctl, nullptr)) {
        qDebug(KMIX_LOG) << devname << ": can't get device description\n";
        s_hdl = nullptr;
        return;
    }

    m_nfds = sioctl_nfds(s_hdl);
    m_pollfd = std::unique_ptr<struct pollfd[]>(new struct pollfd[m_nfds]);

    if (!m_pollfd) {
        qDebug(KMIX_LOG) << "Failed to allocate pollfd array";
        return;
    }

    // Initialize pollfd structure
    sioctl_pollfd(s_hdl, m_pollfd.get(), POLLIN);

    m_polling = true;
    m_pollThread = std::thread(&MixerSndio::pollEvents, this);
}

void MixerSndio::pollEvents()
{
    while (m_polling) {
        int ret = poll(m_pollfd.get(), m_nfds, -1);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            break;
        }

        for (int i = 0; i < m_nfds; ++i) {
            if (m_pollfd && m_pollfd[i].revents != 0) {
                short revents = sioctl_revents(s_hdl, &m_pollfd[i]);
                if (revents & POLLHUP) {
                    qDebug(KMIX_LOG) << "POLLHUP event detected for fd:" << m_pollfd[i].fd;
                    m_polling = false;
                    break;
                }
            }
        }
    }
}

int MixerSndio::open()
{
    // Make sure the GUI layers know we are dynamic so as to always paint us
    // XXX _mixer->setDynamic();
    switch (static_cast<SndioDevTyp>(m_devnum)) {
    case SndioDevTyp::SERVER:
        qDebug(KMIX_LOG) << "DEBUG: SERVER called " << Q_FUNC_INFO;
        _id = "Playback Devices";
        registerCard(i18n("Server Devices"));
        std::for_each(serverDevices.cbegin(), serverDevices.cend(), std::bind(&MixerSndio::addDevice, this, std::placeholders::_1));
        updateRecommendedMaster(serverDevices);
        break;
    case SndioDevTyp::PLAYBACK:
        qDebug(KMIX_LOG) << "DEBUG: PLAYBACK called " << Q_FUNC_INFO;
        _id = "Playback Devices";
        registerCard(i18n("Playback Devices"));
        std::for_each(outputDevices.cbegin(), outputDevices.cend(), std::bind(&MixerSndio::addDevice, this, std::placeholders::_1));
        updateRecommendedMaster(outputDevices);
        break;
    case SndioDevTyp::CAPTURE:
        qDebug(KMIX_LOG) << "DEBUG: CAPTURE called " << Q_FUNC_INFO;
        _id = "Capture Devices";
        registerCard(i18n("Capture Devices"));
        std::for_each(captureDevices.cbegin(), captureDevices.cend(), std::bind(&MixerSndio::addDevice, this, std::placeholders::_1));
        updateRecommendedMaster(captureDevices);
        break;
    case SndioDevTyp::APP_PLAYBACK:
        qDebug(KMIX_LOG) << "DEBUG: called " << Q_FUNC_INFO;
        _id = "Playback Streams";
        registerCard(i18n("Playback Streams"));
        std::for_each(outputApps.cbegin(), outputApps.cend(), std::bind(&MixerSndio::addDevice, this, std::placeholders::_1));
        updateRecommendedMaster(outputApps);
        break;
    default:
        break;
    }
    return 0;
}

void MixerSndio::updateRecommendedMaster(const devices &devs)
{
    if (devs.isEmpty())
        return;

    shared_ptr<MixDevice> res;
    unsigned int prio = 0;

    for (auto &&dev : m_mixDevices) {
        const size_t devprio = devs.value(id2num(dev->id())).index;
        if ((devprio > prio) || !res) {
            prio = devprio;
            res = dev;
        }
    }

    if (res)
        m_recommendedMaster = res;
}

int MixerSndio::close()
{
    closeCommon();
    sioctl_close(s_hdl);
    return 1;
}

int MixerSndio::id2num(const QString &id)
{
    auto it = std::find_if(m_mixDevices.begin(), m_mixDevices.end(),
                           [&id](const auto &device) {
                               return device->id() == id;
                           });

    return (it != m_mixDevices.end()) ? std::distance(m_mixDevices.begin(), it) : -1;
}

bool MixerSndio::hasChangedControls()
{
    return true;
}

int MixerSndio::readVolumeFromHW(const QString &id, shared_ptr<MixDevice> md)
{

    auto odevs = getDervicesForInstance(m_devnum);
    if (odevs.has_value()) {
        devices devs = *odevs;
        return (doForDevice(id, devs, md, [](const DevInfo &dev, shared_ptr<MixDevice> md) -> int {
            md->playbackVolume().setAllVolumes(dev.curval);
            // TODO mute ...
            return Mixer::OK;
        }));
    }
    // bool updated = hasChangedControls();
    return Mixer::OK_UNCHANGED;
}
int MixerSndio::writeVolumeToHW(const QString &id, shared_ptr<MixDevice> md)
{
    auto odevs = getDervicesForInstance(m_devnum);
    if (odevs.has_value()) {
        devices devs = *odevs;
        return (doForDevice(id, devs, md, [](const DevInfo &dev, shared_ptr<MixDevice> md) -> int {
            const auto val = md->playbackVolume().getVolume(Volume::ChannelID::LEFT);
            sioctl_setval(s_hdl, dev.device_index, val);
            return (0);
        }));
    }
    // bool updated = hasChangedControls();
    return 0;
}

/*
 * register a new knob/button, called from the poll() loop.  this may be
 * called when label string changes, in which case we update the
 * existing label widget rather than inserting a new one.
 */
void MixerSndio::ondesc(void *, struct sioctl_desc *d, int curval)
{
    auto devExists = [](const devices &devs, const DevInfo &dev) {
        return std::any_of(devs.cbegin(), devs.cend(), [&dev](const DevInfo &existingDev) { return existingDev == dev; });
    };

    if (d == NULL)
        return;

    switch (d->type) {
    case SIOCTL_NUM:
    case SIOCTL_SW:
        // Support
        break;
    case SIOCTL_VEC:
    case SIOCTL_LIST:
    case SIOCTL_SEL:
        // Not yet supported
        return;
    default:
        return;
    }
    // auto dev = std::shared_ptr<DevInfo>(new DevInfo(d));
    auto dev = DevInfo(d, curval);

    switch (dev.devType) {
    case SndioDevTyp::SERVER:
        if (!devExists(serverDevices, dev)) {
            qDebug(KMIX_LOG) << ": Add Server\n";
            dev.print();
            serverDevices[dev.index] = dev;
        }
        break;
    case SndioDevTyp::PLAYBACK:
        if (!devExists(outputDevices, dev)) {
            qDebug(KMIX_LOG) << ": Add playback\n";
            dev.print();
            outputDevices[dev.index] = dev;
        }
        break;
    case SndioDevTyp::CAPTURE:
        if (!devExists(captureDevices, dev)) {
            qDebug(KMIX_LOG) << ": Add capture\n";
            dev.print();
            captureDevices[dev.index] = dev;
        }
        break;
    case SndioDevTyp::APP_CAPTURE:
        if (!devExists(captureApps, dev)) {
            qDebug(KMIX_LOG) << ": Add app capture\n";
            dev.print();
            captureApps[dev.index] = dev;
        }
        break;
    case SndioDevTyp::APP_PLAYBACK:
        if (!devExists(outputApps, dev)) {
            qDebug(KMIX_LOG) << ": Add appp playback\n";
            dev.print();
            outputApps[dev.index] = dev;
        }
        break;
    default:
        break;
    }
}

/*
 * update a knob/button state, called from the poll() loop
 */
void MixerSndio::onctl(void *, unsigned addr, unsigned val)
{
    auto updateDevInfoByAddr = [&addr, &val](devices &devs) {
        auto dev = std::find_if(devs.begin(), devs.end(), [&addr](const auto &d) -> bool { return (d.device_index == static_cast<int>(addr)); });
        if (dev != devs.end()) {
            dev.value().curval = val;
            // TODO mute
        }
    };
    updateDevInfoByAddr(outputDevices);
}

bool MixerSndio::addDevice(const DevInfo &dev)
{
    if (dev.isMuteable())
        return true;

    MixDevice *md = nullptr;

    switch (dev.devType) {
    case SndioDevTyp::SERVER:
        break;
    case SndioDevTyp::PLAYBACK: {
        md = new MixDevice(_mixer, dev.getName(), dev.getDescription(), dev.getIcon());
        Volume vol(dev.maxval, 0 /* min vol */, true, false);
        // This may not be correct but we have no further information
        // about the channels
        vol.addVolumeChannels(Volume::ChannelMask(Volume::MLEFT | Volume::MRIGHT));
        vol.setAllVolumes(dev.curval);
        md->addPlaybackVolume(vol);
        md->setMuted(false);
    } break;
    case SndioDevTyp::CAPTURE: {
        md = new MixDevice(_mixer, dev.getName(), dev.getDescription(), dev.getIcon());
        Volume vol(dev.maxval, 0 /* min vol */, true, true);
        // This may not be correct but we have no further information
        // about the channels
        vol.addVolumeChannels(Volume::ChannelMask(Volume::MLEFT | Volume::MRIGHT));
        vol.setAllVolumes(dev.curval);
        md->addCaptureVolume(vol);
        md->setRecSource(true);
    } break;
    case SndioDevTyp::APP_PLAYBACK: {
        md = new MixDevice(_mixer, dev.getName(), dev.getDescription(), dev.getIcon());
        Volume vol(dev.maxval, 0 /* min vol */, true, false);
        // This may not be correct but we have no further information
        // about the channels
        vol.addVolumeChannels(Volume::ChannelMask(Volume::MLEFT | Volume::MRIGHT));
        vol.setAllVolumes(dev.curval);
        md->addPlaybackVolume(vol);
        md->setMuted(false);
    } break;
    default:
        break;
    }
    if (!md)
        return true;

    md->setHardwareId(md->id().toLocal8Bit());

    md->setApplicationStream(dev.devType == SndioDevTyp::APP_PLAYBACK || dev.devType == SndioDevTyp::APP_CAPTURE);

    m_mixDevices.append(md->addToPool());
    return true;
}
