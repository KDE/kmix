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
    , m_info(*info)
{
}

QString PulseControl::displayName() const
{
    return QString::fromUtf8(m_info.name);
}

QString PulseControl::iconName() const
{
    return QString::fromUtf8(pa_proplist_gets(m_info.proplist, PA_PROP_DEVICE_ICON_NAME));
}

QMap<Control::Channel, int> PulseControl::volumes() const
{
    QMap<Control::Channel, int> volumes;
    for (uint8_t i = 0;i<m_info.volume.channels;i++) {
        volumes[(Channel)i] = m_info.volume.values[i];
    }
    return volumes;
}

void PulseControl::setVolume(Channel c, int v)
{
    pa_cvolume volume = m_info.volume;
    volume.values[(int)c] = v;
    if (!pa_context_set_sink_volume_by_index(m_context, m_info.index, &volume, NULL, NULL)) {
        qWarning() << "pa_context_set_sink_volume_by_index() failed";
    }
}

bool PulseControl::isMuted() const
{
    return m_info.mute;
}

void PulseControl::cb_refresh(pa_context *c, int success, void* user_data)
{
    PulseControl *that = static_cast<PulseControl*>(user_data);
    emit that->scheduleRefresh(that->m_info.index);
}

void PulseControl::setMute(bool yes)
{
    qDebug() << m_info.index << m_info.name;
    pa_context_set_sink_mute_by_index(m_context, m_info.index, yes, cb_refresh, this);
}

bool PulseControl::canMute() const
{
    return true;
}

void PulseControl::update(const pa_sink_info *info)
{
    //FIXME: Doesn't work!
    //m_info = info;
    qDebug() << "Mute:" << m_info.mute;
}

} //namespace Backends
