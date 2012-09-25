#ifndef PULSESOURCEOUTPUTCONTROL_H
#define PULSESOURCEOUTPUTCONTROL_H

#include "PulseControl.h"

namespace Backends {

class PulseSourceOutputControl : public PulseControl {
    Q_OBJECT
public:
    PulseSourceOutputControl(pa_context *cxt, const pa_source_output_info *info, QObject *parent = 0);
    void setVolume(Channel c, int v);
    void setMute(bool yes);
    void update(const pa_source_output_info *info);
private:
    void cb_client_info(pa_context *cxt, const pa_client_info *info, int eol, void *userdata);
};

}

#endif // PULSESOURCEOUTPUTCONTROL_H
