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

#ifndef ALSACONTROL_H
#define ALSACONTROL_H

#include "Control.h"
#include <alsa/asoundlib.h>

namespace Backends {

class AlsaControl : public Control {
    Q_OBJECT
public:
    AlsaControl(Category category, snd_mixer_elem_t *elem, QObject *parent = 0);
    ~AlsaControl();
    QString displayName() const;
    QString iconName() const;
    int channels() const;
    int getVolume(Channel c) const;
    void setVolume(Channel c, int v);
    bool isMuted() const;
    void setMute(bool yes);
    bool canMute() const;
private:
    snd_mixer_selem_channel_id_t alsaChannel(Channel c) const;
    snd_mixer_elem_t *m_elem;
};

}

#endif // ALSACONTROL_H
