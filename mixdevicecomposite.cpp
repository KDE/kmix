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

#include "mixdevicecomposite.h"




MixDeviceComposite::MixDeviceComposite( Mixer* mixer,  const QString& id, QList<MixDevice*>& mds, const QString& name, ChannelType type ) :
   MixDevice( mixer, id, name, type )
{
    // use a standard init(), but as this is a Composite Control, use doNotRestore == true
    QListIterator<MixDevice*> it(mds);
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        _mds.append(md);
    }
    init(mixer, id, name, "mixer-line", true, 0);
}


MixDeviceComposite::~MixDeviceComposite()
{
    while ( ! _mds.empty() ) {
        _mds.removeAt(0);
    }
}


// @todo: Make sure the composite is updated, when the enclosed  MixDevice's change
//        Recalculating it on each call is highly inefficient
Volume& MixDeviceComposite::playbackVolume()
{
    QListIterator<MixDevice*> it(_mds);
    long volSum = 0;
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        volSum += md->playbackVolume().getAvgVolume(Volume::MALL);
    }
    _compositePlaybackVolume.setAllVolumes(volSum/_mds.count());
    return _compositePlaybackVolume;
}

// @todo: Make sure the composite is updated, when the enclosed  MixDevice's change
//        Recalculating it on each call is highly inefficient
Volume& MixDeviceComposite::captureVolume()
{
    QListIterator<MixDevice*> it(_mds);
    long volSum = 0;
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        volSum += md->captureVolume().getAvgVolume(Volume::MALL);
    }
    _compositeCaptureVolume.setAllVolumes(volSum/_mds.count());
    return _compositeCaptureVolume;
}

bool MixDeviceComposite::isMuted()
{
    bool isMuted = false;
    QListIterator<MixDevice*> it(_mds);
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        isMuted |= md->isMuted();
        if ( isMuted ) break;  // Enough. It can't get more true :-)
    }
    return isMuted;
}



void MixDeviceComposite::setMuted(bool value)
{
    QListIterator<MixDevice*> it(_mds);
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        md->setMuted(value);
    }
}

bool MixDeviceComposite::isRecSource()
{
    bool isRecSource = false;
    QListIterator<MixDevice*> it(_mds);
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        isRecSource |= md->isRecSource();
        if ( isRecSource ) break;  // Enough. It can't get more true :-)
    }
    return isRecSource;
}


void MixDeviceComposite::setRecSource(bool value)
{
    QListIterator<MixDevice*> it(_mds);
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        md->setRecSource(value);
    }
}


bool MixDeviceComposite::isEnum()
{
    bool isEnum = true;
    QListIterator<MixDevice*> it(_mds);
    while ( it.hasNext()) {
        MixDevice* md = it.next();
        isEnum &= md->isEnum();
        if ( ! isEnum ) break;  // Enough. It can't get more false :-)
    }
    return isEnum;
}

