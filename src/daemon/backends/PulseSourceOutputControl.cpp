#include "PulseSourceOutputControl.h"

#include <QtCore/QDebug>

namespace Backends {

PulseSourceOutputControl::PulseSourceOutputControl(pa_context *cxt, const pa_source_output_info *info, PulseAudio *parent)
    : PulseControl(Control::InputStream, cxt, parent)
{
    update(info);
}

void PulseSourceOutputControl::setVolume(Channel c, int v)
{
    m_volumes.values[(int)c] = v;
    if (!pa_context_set_source_output_volume(m_context, m_idx, &m_volumes, NULL, NULL)) {
        qWarning() << "pa_context_set_source_output_volume() failed";
    }
}

void PulseSourceOutputControl::setMute(bool yes)
{
    pa_context_set_source_output_mute(m_context, m_idx, yes, cb_refresh, this);
}

void PulseSourceOutputControl::cb_client_info(pa_context *cxt, const pa_client_info *info, int eol, void *userdata)
{
}

void PulseSourceOutputControl::update(const pa_source_output_info *info)
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

#include "PulseSourceOutputControl.moc"
