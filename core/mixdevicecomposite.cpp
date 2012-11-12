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



const long MixDeviceComposite::VolMax = 10000;

MixDeviceComposite::MixDeviceComposite( Mixer* mixer,  const QString& id, QList<shared_ptr<MixDevice> >& mds, const QString& name, ChannelType type ) :
   MixDevice( mixer, id, name, type )  // this will use doNotRestore == true
{
    setArtificial(true);
    _compositePlaybackVolume = new Volume( MixDeviceComposite::VolMax, 0, true, false);
    _compositePlaybackVolume->addVolumeChannel(Volume::LEFT);
    _compositePlaybackVolume->addVolumeChannel(Volume::RIGHT);

    QListIterator<shared_ptr<MixDevice> > it(mds);
    while ( it.hasNext()) {
    	shared_ptr<MixDevice> md = it.next();
        _mds.append(md);
    }
}


MixDeviceComposite::~MixDeviceComposite()
{
    while ( ! _mds.empty() ) {
        _mds.removeAt(0);
    }
    delete _compositePlaybackVolume;
//    delete _compositeCaptureVolume;
}



Volume& MixDeviceComposite::playbackVolume()
{
    return *_compositePlaybackVolume;
}

// Volume& MixDeviceComposite::captureVolume()
// {
//     return *_compositeCaptureVolume;
// }


void MixDeviceComposite::update()
{
    long volAvg;
    volAvg = calculateVolume( Volume::PlaybackVT  );
    _compositePlaybackVolume->setAllVolumes(volAvg);
    volAvg = calculateVolume( Volume::CaptureVT );
//     _compositeCaptureVolume->setAllVolumes(volAvg);

}

long MixDeviceComposite::calculateVolume(Volume::VolumeType vt)
{
    QListIterator<shared_ptr<MixDevice> > it(_mds);
    long volSum = 0;
    int  volCount = 0;
    while ( it.hasNext())
    {
    	shared_ptr<MixDevice> md = it.next();

        Volume& vol =  ( vt == Volume::CaptureVT ) ? md->captureVolume() : md->playbackVolume();
        if (vol.hasVolume() && (vol.maxVolume() != 0) ) {
            qreal normalizedVolume =
                      ( vol.getAvgVolumePercent(Volume::MALL) * MixDeviceComposite::VolMax )
                    /   vol.maxVolume();
            volSum += normalizedVolume;
            ++volCount;
        }
    }
    if ( volCount == 0 )
        return 0;
    else
        return (volSum/volCount);
}


bool MixDeviceComposite::isMuted()
{
    bool isMuted = false;
    QListIterator<shared_ptr<MixDevice> > it(_mds);
    while ( it.hasNext()) {
    	shared_ptr<MixDevice> md = it.next();
        isMuted |= md->isMuted();
        if ( isMuted ) break;  // Enough. It can't get more true :-)
    }
    return isMuted;
}



void MixDeviceComposite::setMuted(bool value)
{
    QListIterator<shared_ptr<MixDevice> > it(_mds);
    while ( it.hasNext()) {
    	shared_ptr<MixDevice> md = it.next();
        md->setMuted(value);
    }
}

bool MixDeviceComposite::isRecSource()
{
    bool isRecSource = false;
    QListIterator<shared_ptr<MixDevice> > it(_mds);
    while ( it.hasNext()) {
    	shared_ptr<MixDevice> md = it.next();
        isRecSource |= md->isRecSource();
        if ( isRecSource ) break;  // Enough. It can't get more true :-)
    }
    return isRecSource;
}


void MixDeviceComposite::setRecSource(bool value)
{
    QListIterator<shared_ptr<MixDevice> > it(_mds);
    while ( it.hasNext()) {
    	shared_ptr<MixDevice> md = it.next();
        md->setRecSource(value);
    }
}


bool MixDeviceComposite::isEnum()
{
    bool isEnum = true;
    QListIterator<shared_ptr<MixDevice> > it(_mds);
    while ( it.hasNext()) {
    	shared_ptr<MixDevice> md = it.next();
        isEnum &= md->isEnum();
        if ( ! isEnum ) break;  // Enough. It can't get more false :-)
    }
    return isEnum;
}

#include "mixdevicecomposite.moc"
