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

#include "ALSAControl.h"

namespace Backends {

AlsaControl::AlsaControl(Category category, snd_mixer_elem_t *elem, QObject *parent)
    : Control(category, parent)
    , m_elem(elem)
{
}

AlsaControl::~AlsaControl()
{
}

QString AlsaControl::displayName() const
{
    return QString::fromUtf8(snd_mixer_selem_get_name(m_elem));
}

QString AlsaControl::iconName() const
{
    return QString();
}

typedef struct {
    Control::Channel kmix;
    snd_mixer_selem_channel_id_t alsa;
} ChannelMap;

static ChannelMap CMAP[] = {
    { Control::FrontLeft, SND_MIXER_SCHN_FRONT_LEFT },
    { Control::FrontRight, SND_MIXER_SCHN_FRONT_RIGHT },
    { Control::Center, SND_MIXER_SCHN_FRONT_CENTER },
    { Control::RearLeft, SND_MIXER_SCHN_REAR_LEFT },
    { Control::RearRight, SND_MIXER_SCHN_REAR_RIGHT },
    { Control::Subwoofer, SND_MIXER_SCHN_WOOFER },
    { Control::SideLeft, SND_MIXER_SCHN_SIDE_LEFT },
    { Control::SideRight, SND_MIXER_SCHN_SIDE_RIGHT },
    { Control::RearCenter, SND_MIXER_SCHN_REAR_CENTER },
    { Control::Mono, SND_MIXER_SCHN_MONO },
    { (Control::Channel)-1, (snd_mixer_selem_channel_id_t)-1 }
};

snd_mixer_selem_channel_id_t AlsaControl::alsaChannel(Channel c) const
{
    for(ChannelMap *map = &CMAP[0];map->kmix != -1;map++) {
        if (map->kmix == c)
            return map->alsa;
    }
    return SND_MIXER_SCHN_UNKNOWN;
}

int AlsaControl::channels() const
{
    int channelCount = 0;
    for(ChannelMap *map = &CMAP[0];map->kmix != -1;map++) {
        if (snd_mixer_selem_has_playback_channel(m_elem, map->alsa))
            channelCount++;
    }
    return channelCount;
}

int AlsaControl::getVolume(Channel c) const
{
    long min;
    long max;
    long value;
    if (max == 0)
        return 0;
    snd_mixer_selem_get_playback_volume_range(m_elem, &min, &max);
    snd_mixer_selem_get_playback_volume(m_elem, alsaChannel(c), &value);
    return ((value+min)/max)*65536;
}

void AlsaControl::setVolume(Channel c, int v)
{
}

bool AlsaControl::isMuted() const
{
    for(ChannelMap *map = &CMAP[0];map->kmix != -1;map++) {
        int value;
        snd_mixer_selem_get_playback_switch(m_elem, map->alsa, &value);
        if (value)
            return true;
    }
    return false;
}

bool AlsaControl::canMute() const
{
    return snd_mixer_selem_has_playback_switch(m_elem);
}

void AlsaControl::setMute(bool yes)
{
    for(ChannelMap *map = &CMAP[0];map->kmix != -1;map++) {
        snd_mixer_selem_set_playback_switch(m_elem, map->alsa, !yes);
    }
}

}

#include "ALSAControl.moc"
