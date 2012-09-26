#ifndef ALSABACKEND_H
#define ALSABACKEND_H
#include "Backend.h"

#include <alsa/asoundlib.h>

namespace Backends {

class ALSA : public Backend {
    Q_OBJECT
public:
    ALSA(QObject *parent = 0);
    ~ALSA();
    bool open();
private:
    QList<snd_mixer_t*> m_mixers;
};

}

#endif // ALSABACKEND_H
