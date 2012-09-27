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

#include "PulseSourceControl.h"
#include <QtCore/QDebug>
#include "PulseAudio.h"

namespace Backends {

PulseSourceControl::PulseSourceControl(pa_context *cxt, const pa_source_info *info, PulseAudio *parent)
    : PulseControl(Control::HardwareInput, cxt, parent)
    , m_monitor(0)
{
    update(info);
    qDebug() << displayName();
}

void PulseSourceControl::setVolume(Channel c, int v)
{
    m_volumes.values[(int)c] = v;
    if (!pa_context_set_source_volume_by_index(m_context, m_idx, &m_volumes, NULL, NULL)) {
        qWarning() << "pa_context_set_source_volume_by_index() failed";
    }
}

void PulseSourceControl::setMute(bool yes)
{
    pa_context_set_sink_mute_by_index(m_context, m_idx, yes, cb_refresh, this);
}

void PulseSourceControl::update(const pa_source_info *info)
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

bool PulseSourceControl::canMonitor() const
{
    return pa_context_get_server_protocol_version(m_context) >= 13;
}

void PulseSourceControl::startMonitor()
{
    qDebug() << "Starting monitor?";
    if (m_monitor)
        return;
    qDebug() << "k.";
    pa_stream *s;
    pa_buffer_attr attr;
    pa_sample_spec spec;

    spec.channels = 1;
    spec.format = PA_SAMPLE_FLOAT32;
    spec.rate = 2;

    memset(&attr, 0, sizeof(attr));
    attr.fragsize = sizeof(float);
    attr.maxlength = (uint32_t) - 1;

    if (!(s = pa_stream_new(m_context, "Peak Detect", &spec, NULL))) {
        qWarning() << "Couldn't create peak detect stream?!";
        return;
    }

    pa_stream_set_read_callback(s, monitor_cb, this);
    if (pa_stream_connect_record(s, NULL, &attr, (pa_stream_flags_t) (PA_STREAM_DONT_INHIBIT_AUTO_SUSPEND|PA_STREAM_DONT_MOVE|PA_STREAM_PEAK_DETECT|PA_STREAM_ADJUST_LATENCY)) < 0) {
        qWarning() << "Couldn't connect monitor stream?!";
        pa_stream_unref(s);
        return;
    }

    m_monitor = s;
}

void PulseSourceControl::monitor_cb(pa_stream *s, size_t length, void *user_data)
{
    PulseSourceControl *that = static_cast<PulseSourceControl*>(user_data);
    const void *data;
    double v;
    if (pa_stream_peek(s, &data, &length) < 0) {
        qWarning() << "Couldn't read monitor stream?!";
        return;
    }

    assert(length > 0);
    assert(length % sizeof(float) == 0);

    v = ((const float*) data)[length / sizeof(float) - 1];
    pa_stream_drop(s);

    v = qMax(0.0, qMin(v, 1.0));

    that->levelUpdate(v*65536);
}

void PulseSourceControl::stopMonitor()
{
    qDebug() << "Stopping monitor";
    if (m_monitor)
        pa_stream_unref(m_monitor);
}

}

#include "PulseSourceControl.moc"
