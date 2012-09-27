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
