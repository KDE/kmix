/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 2008 Helio Chissini de Castro <helio@kde.org>
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

#include <cstdlib>
#include <QEventLoop>

#include "mixer_pulse.h"
#include "mixer.h"

#include <pulse/glib-mainloop.h>
#include <pulse/ext-stream-restore.h>


#define KMIXPA_PLAYBACK     0
#define KMIXPA_CAPTURE      1
#define KMIXPA_APP_PLAYBACK 2
#define KMIXPA_APP_CAPTURE  3

static unsigned int refcount = 0;
static pa_context *context = NULL;
static pa_glib_mainloop *mainloop = NULL;
static QEventLoop *s_connectionEventloop = NULL;
static enum { UNKNOWN, ACTIVE, INACTIVE } s_pulseActive = UNKNOWN;
static int s_OutstandingRequests = 0;

typedef QMap<int,devinfo> devmap;

typedef struct {
    QString description;
    devmap outputDevices;
    devmap captureDevices;
} cardinfo;

static QMap<int,cardinfo> s_Cards;
static QMap<int,int> s_CardIndexMap; // pa card idx => our card idx
static int s_CardCounter = 0;

static void dec_outstanding() {
    if (s_OutstandingRequests <= 0)
        return;

    if (--s_OutstandingRequests <= 0)
    {
        s_pulseActive = ACTIVE;
        if (s_connectionEventloop) {
            s_connectionEventloop->exit(0);
            s_connectionEventloop = NULL;

            // If we have no devices then we consider PA to be 'INACTIVE'
            if (!s_CardCounter)
                s_pulseActive = INACTIVE;
            else
                s_pulseActive = ACTIVE;
        }
    }
}

void card_cb(pa_context *c, const pa_card_info *i, int eol, void *) {

    Q_ASSERT(c == context);

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Card callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding();
        return;
    }

    // Do we have this card already?
    int idx;
    if (s_CardIndexMap.contains(i->index))
        idx = s_CardIndexMap[i->index];
    else
        idx = s_CardIndexMap[i->index] = s_CardCounter++;

    /*if (!s_Cards.contains(idx))
    {
        cardinfo card;
        s_Cards[idx] = card;
    }*/

    s_Cards[idx].description = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_DESCRIPTION);

    kDebug(67100) << "Got some info about card: " << s_Cards[idx].description;
}

void sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *) {

    Q_ASSERT(c == context);

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Sink callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding();
        return;
    }

    devinfo s;
    s.index = i->index;
    s.name = i->name;
    s.description = i->description;
    s.volume = i->volume;
    s.channel_map = i->channel_map;
    s.mute = !!i->mute;

    devmap* p_devmap = NULL;
    if (PA_INVALID_INDEX == i->card) {
        // Check to see if we have this device already in any card
        QMap<int,cardinfo>::iterator iter;
        for (iter = s_Cards.begin(); iter != s_Cards.end(); ++iter)
        {
            if (iter->outputDevices.contains(i->index))
            {
                p_devmap = &iter->outputDevices;
                break;
            }
        }
        if (!p_devmap)
        {
            // We need to create a new virtual card for our sink
            int cardidx = s_CardCounter++;
            s_Cards[cardidx].description = QString("Fake Card for %1").arg(i->description);
            p_devmap = &s_Cards[cardidx].outputDevices;
        }
    }
    else
    {
        if (!s_CardIndexMap.contains(i->card))
        {
            kError(67100) << "Got info about a sink attached to a card I know nothing about.";
            return;
        }
        int cardidx = s_CardIndexMap[i->card];
        p_devmap = &s_Cards[cardidx].outputDevices;
    }

    (*p_devmap)[s.index] = s;
    kDebug(67100) << "Got some info about sink: " << s.description;
}

