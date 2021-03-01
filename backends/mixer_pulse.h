/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 2008 Helio Chissini de Castro <helio@kde.org>
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

#ifndef MIXER_PULSE_H
#define MIXER_PULSE_H

#include "mixer_backend.h"
#include <pulse/pulseaudio.h>

struct QtPaMainLoop;

typedef QMap<uint8_t,Volume::ChannelID> chanIDMap;
typedef struct {
    int index;
    int device_index;
    QString name;
    QString description;
    QString icon_name;
    pa_cvolume volume;
    pa_channel_map channel_map;
    bool mute;
    QString stream_restore_rule;

    Volume::ChannelMask chanMask;
    chanIDMap chanIDs;
    unsigned int priority;
} devinfo;
typedef QMap<int,devinfo> devmap;

class Mixer_PULSE : public Mixer_Backend
{
    Q_OBJECT

    public:
        Mixer_PULSE(Mixer *mixer, int devnum);
        virtual ~Mixer_PULSE();

        int readVolumeFromHW( const QString& id, shared_ptr<MixDevice> ) override;
        int writeVolumeToHW ( const QString& id, shared_ptr<MixDevice> ) override;

        QString currentStreamDevice(const QString &id) const override;
        bool moveStream( const QString& id, const QString& destId ) override;

        QString getDriverName() override;
        QString getId() const override { return _id; }

        bool needsPolling() override { return false; }

        // Only used internally, but need to be able to be called by
        // static PulseAudio callback functions.
        void triggerUpdate();
        void addWidget(int index, bool = false);
        void removeWidget(int index);
        void removeAllWidgets();
        MixSet *getMixSet() { return &m_mixDevices; }
        int id2num(const QString& id);

    protected:
        int open() override;
        int close() override;

        int fd;
        QString _id;
        std::unique_ptr<QtPaMainLoop> m_mainloop;

    private:
        bool addDevice(devinfo& dev, bool isAppStream = false);
        bool connectToDaemon();
        void emitControlsReconfigured();
        void updateRecommendedMaster(devmap* map);

   protected slots:
        void pulseControlsReconfigured(QString mixerId);
        void pulseControlsReconfigured();

public:
        void reinit() override;

};

#endif
