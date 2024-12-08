/*
 * SPDX-FileCopyrightText: 2024 Rafael Sadowski <rafael@rsadowski.de>
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef MIXER_SNDIO_H
#define MIXER_SNDIO_H

#include "mixerbackend.h"

#include <iostream>
#include <sndio.h>
#include <thread>

enum SndioDevTyp : int { PLAYBACK = 0, CAPTURE = 1, APP_PLAYBACK = 2, APP_CAPTURE = 3, SERVER = 4 };

struct DevInfo
{
    DevInfo() = default;
    DevInfo(sioctl_desc *d, int curval)
        : index(d->addr)
        , device_index(d->addr)
        , name(d->display)
        , description(d->display)
        , icon_name(QLatin1String("dialog-information"))
        , type(d->type)
        , func(d->func)
        , group(d->group)
        , node0(d->node0)
        , node1(d->node1)
        , maxval(d->maxval)
        , curval(curval)
    {
        if (!group.isEmpty() && group.contains({"app"}))
            devType = SndioDevTyp::APP_PLAYBACK;
        else if (group.isEmpty() && QString(node0.name).contains({"output"}))
            devType = SndioDevTyp::PLAYBACK;
        else if (group.isEmpty() && QString(node0.name).contains({"input"}))
            devType = SndioDevTyp::CAPTURE;
        else if (!name.isEmpty() && QString(node0.name).contains({"server"}))
            devType = SndioDevTyp::SERVER;
    }

    bool isMuteable() const
    {
        return func.contains("mute");
    }

    QString getName() const
    {
        return QString::number(index);
    }

    QString getDescription() const
    {
        return description.isEmpty() ? node0.name : "default";
    }

    QString getIcon() const
    {
        return icon_name;
    }

    bool operator==(const DevInfo &dev) const
    {
        return dev.group == group && dev.func == func && std::strcmp(dev.node0.name, node0.name) == 0;
    }

    void print()
    {
        std::cout << " index: \"" << index << "\""
                  << " device_index: \"" << device_index << "\""
                  << " name: \"" << name.toLatin1().constData() << "\""
                  << " description: \"" << description.toLatin1().constData() << "\""
                  << " addr: \"" << addr << "\""
                  << " type: \"" << type << "\""
                  << " func: \"" << func.toLatin1().constData() << "\""
                  << " group: \"" << group.toLatin1().constData() << "\""
                  << " node0: \"" << node0.name << "\""
                  << " node1: \"" << node1.name << "\""
                  << " maxval: \"" << maxval << "\""
                  << " curval: \"" << curval << "\"" << std::endl;
    }

    SndioDevTyp devType;
    int index;
    int device_index;
    QString name;
    QString description;
    QString icon_name;

    /* control address */
    unsigned int addr;

    /* one of above */
    unsigned int type;

    /* function name, ex. "level" */
    QString func;

    /* group this control belongs to */
    QString group;

    /* affected node */
    struct sioctl_node node0;

    /* dito for SIOCTL_{VEC,LIST,SEL} */
    struct sioctl_node node1;

    /* max value */
    unsigned int maxval = 0;

    /* current val */
    int curval = 0;
};
using TDevInfo = std::shared_ptr<DevInfo>;
using devices = QMap<int, DevInfo>;

class MixerSndio : public MixerBackend
{
    Q_OBJECT

public:
    MixerSndio(Mixer *mixer, int devnum);
    virtual ~MixerSndio();

    QString getDriverName() override;

    int readVolumeFromHW(const QString &id, shared_ptr<MixDevice>) override;

    int writeVolumeToHW(const QString &id, shared_ptr<MixDevice>) override;

    virtual bool hasChangedControls() override;

protected:
    int open() override;
    int close() override;
    bool needsPolling() override { return false; }

private:
    static void ondesc(void *, struct sioctl_desc *, int);
    static void onctl(void *, unsigned, unsigned);

    void setupSndio(const QString &);
    void pollEvents();

    bool addDevice(const DevInfo &);
    void updateRecommendedMaster(const devices &map);
    int id2num(const QString &id);
    devices m_devs;
    int fd;
    QString _id;
    QString m_devname;
    bool m_polling = false;
    std::thread m_pollThread;
};

#endif