void source_cb(pa_context *c, const pa_source_info *i, int eol, void *) {

    Q_ASSERT(c == context);

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Source callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding();
        return;
    }

    // Do something....
    if (PA_INVALID_INDEX != i->monitor_of_sink)
    {
        kDebug(67100) << "Ignoring Monitor Source: " << i->description;
        return;
    }

    devinfo s;
    s.index = i->index;
    s.name = i->name;
    s.description = i->description;
    s.volume = i->volume;
    s.channel_map = i->channel_map;
    s.mute = !!i->mute;

    devmap* p_devmap = NULL;
    if (PA_INVALID_INDEX == i->card) {
        // Check to see if we have this device already in any card
        QMap<int,cardinfo>::iterator iter;
        for (iter = s_Cards.begin(); iter != s_Cards.end(); ++iter)
        {
            if (iter->captureDevices.contains(i->index))
            {
                p_devmap = &iter->captureDevices;
                break;
            }
        }
        if (!p_devmap)
        {
            // We need to create a new virtual card for our sink
            int cardidx = s_CardCounter++;
            s_Cards[cardidx].description = QString("Fake Card for %1").arg(i->description);
            p_devmap = &s_Cards[cardidx].captureDevices;
        }
    }
    else
    {
        if (!s_CardIndexMap.contains(i->card))
        {
            kError(67100) << "Got info about a source attached to a card I know nothing about.";
            return;
        }
        int cardidx = s_CardIndexMap[i->card];
        p_devmap = &s_Cards[cardidx].captureDevices;
    }

    (*p_devmap)[s.index] = s;
    kDebug(67100) << "Got some info about source: " << s.description;
}

void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *) {

    Q_ASSERT(c == context);

    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                // Todo: Remove our cache of this sink (of 'index')
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_sink_info_by_index(c, index, sink_cb, NULL))) {
                    kWarning(67100) << "pa_context_get_sink_info_by_index() failed";
                    return;
                }
                pa_operation_unref(o);
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SOURCE:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                // Todo: Remove our cache of this source (of 'index')
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_source_info_by_index(c, index, source_cb, NULL))) {
                    kWarning(67100) << "pa_context_get_source_info_by_index() failed";
                    return;
                }
                pa_operation_unref(o);
            }
            break;

    }
}


static void context_state_callback(pa_context *c, void *)
{
    Q_ASSERT(c == context);

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY:
            // Attempt to load things up
            pa_operation *o;

            pa_context_set_subscribe_callback(c, subscribe_cb, NULL);

            if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)
                                           (PA_SUBSCRIPTION_MASK_SINK|
                                            PA_SUBSCRIPTION_MASK_SOURCE), NULL, NULL))) {
                kWarning(67100) << "pa_context_subscribe() failed";
                return;
            }
            pa_operation_unref(o);

            if (!(o = pa_context_get_card_info_list(c, card_cb, NULL))) {
                kWarning(67100) << "pa_context_get_card_info_list() failed";
                return;
            }
            pa_operation_unref(o);
            s_OutstandingRequests++;

            if (!(o = pa_context_get_sink_info_list(c, sink_cb, NULL))) {
                kWarning(67100) << "pa_context_get_sink_info_list() failed";
                return;
            }
            pa_operation_unref(o);
            s_OutstandingRequests++;

            if (!(o = pa_context_get_source_info_list(c, source_cb, NULL))) {
                kWarning(67100) << "pa_context_get_source_info_list() failed";
                return;
            }
            pa_operation_unref(o);
            s_OutstandingRequests++;

            break;

        case PA_CONTEXT_FAILED:
            s_pulseActive = INACTIVE;
            if (s_connectionEventloop) {
                s_connectionEventloop->exit(0);
                s_connectionEventloop = NULL;
            }
            break;

        case PA_CONTEXT_TERMINATED:
        default:
            s_pulseActive = INACTIVE;
            /// @todo Deal with reconnection...
            break;
    }
}

static void pulse_cvolume_to_Volume(pa_cvolume& cvolume, Volume* volume)
{
    Q_ASSERT(volume);

    if (cvolume.channels < 1 || cvolume.channels > 3)
    {
        kWarning(67100) <<  "Unable to set volume for a " << cvolume.channels << " channel device";
        return;
    }

    for (int i=0; i < cvolume.channels; ++i)
    {
        //kDebug(67100) <<  "Setting volume for channel " << i << " to " << cvolume.values[i] << " (" << ((100*cvolume.values[i]) / PA_VOLUME_NORM) << "%)";
        volume->setVolume((Volume::ChannelID)i, (long)cvolume.values[i]);
    }
}

