#ifndef ALSABACKEND_H
#define ALSABACKEND_H
#include "Backend.h"

namespace Backends {

class ALSA : public Backend {
    Q_OBJECT
public:
    ALSA(QObject *parent = 0);
    ~ALSA();
    bool open();
};

}

#endif // ALSABACKEND_H
