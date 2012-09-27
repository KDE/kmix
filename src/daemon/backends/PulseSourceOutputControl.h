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

#ifndef PULSESOURCEOUTPUTCONTROL_H
#define PULSESOURCEOUTPUTCONTROL_H

#include "PulseControl.h"

namespace Backends {

class PulseSourceOutputControl : public PulseControl {
    Q_OBJECT
public:
    PulseSourceOutputControl(pa_context *cxt, const pa_source_output_info *info, PulseAudio *parent);
    void setVolume(Channel c, int v);
    void setMute(bool yes);
    void update(const pa_source_output_info *info);
private:
    void cb_client_info(pa_context *cxt, const pa_client_info *info, int eol, void *userdata);
};

}

#endif // PULSESOURCEOUTPUTCONTROL_H
