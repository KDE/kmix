#ifndef ALSACONTROL_H
#define ALSACONTROL_H

#include "Control.h"
#include <alsa/asoundlib.h>

namespace Backends {

class AlsaControl : public Control {
    Q_OBJECT
public:
    AlsaControl(Category category, snd_mixer_elem_t *elem, QObject *parent = 0);
    ~AlsaControl();
    QString displayName() const;
    QString iconName() const;
    int channels() const;
    int getVolume(Channel c) const;
    void setVolume(Channel c, int v);
    bool isMuted() const;
    void setMute(bool yes);
    bool canMute() const;
private:
    snd_mixer_selem_channel_id_t alsaChannel(Channel c) const;
    snd_mixer_elem_t *m_elem;
};

}

#endif // ALSACONTROL_H
