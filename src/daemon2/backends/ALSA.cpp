#include "ALSA.h"
#include <alsa/asoundlib.h>
#include <QtCore/QDebug>

namespace Backends {

ALSA::ALSA(QObject *parent)
    : Backend(parent)
{
}

ALSA::~ALSA()
{
}

bool ALSA::open()
{
    int card = -1;
    while (snd_card_next(&card) == 0 && card > -1) {
        int err;
        QString devName;
        devName = QString("hw:%1").arg(card);

        qDebug() << "Card" << devName;
        snd_mixer_t *mixer;
        if ((err = snd_mixer_open(&mixer, 0)) == 0) {
            if ((err = snd_mixer_attach(mixer, devName.toAscii().constData())) == 0) {
                if ((err = snd_mixer_selem_register(mixer, NULL, NULL)) != 0) {
                    qDebug() << "Couldn't register simple controls" << snd_strerror(err);
                    continue;
                }
                if ((err = snd_mixer_load(mixer)) != 0) {
                    qDebug() << "Couldn't load mixer" << snd_strerror(err);
                    continue;
                }
                snd_mixer_elem_t *elem;
                elem = snd_mixer_first_elem(mixer);
                while (elem) {
                    elem = snd_mixer_elem_next(elem);
                }
                snd_mixer_detach(mixer, devName.toAscii().constData());
            } else {
                qDebug() << "Could not attach mixer to" << devName;
            }
            snd_mixer_close(mixer);
        } else {
            qDebug() << "Could not open mixer";
        }
    }
    return true;
}

}
