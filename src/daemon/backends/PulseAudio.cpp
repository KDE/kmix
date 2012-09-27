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

#include "PulseAudio.h"
#include "PulseControl.h"
#include "PulseSourceOutputControl.h"
#include "PulseSinkInputControl.h"
#include "PulseSinkControl.h"
#include "PulseSourceControl.h"
#include <QtCore/QDebug>

namespace Backends {

PulseAudio::PulseAudio(QObject *parent)
    : Backend(parent)
    , m_context(0)
{
    m_loop = pa_glib_mainloop_new(NULL);
    m_loopAPI = pa_glib_mainloop_get_api(m_loop);
}

PulseAudio::~PulseAudio()
{
    if (m_context) {
        pa_context_unref(m_context);
        m_context = NULL;
    }
    if (m_loop) {
        pa_glib_mainloop_free(m_loop);
        m_loop = NULL;
    }
}

//FIXME: Lots of copypasta code! WTF!!!
void PulseAudio::sink_input_cb(pa_context *cxt, const pa_sink_input_info *info, int eol, gpointer user_data)
{
    PulseAudio *that = static_cast<PulseAudio*>(user_data);
    if (eol < 0) {
        if (pa_context_errno(cxt) == PA_ERR_NOENTITY)
            return;
    }
    if (eol > 0) {
        return;
    }
    PulseSinkInputControl *control;
    if (!that->m_sinkInputs.contains(info->index)) {
        control = new PulseSinkInputControl(cxt, info, that->m_sinks[info->sink], that);
        foreach(PulseSinkControl *sink, that->m_sinks) {
            control->addAlternateTarget(sink);
        }

        QObject::connect(control, SIGNAL(scheduleRefresh(int)), that, SLOT(refreshSinkInput(int)));
        that->m_sinkInputs[info->index] = control;
        that->registerControl(control);
    } else {
        control = that->m_sinkInputs[info->index];
        control->update(info);
    }
}

void PulseAudio::source_cb(pa_context *cxt, const pa_source_info *info, int eol, gpointer user_data)
{
    PulseAudio *that = static_cast<PulseAudio*>(user_data);
    if (eol < 0) {
        if (pa_context_errno(cxt) == PA_ERR_NOENTITY)
            return;
    }
    if (eol > 0) {
        return;
    }
    PulseSourceControl *control;
    if (!that->m_sources.contains(info->index)) {
        control = new PulseSourceControl(cxt, info, that);
        QObject::connect(control, SIGNAL(scheduleRefresh(int)), that, SLOT(refreshSource(int)));
        that->m_sources[info->index] = control;
        that->registerControl(control);
    } else {
        control = that->m_sources[info->index];
        control->update(info);
    }
}

PulseSinkControl *PulseAudio::sink(int idx)
{
    if (m_sinks.contains(idx))
        return m_sinks[idx];
    return 0;
}

void PulseAudio::source_output_cb(pa_context *cxt, const pa_source_output_info *info, int eol, gpointer user_data)
{
    PulseAudio *that = static_cast<PulseAudio*>(user_data);
    if (eol < 0) {
        if (pa_context_errno(cxt) == PA_ERR_NOENTITY)
            return;
    }
    if (eol > 0) {
        return;
    }
    PulseSourceOutputControl *control;
    const char *app;
    if ((app = pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_ID))) {
        qDebug() << "recording App ID:" << app;
        if (strcmp(app, "org.PulseAudio.pavucontrol") == 0)
            return;
        if (strcmp(app, "org.kde.kmixd") == 0)
            return;
        if (strcmp(app, "org.gnome.VolumeControl") == 0)
            return;
    }
    if (!that->m_sourceOutputs.contains(info->index)) {
        control = new PulseSourceOutputControl(cxt, info, that);
        QObject::connect(control, SIGNAL(scheduleRefresh(int)), that, SLOT(refreshSourceOutput(int)));
        that->m_sourceOutputs[info->index] = control;
        that->registerControl(control);
    } else {
        control = that->m_sourceOutputs[info->index];
        control->update(info);
    }
}

void PulseAudio::sink_cb(pa_context *cxt, const pa_sink_info *info, int eol, gpointer user_data)
{
    PulseAudio *that = static_cast<PulseAudio*>(user_data);
    if (eol < 0) {
        if (pa_context_errno(cxt) == PA_ERR_NOENTITY)
            return;
    }
    if (eol > 0) {
        return;
    }
    PulseSinkControl *control;
    if (!that->m_sinks.contains(info->index)) {
        control = new PulseSinkControl(cxt, info, that);
        QObject::connect(control, SIGNAL(scheduleRefresh(int)), that, SLOT(refreshSink(int)));
        that->m_sinks[info->index] = control;
        that->registerControl(control);
    } else {
        control = that->m_sinks[info->index];
        control->update(info);
    }
}

void PulseAudio::refreshSink(int idx)
{
    pa_context_get_sink_info_by_index(m_context, idx, sink_cb, this);
}

void PulseAudio::refreshSinkInput(int idx)
{
    pa_context_get_sink_input_info(m_context, idx, sink_input_cb, this);
}

