/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2010 Christian Esken <esken@kde.org>
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

#include "core/mixdevicecomposite.h"


const int MixDeviceComposite::m_maxVolume = 10000;

MixDeviceComposite::MixDeviceComposite(Mixer* mixer,
                                       const QString& id,
                                       QList<shared_ptr<MixDevice> >& mds,
                                       const QString& name,
                                       ChannelType type)
    : MixDevice(mixer, id, name, type)  // this will use doNotRestore == true
{
    setArtificial(true);
    m_compositePlaybackVolume = new Volume(MixDeviceComposite::m_maxVolume, 0, true, false);
    m_compositePlaybackVolume->addVolumeChannel(Volume::LEFT);
    m_compositePlaybackVolume->addVolumeChannel(Volume::RIGHT);

    QListIterator<shared_ptr<MixDevice> > it(mds);
    while (it.hasNext()) {
        shared_ptr<MixDevice> md = it.next();
        m_mds.append(md);
    }
}


MixDeviceComposite::~MixDeviceComposite()
{
    while (!m_mds.empty()) {
        m_mds.removeAt(0);
    }
    delete m_compositePlaybackVolume;
//    delete _compositeCaptureVolume;
}


Volume& MixDeviceComposite::playbackVolume()
{
    return *m_compositePlaybackVolume;
}

// Volume& MixDeviceComposite::captureVolume()
// {
//     return *_compositeCaptureVolume;
// }


void MixDeviceComposite::update()
{
    int volAvg;
    volAvg = calculateVolume(Volume::PlaybackVT);
    m_compositePlaybackVolume->setAllVolumes(volAvg);
    volAvg = calculateVolume(Volume::CaptureVT);
//     _compositeCaptureVolume->setAllVolumes(volAvg);
}

int MixDeviceComposite::calculateVolume(Volume::VolumeType vt)
{
    QListIterator<shared_ptr<MixDevice> > it(m_mds);
    int volSum = 0;
    int volCount = 0;
    while (it.hasNext()) {
        shared_ptr<MixDevice> md = it.next();

        Volume& vol = vt == Volume::CaptureVT ? md->captureVolume() : md->playbackVolume();
        if (vol.hasVolume() && (vol.maxVolume() != 0)) {
            qreal normalizedVolume =
                      (vol.getAvgVolume(Volume::MALL) * MixDeviceComposite::m_maxVolume)
                      / vol.maxVolume();
            volSum += normalizedVolume;
            ++ volCount;
        }
    }
    if (volCount == 0)
        return 0;
    else
        return (volSum / volCount);
}


bool MixDeviceComposite::isMuted()
{
    bool isMuted = false;
    QListIterator<shared_ptr<MixDevice> > it(m_mds);
    while (it.hasNext()) {
        shared_ptr<MixDevice> md = it.next();
        isMuted |= md->isMuted();
        if ( isMuted )
            break;  // Enough. It can't get more true :-)
    }
    return isMuted;
}


void MixDeviceComposite::setMuted(bool value)
{
    QListIterator<shared_ptr<MixDevice> > it(m_mds);
    while (it.hasNext()) {
        shared_ptr<MixDevice> md = it.next();
        md->setMuted(value);
    }
}

bool MixDeviceComposite::isRecSource()
{
    bool isRecSource = false;
    QListIterator<shared_ptr<MixDevice> > it(m_mds);
    while (it.hasNext()) {
        shared_ptr<MixDevice> md = it.next();
        isRecSource |= md->isRecSource();
        if (isRecSource)
            break;  // Enough. It can't get more true :-)
    }
    return isRecSource;
}


void MixDeviceComposite::setRecSource(bool value)
{
    QListIterator<shared_ptr<MixDevice> > it(m_mds);
    while (it.hasNext()) {
        shared_ptr<MixDevice> md = it.next();
        md->setRecSource(value);
    }
}


bool MixDeviceComposite::isEnum()
{
    bool isEnum = true;
    QListIterator<shared_ptr<MixDevice> > it(m_mds);
    while (it.hasNext()) {
        shared_ptr<MixDevice> md = it.next();
        isEnum &= md->isEnum();
        if (!isEnum)
            break;  // Enough. It can't get more false :-)
    }
    return isEnum;
}

