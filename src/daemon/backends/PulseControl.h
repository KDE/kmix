/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) Trever Fischer <tdfischer@fedoraproject.org>
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

#ifndef PULSECONTROL_H
#define PULSECONTROL_H
#include "Control.h"
#include <pulse/pulseaudio.h>
#include <QtCore/QMap>

namespace Backends {

class PulseAudio;

class PulseControl : public Control {
    Q_OBJECT
public:
    PulseControl(Category category, pa_context *cxt, PulseAudio *parent = 0);
    PulseAudio *backend() const;
    QString displayName() const;
    QString iconName() const;
    int channels() const;
    int getVolume(Channel channel) const;
    bool isMuted() const;
    bool canMute() const;
    bool canMonitor() const;
    int pulseIndex() const;

signals:
    void scheduleRefresh(int index);
protected:
    static void cb_refresh(pa_context *c, int success, void* user_data);
    void notifyVolumeUpdate(Channel c);
    void updateVolumes(const pa_cvolume &volumes);
    void stopMonitor();
    void startMonitor();
    pa_context *m_context;
    int m_idx;
    QString m_displayName;
    QString m_iconName;
    pa_cvolume m_volumes;
    bool m_muted;
    PulseAudio *m_backend;
};

} //namespace Backends

#endif //PULSECONTROL_H