static pa_cvolume pulse_Volume_to_cvolume(Volume& volume, pa_cvolume cvolume)
{
    if (cvolume.channels < 1 || cvolume.channels > 3)
    {
        kWarning(67100) <<  "Unable to set volume for a " << cvolume.channels << " channel device";
        return cvolume;
    }

    for (int i=0; i < cvolume.channels; ++i)
    {
        cvolume.values[i] = (uint32_t)volume.getVolume((Volume::ChannelID)i);
        //kDebug(67100) <<  "Setting volume for channel " << i << " to " << cvolume.values[i] << " (" << ((100*cvolume.values[i]) / PA_VOLUME_NORM) << "%)";
    }
    return cvolume;
}


void Mixer_PULSE::addDevice(devinfo& dev, bool capture)
{
    Volume *v;
    if ((v = addVolume(dev.channel_map, capture)))
    {
        pulse_cvolume_to_Volume(dev.volume, v);
        MixDevice* md = new MixDevice( _mixer, dev.name, dev.description);
        md->addPlaybackVolume(*v);
        md->setMuted(dev.mute);
        m_mixDevices.append(md);
        delete v;
    }
}

Volume* Mixer_PULSE::addVolume(pa_channel_map& cmap, bool capture)
{
    Volume* vol = 0;

    // --- Regular control (not enumerated) ---
    Volume::ChannelMask chn = Volume::MNONE;

    // Special case - mono
    if (1 == cmap.channels && PA_CHANNEL_POSITION_MONO == cmap.map[0]) {
        // We just use the left channel to represent this.
        chn = (Volume::ChannelMask)( chn | Volume::MLEFT);
    } else {
        for (uint8_t i = 0; i < cmap.channels; ++i) {
            switch (cmap.map[i]) {
                case PA_CHANNEL_POSITION_MONO:
                    kWarning(67100) << "Channel Map contains a MONO element but has >1 channel - we can't handle this.";
                    return vol;

                case PA_CHANNEL_POSITION_FRONT_LEFT:
                    chn = (Volume::ChannelMask)( chn | Volume::MLEFT);
                    break;
                case PA_CHANNEL_POSITION_FRONT_RIGHT:
                    chn = (Volume::ChannelMask)( chn | Volume::MRIGHT);
                    break;
                case PA_CHANNEL_POSITION_FRONT_CENTER:
                    chn = (Volume::ChannelMask)( chn | Volume::MCENTER);
                    break;
                case PA_CHANNEL_POSITION_REAR_CENTER:
                    chn = (Volume::ChannelMask)( chn | Volume::MREARCENTER);
                    break;
                case PA_CHANNEL_POSITION_REAR_LEFT:
                    chn = (Volume::ChannelMask)( chn | Volume::MSURROUNDLEFT);
                    break;
                case PA_CHANNEL_POSITION_REAR_RIGHT:
                    chn = (Volume::ChannelMask)( chn | Volume::MSURROUNDRIGHT);
                    break;
                case PA_CHANNEL_POSITION_LFE:
                    chn = (Volume::ChannelMask)( chn | Volume::MWOOFER);
                    break;
                case PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER:
                    chn = (Volume::ChannelMask)( chn | Volume::MREARSIDELEFT);
                    break;
                case PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER:
                    chn = (Volume::ChannelMask)( chn | Volume::MREARSIDERIGHT);
                    break;
                default:
                    kWarning(67100) << "Channel Map contains a pa_channel_position we cannot handle " << cmap.map[i];
                    break;
            }
        }
    }

    if (chn != Volume::MNONE) {
        //kDebug() << "Add somthing with chn=" << chn << ", capture=" << capture;
        vol = new Volume( chn, PA_VOLUME_NORM, PA_VOLUME_MUTED, false, capture);
    }

    return vol;
}


Mixer_Backend* PULSE_getMixer( Mixer *mixer, int devnum )
{
   Mixer_Backend *l_mixer;
   l_mixer = new Mixer_PULSE( mixer, devnum );
   return l_mixer;
}


