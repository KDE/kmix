#include "PulseSinkInputControl.h"

#include <QtCore/QDebug>

namespace Backends {

PulseSinkInputControl::PulseSinkInputControl(pa_context *cxt, const pa_sink_input_info *info, QObject *parent)
    : PulseControl(Control::OutputStream, cxt, parent)
{
    update(info);
}

void PulseSinkInputControl::setVolume(Channel c, int v)
{
    m_volumes.values[(int)c] = v;
    if (!pa_context_set_sink_input_volume(m_context, m_idx, &m_volumes, NULL, NULL)) {
        qWarning() << "pa_context_set_sink_input_volume() failed";
    }
}

void PulseSinkInputControl::setMute(bool yes)
{
    pa_context_set_sink_input_mute(m_context, m_idx, yes, cb_refresh, this);
}

void PulseSinkInputControl::update(const pa_sink_input_info *info)
{
    m_idx = info->index;
    m_displayName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME));
    m_iconName = QString::fromUtf8(pa_proplist_gets(info->proplist, PA_PROP_WINDOW_ICON_NAME));
    updateVolumes(info->volume);
    if (m_muted != info->mute) {
        emit muteChanged(info->mute);
    }
    m_muted = info->mute;
}

}

#include "PulseSinkInputControl.moc"
