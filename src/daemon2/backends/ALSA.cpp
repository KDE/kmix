#include "ALSA.h"
#include "ALSAControl.h"
#include <QtCore/QDebug>

namespace Backends {

ALSA::ALSA(QObject *parent)
    : Backend(parent)
{
}

ALSA::~ALSA()
{
    foreach(snd_mixer_t *mixer, m_mixers) {
        snd_mixer_close(mixer);
    }
}

bool ALSA::probe()
{
    int card = -1;
    return (snd_card_next(&card) == 0 && card > -1);
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
                    qDebug() << "element" << snd_mixer_selem_get_name(elem);
                    Control *control = new AlsaControl(Control::HardwareOutput, elem, this);
                    registerControl(control);
                    elem = snd_mixer_elem_next(elem);
                }
            } else {
                qDebug() << "Could not attach mixer to" << devName;
            }
            m_mixers << mixer;
        } else {
            qDebug() << "Could not open mixer";
        }
    }
    return true;
}

}