Mixer_PULSE::Mixer_PULSE(Mixer *mixer, int devnum) : Mixer_Backend(mixer, devnum)
{
   if ( devnum == -1 )
      m_devnum = 0;

   ++refcount;
   if (INACTIVE != s_pulseActive && 1 == refcount)
   {
       mainloop = pa_glib_mainloop_new(g_main_context_default());
       g_assert(mainloop);

       pa_mainloop_api *api = pa_glib_mainloop_get_api(mainloop);
       g_assert(api);

       context = pa_context_new(api, "KMix KDE 4");
       g_assert(context);

       // We create a simple event loop to allow the glib loop
       // to iterate until we've connected or not to the server.
       s_connectionEventloop = new QEventLoop;

       // (cg) Convert to PA_CONTEXT_NOFLAGS when PulseAudio 0.9.19 is required
       if (pa_context_connect(context, NULL, static_cast<pa_context_flags_t>(0), 0) >= 0) {
           pa_context_set_state_callback(context, &context_state_callback, s_connectionEventloop);
           // Now we block until we connect or otherwise...
           s_connectionEventloop->exec();
       }
   }
}

Mixer_PULSE::~Mixer_PULSE()
{
    if (refcount > 0)
    {
        --refcount;
        if (0 == refcount)
        {
            pa_context_unref(context);
            pa_glib_mainloop_free(mainloop);
        }
    }
}

int Mixer_PULSE::open()
{
    kDebug(67100) <<  "Trying Pulse sink";

    Volume *v;
    if (ACTIVE == s_pulseActive && false)
    {
        // Check to see if we have a "card" for m_devnum
        if (s_Cards.contains(m_devnum))
        {
            kDebug(67100) <<  "Found PulseAudio 'card' " << m_devnum;
            cardinfo *card = &s_Cards[m_devnum];

            m_mixerName = card->description;

            devmap::iterator iter;
            for (iter = card->outputDevices.begin(); iter != card->outputDevices.end(); ++iter)
            {
                if ((v = addVolume(iter->channel_map, false)))
                {
                    pulse_cvolume_to_Volume(iter->volume, v);
                    MixDevice* md = new MixDevice( _mixer, iter->name, QString("Playback: %1").arg(iter->description));
                    md->addPlaybackVolume(*v);
                    md->setMuted(iter->mute);
                    m_mixDevices.append(md);
                    delete v;
                }
            }
            for (iter = card->captureDevices.begin(); iter != card->captureDevices.end(); ++iter)
            {
                if ((v = addVolume(iter->channel_map, true)))
                {
                    pulse_cvolume_to_Volume(iter->volume, v);
                    MixDevice* md = new MixDevice( _mixer, iter->name, QString("Capture: %1").arg(iter->description));
                    md->addCaptureVolume(*v);
                    md->setMuted(iter->mute);
                    m_mixDevices.append(md);
                    delete v;
                }
            }
        }
    }
    else if (ACTIVE == s_pulseActive)
    {
        QMap<int,cardinfo>::iterator citer;
        devmap::iterator iter;
        if (KMIXPA_PLAYBACK == m_devnum)
        {
            m_mixerName = "Playback Devices";
            for (citer = s_Cards.begin(); citer != s_Cards.end(); ++citer)
            {
                cardinfo *card = &(*citer);
                for (iter = card->outputDevices.begin(); iter != card->outputDevices.end(); ++iter)
                    addDevice(*iter, false);
            }
        }
        else if (KMIXPA_CAPTURE == m_devnum)
        {
            m_mixerName = "Capture Devices";
            for (citer = s_Cards.begin(); citer != s_Cards.end(); ++citer)
            {
                cardinfo *card = &(*citer);
                for (iter = card->captureDevices.begin(); iter != card->captureDevices.end(); ++iter)
                    addDevice(*iter, true);
            }
        }
        else if (KMIXPA_APP_PLAYBACK == m_devnum)
        {
            // TODO: "Applications (Playback)".
        }
        else if (KMIXPA_APP_CAPTURE == m_devnum)
        {
            // TODO: "Applications (Capture)".
        }
    }
 
    m_isOpen = true;

    return 0;
}

