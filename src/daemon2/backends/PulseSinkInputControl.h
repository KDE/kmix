#ifndef PULSESINKINPUTCONTROL_H
#define PULSESINKINPUTCONTROL_H

#include "PulseControl.h"

namespace Backends {

class PulseSinkInputControl : public PulseControl {
    Q_OBJECT
public:
    PulseSinkInputControl(pa_context *cxt, const pa_sink_input_info *info, QObject *parent = 0);
    void setVolume(Channel c, int v);
    void setMute(bool yes);
    void update(const pa_sink_input_info *info);
};

}

#endif // PULSESINKINPUTONTROL_H
