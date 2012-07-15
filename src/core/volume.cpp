/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include "core/volume.h"

// for operator<<()
#include <iostream>

#include <kdebug.h>


int Volume::m_channelMaskEnum[9] =
{
    MLEFT, MRIGHT, MCENTER,
    MWOOFER,
    MSURROUNDLEFT, MSURROUNDRIGHT,
    MREARSIDELEFT, MREARSIDERIGHT,
    MREARCENTER
};

QString Volume::ChannelNameReadable[9] =
{
    "Left", "Right",
    "Center", "Subwoofer",
    "Surround Left", "Surround Right",
    "Side Left", "Side Right",
    "Rear Center"
};

char Volume::ChannelNameForPersistence[9][30] =
{
    "volumeL", "volumeR",
    "volumeCenter", "volumeWoofer",
    "volumeSurroundL", "volumeSurroundR",
    "volumeSideL", "volumeSideR",
    "volumeRearCenter"
};

// Forbidden/private. Only here because if there is no CaptureVolume we need the values initialized
// And also QMap requires it.
Volume::Volume()
{
    m_minVolume = 0;
    m_maxVolume = 0;
    m_hasSwitch = false;
}

VolumeChannel::VolumeChannel(Volume::ChannelID chid)
    : m_volume(0)
    , m_chid(chid)
{

}

VolumeChannel::VolumeChannel()
{

}

Volume::Volume(int maxVolume, int minVolume, bool hasSwitch, bool isCapture)
{
    init((ChannelMask) 0, maxVolume, minVolume, hasSwitch, isCapture);
}

/**
 * @Deprecated
 */
void Volume::addVolumeChannels(ChannelMask chmask)
{
    for (Volume::ChannelID chid = Volume::CHIDMIN; chid <= Volume::CHIDMAX;) {
        if (chmask & Volume::m_channelMaskEnum[chid]) {
            addVolumeChannel(VolumeChannel(chid));
        }
        chid = (Volume::ChannelID)( 1 + (int)chid); // ugly
    } // for all channels
}

void Volume::addVolumeChannel(VolumeChannel ch)
{
    m_volumesL.insert(ch.m_chid, ch);
}

void Volume::init(ChannelMask chmask, int maxVolume, int minVolume, bool hasSwitch, bool isCapture)
{
    m_chmask          = chmask;
    m_maxVolume       = maxVolume;
    m_minVolume       = minVolume;
    m_hasSwitch       = hasSwitch;
    m_isCapture       = isCapture;
    //_muted           = false;
    m_switchActivated = false;
}

QMap<Volume::ChannelID, VolumeChannel> Volume::getVolumes() const
{
    return m_volumesL;
}

void Volume::setSwitch(bool val)
{
     m_switchActivated = val;
}

bool Volume::isSwitchActivated() const
{
     return m_switchActivated && m_hasSwitch;
}

// @ compatibility
void Volume::setAllVolumes(int vol)
{
    int finalVol = volrange(vol);
    QMap<Volume::ChannelID, VolumeChannel>::iterator it = m_volumesL.begin();
    while (it != m_volumesL.end()) {
        it.value().m_volume = finalVol;
        ++it;
    }
}

void Volume::changeAllVolumes(int step)
{
    QMap<Volume::ChannelID, VolumeChannel>::iterator it = m_volumesL.begin();
    while (it != m_volumesL.end()) {
        it.value().m_volume = volrange(it.value().m_volume + step);
        ++it;
    }
}


// @ compatibility
void Volume::setVolume(ChannelID chid, int vol)
{
    QMap<Volume::ChannelID, VolumeChannel>::iterator it = m_volumesL.find(chid);
    if (it != m_volumesL.end()) {
        it.value().m_volume = vol;
    }
}

/**
 * Copy the volume elements contained in v to this Volume object.
 */
void Volume::setVolume(const Volume &v)
{
    foreach (VolumeChannel vc, m_volumesL) {
        ChannelID chid = vc.m_chid;
        v.getVolumes()[chid].m_volume = vc.m_volume;
    }
}

int Volume::maxVolume()
{
    return m_maxVolume;
}

int Volume::minVolume()
{
    return m_minVolume;
}

int Volume::volumeSpan()
{
    return m_maxVolume - m_minVolume + 1;
}

int Volume::getVolume(ChannelID chid)
{
    return m_volumesL.value(chid).m_volume;
}

qreal Volume::getAvgVolume(ChannelMask chmask)
{
    int avgVolumeCounter = 0;
    int sumOfActiveVolumes = 0;
    foreach (VolumeChannel vc, m_volumesL) {
        if (Volume::m_channelMaskEnum[vc.m_chid] & chmask) {
            sumOfActiveVolumes += vc.m_volume;
            ++ avgVolumeCounter;
        }
    }
    if (avgVolumeCounter != 0) {
        qreal sumOfActiveVolumesQreal = sumOfActiveVolumes;
        sumOfActiveVolumesQreal /= avgVolumeCounter;
        return sumOfActiveVolumesQreal;
    }
    else
        return 0;
}


int Volume::getAvgVolumePercent(ChannelMask chmask)
{
    qreal volume = getAvgVolume(chmask);
    // min=-100, max=200 => volSpan = 301
    // volume = -50 =>  volShiftedToZero = -50+min = 50
    qreal volSpan = volumeSpan();
    qreal volShiftedToZero = volume - m_minVolume;
    qreal percentReal = volSpan == 0 ? 0 : (100 * volShiftedToZero) / (volSpan - 1);
    int percent = qRound(percentReal);

    return percent;
}

int Volume::count()
{
    return getVolumes().count();
}

bool Volume::hasSwitch() const
{
     return m_hasSwitch;
     // TODO { return _hasSwitch || hasVolume() ; }
     // "|| hasVolume()", because we simulate a switch, if it is not available as hardware.
}

bool Volume::hasVolume() const
{
     return m_maxVolume != m_minVolume;
}

bool Volume::isCapture() const
{
     return m_isCapture; // -<- Query this, to find out whether this is a capture or a playback volume
}

void Volume::setSwitchType(Volume::SwitchType type)
{
     m_switchType = type;
}

Volume::SwitchType Volume::switchType() const
{
     return m_switchType;
}

/**
 * returns a "sane" volume level. This means, it is a volume level inside the
 * valid bounds
 */
int Volume::volrange(int vol)
{
    if (vol < m_minVolume) {
        return m_minVolume;
    }
    else if (vol < m_maxVolume) {
        return vol;
    }
    else {
        return m_maxVolume;
    }
}


std::ostream& operator<<(std::ostream& os, const Volume& vol) {
    os << "(";

    bool first = true;
    foreach (const VolumeChannel vc, vol.getVolumes()) {
        if ( !first )  os << ",";
        else first = false;
        os << vc.m_volume;
    } // all channels
    os << ")";

    os << " [" << vol.m_minVolume << "-" << vol.m_maxVolume;
    if (vol.m_switchActivated) {
        os << " : switch active ]";
    } else {
        os << " : switch inactive ]";
    }

    return os;
}

QDebug operator<<(QDebug os, const Volume& vol) {
    os << "(";
    bool first = true;
    foreach (VolumeChannel vc, vol.getVolumes()) {
        if ( !first )  os << ",";
        else first = false;
        os << vc.m_volume;
    } // all channels
    os << ")";

    os << " [" << vol.m_minVolume << "-" << vol.m_maxVolume;
    if ( vol.m_switchActivated ) {
        os << " : switch active ]";
    } else {
        os << " : switch inactive ]";
    }

    return os;
}