int Mixer_PULSE::close()
{
    return 1;
}

int Mixer_PULSE::readVolumeFromHW( const QString& id, MixDevice *md )
{
    // TODO Work out a way to prevent polling and push it instead...
    QMap<int,cardinfo>::iterator citer;
    devmap::iterator iter;
    for (citer = s_Cards.begin(); citer != s_Cards.end(); ++citer)
    {
        cardinfo *card = &(*citer);
        if (!md->isRecSource())
        {
            for (iter = card->outputDevices.begin(); iter != card->outputDevices.end(); ++iter)
            {
                if (iter->name == id)
                {
                    pulse_cvolume_to_Volume(iter->volume, &md->playbackVolume());
                    md->setMuted(iter->mute);
                    return 0;
                }
            }
        }
        else
        {
            for (iter = card->captureDevices.begin(); iter != card->captureDevices.end(); ++iter)
            {
                if (iter->name == id)
                {
                    pulse_cvolume_to_Volume(iter->volume, &md->captureVolume());
                    md->setMuted(iter->mute);
                    return 0;
                }
            }
        }
    }
    return 0;
}

int Mixer_PULSE::writeVolumeToHW( const QString& id, MixDevice *md )
{
    QMap<int,cardinfo>::iterator citer;
    devmap::iterator iter;
    for (citer = s_Cards.begin(); citer != s_Cards.end(); ++citer)
    {
        cardinfo *card = &(*citer);
        if (md->playbackVolume().hasVolume())
        {
            for (iter = card->outputDevices.begin(); iter != card->outputDevices.end(); ++iter)
            {
                if (iter->name == id)
                {
                    pa_operation *o;

                    pa_cvolume volume = pulse_Volume_to_cvolume(md->playbackVolume(), iter->volume);
                    if (!(o = pa_context_set_sink_volume_by_index(context, iter->index, &volume, NULL, NULL))) {
                        kWarning(67100) <<  "pa_context_set_sink_volume_by_index() failed";
                        return Mixer::ERR_READ;
                    }
                    pa_operation_unref(o);

                    if (!(o = pa_context_set_sink_mute_by_index(context, iter->index, (md->isMuted() ? 1 : 0), NULL, NULL))) {
                        kWarning(67100) <<  "pa_context_set_sink_mute_by_index() failed";
                        return Mixer::ERR_READ;
                    }
                    pa_operation_unref(o);

                    return 0;
                }
            }
        }
        else
        {
            for (iter = card->captureDevices.begin(); iter != card->captureDevices.end(); ++iter)
            {
                if (iter->name == id)
                {
                    pa_operation *o;

                    pa_cvolume volume = pulse_Volume_to_cvolume(md->captureVolume(), iter->volume);
                    if (!(o = pa_context_set_source_volume_by_index(context, iter->index, &volume, NULL, NULL))) {
                        kWarning(67100) <<  "pa_context_set_source_volume_by_index() failed";
                        return Mixer::ERR_READ;
                    }
                    pa_operation_unref(o);

                    if (!(o = pa_context_set_source_mute_by_index(context, iter->index, (md->isMuted() ? 1 : 0), NULL, NULL))) {
                        kWarning(67100) <<  "pa_context_set_source_mute_by_index() failed";
                        return Mixer::ERR_READ;
                    }
                    pa_operation_unref(o);

                    return 0;
                }
            }
        }
    }
    return 0;
}

void Mixer_PULSE::setRecsrcHW( const QString& /*id*/, bool /* on */ )
{
   return;
}

bool Mixer_PULSE::isRecsrcHW( const QString& id )
{
/*   int devnum = id2num(id);
   switch ( devnum )
   {
      case MIXERDEV_MICROPHONE :
      case MIXERDEV_LINE_IN :
      case MIXERDEV_CD :
         return true;

      default :
         return false;
   }*/
   return false;
}


QString PULSE_getDriverName() {
        return "PulseAudio";
}

QString Mixer_PULSE::getDriverName()
{
        return "PulseAudio";
}

