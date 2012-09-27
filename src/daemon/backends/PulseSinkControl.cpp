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

#include "PulseSinkControl.h"
#include <QtCore/QDebug>
#include "PulseAudio.h"

namespace Backends {

PulseSinkControl::PulseSinkControl(pa_context *cxt, const pa_sink_info *info, PulseAudio *parent)
    : PulseControl(Control::HardwareOutput, cxt, parent)
{
    update(info);
}

void PulseSinkControl::setVolume(Channel c, int v)
{
    m_volumes.values[(int)c] = v;
    if (!pa_context_set_sink_volume_by_index(m_context, m_idx, &m_volumes, NULL, NULL)) {
        qWarning() << "pa_context_set_sink_volume_by_index() failed";
    }
}

void PulseSinkControl::setMute(bool yes)
{
    pa_context_set_sink_mute_by_index(m_context, m_idx, yes, cb_refresh, this);
}

void PulseSinkControl::update(const pa_sink_info *info)
{
    m_idx = info->index;
    m_displayName = QString::fromUtf8(info->description);
    m_iconName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_DEVICE_ICON_NAME));
    updateVolumes(info->volume);
    if (m_muted != info->mute) {
        emit muteChanged(info->mute);
    }
    m_muted = info->mute;
}

}

#include "PulseSinkControl.moc"
