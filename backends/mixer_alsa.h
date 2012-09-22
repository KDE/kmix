//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef MIXER_ALSA_H
#define MIXER_ALSA_H

// QT includes
#include <QList>
#include <QHash>

// Forward QT includes
class QString;
class QSocketNotifier;

#include "mixer_backend.h"

extern "C"
{
   #include <alsa/asoundlib.h>
}

class Mixer_ALSA : public Mixer_Backend
{
public:
    explicit Mixer_ALSA(Mixer *mixer, int device = -1 );
    ~Mixer_ALSA();

    virtual int  readVolumeFromHW( const QString& id, shared_ptr<MixDevice> md );
    virtual int  writeVolumeToHW ( const QString& id, shared_ptr<MixDevice> md );
    virtual void setEnumIdHW( const QString& id, unsigned int);
    virtual unsigned int enumIdHW(const QString& id);
    virtual bool prepareUpdateFromHW();

    virtual bool needsPolling() { return false; }
    virtual QString getDriverName();

protected:
    virtual int open();
    virtual int close();
    int id2num(const QString& id);

private:

    int openAlsaDevice(const QString& devName);
    void addEnumerated(snd_mixer_elem_t *elem, QList<QString*>&);
    Volume* addVolume(snd_mixer_elem_t *elem, bool capture);
    int setupAlsaPolling();
    void deinitAlsaPolling();

    virtual bool isRecsrcHW( const QString& id );
    int identify( snd_mixer_selem_id_t *sid );
    snd_mixer_elem_t* getMixerElem(int devnum);

    virtual QString errorText(int mixer_error);
    typedef QList<snd_mixer_selem_id_t *>AlsaMixerSidList;
    AlsaMixerSidList mixer_sid_list;
    typedef QList<snd_mixer_elem_t *> AlsaMixerElemList;
    AlsaMixerElemList mixer_elem_list;
    typedef QHash<QString,int> Id2numHash;
    Id2numHash m_id2numHash;

    bool _initialUpdate;
    snd_mixer_t* _handle;
    snd_ctl_t* ctl_handle;

    QString devName;
    struct pollfd  *m_fds;
    QList<QSocketNotifier*> m_sns;
    //int m_count;
};

#endif
