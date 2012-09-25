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

PulseControl::PulseControl(Category category, pa_context *cxt, QObject *parent)
    : Control(category, parent)
    , m_context(cxt)
{
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

bool PulseControl::isMuted() const
{
    return m_muted;
}

void PulseControl::cb_refresh(pa_context *c, int success, void* user_data)
{
    PulseControl *that = static_cast<PulseControl*>(user_data);
    emit that->scheduleRefresh(that->m_idx);
}

bool PulseControl::canMute() const
{
    return true;
}

void PulseControl::updateVolumes(const pa_cvolume &volumes)
{
    if (m_volumes.channels == volumes.channels) {
        for (int channel = 0;channel < volumes.channels;channel++) {
            if (m_volumes.values[channel] != volumes.values[channel]) {
                emit volumeChanged((Channel)channel);
            }
        }
    }
    m_volumes = volumes;
}

} //namespace Backends

#include "PulseControl.moc"