void PulseAudio::refreshSource(int idx)
{
    pa_context_get_source_info_by_index(m_context, idx, source_cb, this);
}

void PulseAudio::refreshSourceOutput(int idx)
{
    pa_context_get_source_output_info(m_context, idx, source_output_cb, this);
}

void PulseAudio::subscribe_cb(pa_context *cxt, pa_subscription_event_type t, uint32_t index, gpointer user_data)
{
    PulseAudio *that = static_cast<PulseAudio*>(user_data);
    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                PulseControl *control = that->m_sinks.take(index);
                that->deregisterControl(control);
                control->deleteLater();
            } else {
                pa_operation *op;
                if (!(op = pa_context_get_sink_info_by_index(cxt, index, sink_cb, user_data))) {
                    qWarning() << "pa_context_get_sink_info_by_index failed";
                    return;
                }
                pa_operation_unref(op);
            }
            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                PulseControl *control = that->m_sourceOutputs.take(index);
                that->deregisterControl(control);
                that->m_excludedSourceOutputs.removeOne(control->pulseIndex());
                control->deleteLater();
            } else {
                pa_operation *op;
                if (!(op = pa_context_get_source_output_info(cxt, index, source_output_cb, user_data))) {
                    qWarning() << "pa_context_get_source_output_info failed";
                    return;
                }
                pa_operation_unref(op);
            }
            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                PulseControl *control = that->m_sources.take(index);
                that->deregisterControl(control);
                control->deleteLater();
            } else {
                pa_operation *op;
                if (!(op = pa_context_get_source_info_by_index(cxt, index, source_cb, user_data))) {
                    qWarning() << "pa_context_get_source_info_by_index failed";
                    return;
                }
                pa_operation_unref(op);
            }
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                PulseControl *control = that->m_sinkInputs.take(index);
                that->deregisterControl(control);
                control->deleteLater();
            } else {
                pa_operation *op;
                if (!(op = pa_context_get_sink_input_info(cxt, index, sink_input_cb, user_data))) {
                    qWarning() << "pa_context_get_sink_input_info failed";
                    return;
                }
                pa_operation_unref(op);
            }
            break;
    }
}

void PulseAudio::context_state_callback(pa_context *cxt, gpointer user_data)
{
    PulseAudio *that = static_cast<PulseAudio*>(user_data);
    pa_context_state_t state = pa_context_get_state(cxt);
    if (state == PA_CONTEXT_READY) {
        pa_operation *op;
        pa_context_set_subscribe_callback(cxt, subscribe_cb, that);
        if (!(op = pa_context_subscribe(cxt, (pa_subscription_mask_t) (
                PA_SUBSCRIPTION_MASK_SINK |
                PA_SUBSCRIPTION_MASK_SOURCE | 
                PA_SUBSCRIPTION_MASK_CLIENT | 
                PA_SUBSCRIPTION_MASK_SINK_INPUT |
                PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT),
                NULL, NULL))) {
            qWarning() << "pa_context_subscribe failed";
            return;
        }
        pa_operation_unref(op);
        if (!(op = pa_context_get_sink_info_list(cxt, sink_cb, that))) {
            return;
        }
        pa_operation_unref(op);
        if (!(op = pa_context_get_source_info_list(cxt, source_cb, that))) {
            return;
        }
        pa_operation_unref(op);
        if (!(op = pa_context_get_source_output_info_list(cxt, source_output_cb, that))) {
            return;
        }
        pa_operation_unref(op);
        if (!(op = pa_context_get_sink_input_info_list(cxt, sink_input_cb, that))) {
            return;
        }
        pa_operation_unref(op);
    }
}

bool PulseAudio::probe()
{
    bool ret;
    pa_context *context = pa_context_new(m_loopAPI, "KMix");
    ret = (pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, 0));
    pa_context_disconnect(context);
    pa_context_unref(context);
    if (ret == 0)
        return true;
    qDebug() << "Could not connect to pulse:" << pa_strerror(ret);
    return false;
}

bool PulseAudio::open()
{
    int ret;
    if (!m_context) {
        pa_proplist *props = pa_proplist_new();
        pa_proplist_sets(props, PA_PROP_APPLICATION_ID, "org.kde.kmixd");
        pa_proplist_sets(props, PA_PROP_APPLICATION_NAME, "KMix");
        pa_proplist_sets(props, PA_PROP_APPLICATION_ICON_NAME, "kmix");
        //TODO
        //pa_proplist_sets(props, PA_PROP_APPLICATION_VERSION, KMIXD_VERSION);
        m_context = pa_context_new_with_proplist(m_loopAPI, "KMix", props);
    }
    if ((ret = pa_context_connect(m_context, NULL, PA_CONTEXT_NOFAIL, 0)) < 0) {
        qDebug() << "Could not connect to pulse:" << pa_strerror(ret);
        pa_context_unref(m_context);
        m_context = NULL;
        return false;
    }
    pa_context_set_state_callback(m_context, &context_state_callback, this);
    return true;
}

} //namespace Backends
