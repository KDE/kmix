#ifndef PULSESINKCONTROL_H
#define PULSESINKCONTROL_H

#include "PulseControl.h"

namespace Backends {

class PulseSinkControl : public PulseControl {
    Q_OBJECT
public:
    PulseSinkControl(pa_context *cxt, const pa_sink_info *info, PulseAudio *parent = 0);
    void setVolume(Channel c, int v);
    void setMute(bool yes);
    void update(const pa_sink_info *info);
};

}

#endif // PULSESINKCONTROL_H
