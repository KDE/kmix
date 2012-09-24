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

#include "PulseControl.h"
#include <QtCore/QMap>
#include <QtCore/QDebug>

namespace Backends {

PulseControl::PulseControl(pa_context *cxt, const pa_sink_info *info, QObject *parent)
    : Control(parent)
    , m_context(cxt)
{
    qDebug() << "New control" << displayName() << iconName();
    update(info);
}

QString PulseControl::displayName() const
{
    return m_displayName;
}

QString PulseControl::iconName() const
{
    return m_iconName;
}

int PulseControl::channels() const
{
    return m_volumes.channels;
}

int PulseControl::getVolume(Channel channel) const
{
    return m_volumes.values[(int)channel];
}

void PulseControl::setVolume(Channel c, int v)
{
    m_volumes.values[(int)c] = v;
    if (!pa_context_set_sink_volume_by_index(m_context, m_idx, &m_volumes, NULL, NULL)) {
        qWarning() << "pa_context_set_sink_volume_by_index() failed";
    }
}

bool PulseControl::isMuted() const
{
    return m_muted;
}

void PulseControl::cb_refresh(pa_context *c, int success, void* user_data)
{
    PulseControl *that = static_cast<PulseControl*>(user_data);
    emit that->scheduleRefresh(that->m_idx);
}

void PulseControl::setMute(bool yes)
{
    pa_context_set_sink_mute_by_index(m_context, m_idx, yes, cb_refresh, this);
}

bool PulseControl::canMute() const
{
    return true;
}

void PulseControl::update(const pa_sink_info *info)
{
    m_idx = info->index;
    m_displayName = QString::fromUtf8(info->name);
    m_iconName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_DEVICE_ICON_NAME));
    qDebug() << m_volumes.channels << info->volume.channels;
    if (m_volumes.channels == info->volume.channels) {
        for (int channel = 0;channel < info->volume.channels;channel++) {
            if (m_volumes.values[channel] != info->volume.values[channel]) {
                qDebug() << "Volume on" << channel << "from" << m_volumes.values[channel] << "to" << info->volume.values[channel];
                emit volumeChanged((Channel)channel);
            }
        }
    }
    m_volumes = info->volume;
    m_muted = info->mute;
}

} //namespace Backends
