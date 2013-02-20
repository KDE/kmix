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

#include "PulseSinkInputControl.h"
#include "PulseSinkControl.h"
#include "PulseAudio.h"

#include <QtCore/QDebug>

namespace Backends {

PulseSinkInputControl::PulseSinkInputControl(pa_context *cxt, const pa_sink_input_info *info, PulseSinkControl *initialSink, PulseAudio *parent)
    : PulseControl(Control::OutputStream, cxt, parent)
{
    setCurrentTarget(initialSink);
    update(info);
}

void PulseSinkInputControl::setVolume(Channel c, int v)
{
    m_volumes.values[(int)c] = qMax(0, v);
    if (!pa_context_set_sink_input_volume(m_context, m_idx, &m_volumes, NULL, NULL)) {
        qWarning() << "pa_context_set_sink_input_volume() failed";
    } else {
        notifyVolumeUpdate(c);
    }
}

void PulseSinkInputControl::setMute(bool yes)
{
    pa_context_set_sink_input_mute(m_context, m_idx, yes, cb_refresh, this);
}

void PulseSinkInputControl::update(const pa_sink_input_info *info)
{
    m_idx = info->index;
    m_displayName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_WINDOW_NAME));
    if (m_displayName.isEmpty())
        m_displayName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME));
    if (m_displayName.isEmpty())
        m_displayName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_BINARY));
    m_iconName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_WINDOW_ICON_NAME));
    if (m_iconName.isEmpty())
        m_iconName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_ICON_NAME));
    if (m_iconName.isEmpty())
        m_iconName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_BINARY));
    updateVolumes(info->volume);
    if (m_muted != info->mute) {
        emit muteChanged(info->mute);
    }
    m_muted = info->mute;
    if (info->sink != m_sinkIdx) {
        m_sinkIdx = info->sink;
        setCurrentTarget(backend()->sink(info->sink));
    }
}

void PulseSinkInputControl::changeTarget(Control *t)
{
    PulseSinkControl *sink = dynamic_cast<PulseSinkControl*>(t);
    if (sink) {
        int idx = sink->pulseIndex();
        pa_context_move_sink_input_by_index(m_context, m_idx, idx, cb_refresh, this);
    }
}

}

#include "PulseSinkInputControl.moc"
