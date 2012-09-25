#include "PulseSinkControl.h"
#include <QtCore/QDebug>

namespace Backends {

PulseSinkControl::PulseSinkControl(pa_context *cxt, const pa_sink_info *info, QObject *parent)
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
