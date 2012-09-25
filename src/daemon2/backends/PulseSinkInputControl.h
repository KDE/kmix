#ifndef PULSESINKINPUTCONTROL_H
#define PULSESINKINPUTCONTROL_H

#include "PulseControl.h"

namespace Backends {

class PulseSinkControl;

class PulseSinkInputControl : public PulseControl {
    Q_OBJECT
public:
    PulseSinkInputControl(pa_context *cxt, const pa_sink_input_info *info, PulseSinkControl *initialSink, PulseAudio *parent = 0);
    void setVolume(Channel c, int v);
    void setMute(bool yes);
    void update(const pa_sink_input_info *info);
    void changeTarget(Control *t);
signals:
    void sinkChanged(PulseSinkInputControl *self, int idx);
private:
    int m_sinkIdx;
};

}

#endif // PULSESINKINPUTONTROL_H
