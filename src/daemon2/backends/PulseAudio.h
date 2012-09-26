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

#include "Backend.h"
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <QtCore/QMap>

namespace Backends {
class PulseControl;
class PulseSinkControl;
class PulseSourceOutputControl;
class PulseSinkInputControl;

class PulseAudio : public Backend {
    Q_OBJECT
public:
    PulseAudio(QObject *parent = 0);
    ~PulseAudio();
    bool open();
    bool probe();
    PulseSinkControl *sink(int idx);
private slots:
    void refreshSink(int idx);
    void refreshSourceOutput(int idx);
    void refreshSinkInput(int idx);
private:
    static void context_state_callback(pa_context *cxt, gpointer user_data);
    static void sink_cb(pa_context *cxt, const pa_sink_info *info, int eol, gpointer user_data);
    static void source_output_cb(pa_context *cxt, const pa_source_output_info *info, int eol, gpointer user_data);
    static void sink_input_cb(pa_context *cxt, const pa_sink_input_info *info, int eol, gpointer user_data);
    static void subscribe_cb(pa_context *cxt, pa_subscription_event_type t, uint32_t index, gpointer user_data);
    pa_glib_mainloop *m_loop;
    pa_mainloop_api *m_loopAPI;
    pa_context *m_context;
    QMap<int, PulseSinkControl*> m_sinks;
    QMap<int, PulseSourceOutputControl*> m_sourceOutputs;
    QMap<int, PulseSinkInputControl*> m_sinkInputs;
};

} //namespace Backends
