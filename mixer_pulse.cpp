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

// (cg) I've not yet worked out why this is not defined.....
# define UINT32_MAX     (4294967295U)

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <pulse/ext-stream-restore.h>

static unsigned int refcount = 0;
static pa_context *context = NULL;
static pa_glib_mainloop *mainloop = NULL;
static QEventLoop *s_connectionEventloop = NULL;
static enum { UNKNOWN, ACTIVE, INACTIVE } s_pulseActive = UNKNOWN;
static int s_OutstandingRequests = 0;


typedef struct {
    int index;
    QString name;
    QString description;
    pa_cvolume volume;
    pa_channel_map channel_map;
    bool mute;
} devinfo;

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
                // *iter->volume.channels
                // Fix me: Map the channels to the ChanMask... maybe we need the sink/source channel_map for this...
                Volume::ChannelMask chmask = Volume::MMAIN;
                Volume vol(chmask, PA_VOLUME_MAX, PA_VOLUME_MUTED, false, false);
                MixDevice* md = new MixDevice( _mixer, iter->name, QString("Playback: %1").arg(iter->description));
                md->addPlaybackVolume(vol);
                m_mixDevices.append( md );
            }
            for (iter = card->captureDevices.begin(); iter != card->captureDevices.end(); ++iter)
            {
                // *iter->volume.channels
                // Fix me: Map the channels to the ChanMask... maybe we need the sink/source channel_map for this...
                Volume::ChannelMask chmask = Volume::MMAIN;
                Volume vol(chmask, PA_VOLUME_MAX, PA_VOLUME_MUTED, false, true);
                MixDevice* md = new MixDevice( _mixer, iter->name, QString("Capture: %1").arg(iter->description));
                md->addCaptureVolume(vol);
                m_mixDevices.append( md );
            }
        }
    }
    else if (ACTIVE == s_pulseActive)
    {
        QMap<int,cardinfo>::iterator citer;
        devmap::iterator iter;
        if (0 == m_devnum)
        {
            m_mixerName = "Playback Devices";
            for (citer = s_Cards.begin(); citer != s_Cards.end(); ++citer)
            {
                cardinfo *card = &(*citer);
                // Fix me: Map the channels to the ChanMask... maybe we need the sink/source channel_map for this...
                for (iter = card->outputDevices.begin(); iter != card->outputDevices.end(); ++iter)
                {
                    // *iter->volume.channels
                    // Fix me: Map the channels to the ChanMask... maybe we need the sink/source channel_map for this...
                    Volume::ChannelMask chmask = Volume::MMAIN;
                    Volume vol(chmask, PA_VOLUME_MAX, PA_VOLUME_MUTED, false, false);
                    MixDevice* md = new MixDevice( _mixer, iter->name, iter->description);
                    md->addPlaybackVolume(vol);
                    m_mixDevices.append( md );
                }
            }
        }
        else if (1 == m_devnum)
        {
            m_mixerName = "Capture Devices";
            for (citer = s_Cards.begin(); citer != s_Cards.end(); ++citer)
            {
                cardinfo *card = &(*citer);
                // Fix me: Map the channels to the ChanMask... maybe we need the sink/source channel_map for this...
                for (iter = card->captureDevices.begin(); iter != card->captureDevices.end(); ++iter)
                {
                    // *iter->volume.channels
                    // Fix me: Map the channels to the ChanMask... maybe we need the sink/source channel_map for this...
                    Volume::ChannelMask chmask = Volume::MMAIN;
                    Volume vol(chmask, PA_VOLUME_MAX, PA_VOLUME_MUTED, false, true);
                    MixDevice* md = new MixDevice( _mixer, iter->name, iter->description);
                    md->addCaptureVolume(vol);
                    m_mixDevices.append( md );
                }
            }
        }
        else if (2 == m_devnum)
        {
            // TODO: "Applications".
        }
    }
 
/* 
      //
      // Mixer is open. Now define all of the mix devices.
      //

         for ( int idx = 0; idx < numDevs; idx++ )
         {
            Volume vol( 2, AUDIO_MAX_GAIN );
            QString id;
            id.setNum(idx);
            MixDevice* md = new MixDevice( _mixer, id,
               QString(MixerDevNames[idx]), MixerChannelTypes[idx]);
            md->addPlaybackVolume(vol);
            md->setRecSource( isRecsrcHW( idx ) );
            m_mixDevices.append( md );
         }
*/

    m_isOpen = true;

    return 0;
}

int Mixer_PULSE::close()
{
    return 1;
}

int Mixer_PULSE::readVolumeFromHW( const QString& id, MixDevice *md )
{
/*   audio_info_t audioinfo;
   uint_t devMask = MixerSunPortMasks[devnum];

   Volume& volume = md->playbackVolume();
   int devnum = id2num(id);
   //
   // Read the current audio information from the driver
   //
   if ( ioctl( fd, AUDIO_GETINFO, &audioinfo ) < 0 )
   {
      return( Mixer::ERR_READ );
   }
   else
   {
      //
      // Extract the appropriate fields based on the requested device
      //
      switch ( devnum )
      {
         case MIXERDEV_MASTER_VOLUME :
            volume.setSwitchActivated( audioinfo.output_muted );
            GainBalanceToVolume( audioinfo.play.gain,
                                 audioinfo.play.balance,
                                 volume );
            break;

         case MIXERDEV_RECORD_MONITOR :
            md->setMuted(false);
            volume.setAllVolumes( audioinfo.monitor_gain );
            break;

         case MIXERDEV_INTERNAL_SPEAKER :
         case MIXERDEV_HEADPHONE :
         case MIXERDEV_LINE_OUT :
            md->setMuted( (audioinfo.play.port & devMask) ? false : true );
            GainBalanceToVolume( audioinfo.play.gain,
                                 audioinfo.play.balance,
                                 volume );
            break;

         case MIXERDEV_MICROPHONE :
         case MIXERDEV_LINE_IN :
         case MIXERDEV_CD :
            md->setMuted( (audioinfo.record.port & devMask) ? false : true );
            GainBalanceToVolume( audioinfo.record.gain,
                                 audioinfo.record.balance,
                                 volume );
            break;

         default :
            return Mixer::ERR_READ;
      }
      return 0;
   }*/
   return 0;
}

int Mixer_PULSE::writeVolumeToHW( const QString& id, MixDevice *md )
{
/*   uint_t gain;
   uchar_t balance;
   uchar_t mute;

   Volume& volume = md->playbackVolume();
   int devnum = id2num(id);
   //
   // Convert the Volume(left vol, right vol) to the Gain/Balance Sun uses
   //
   VolumeToGainBalance( volume, gain, balance );
   mute = md->isMuted() ? 1 : 0;

   //
   // Read the current audio settings from the hardware
   //
   audio_info_t audioinfo;
   if ( ioctl( fd, AUDIO_GETINFO, &audioinfo ) < 0 )
   {
      return( Mixer::ERR_READ );
   }

   //
   // Now, based on the devnum that we are writing to, update the appropriate
   // volume field and twiddle the appropriate bitmask to enable/mute the
   // device as necessary.
   //
   switch ( devnum )
   {
      case MIXERDEV_MASTER_VOLUME :
         audioinfo.play.gain = gain;
         audioinfo.play.balance = balance;
         audioinfo.output_muted = mute;
         break;

      case MIXERDEV_RECORD_MONITOR :
         audioinfo.monitor_gain = gain;
         // no mute or balance for record monitor
         break;

      case MIXERDEV_INTERNAL_SPEAKER :
      case MIXERDEV_HEADPHONE :
      case MIXERDEV_LINE_OUT :
         audioinfo.play.gain = gain;
         audioinfo.play.balance = balance;
         if ( mute )
            audioinfo.play.port &= ~MixerSunPortMasks[devnum];
         else
            audioinfo.play.port |= MixerSunPortMasks[devnum];
         break;

      case MIXERDEV_MICROPHONE :
      case MIXERDEV_LINE_IN :
      case MIXERDEV_CD :
         audioinfo.record.gain = gain;
         audioinfo.record.balance = balance;
         if ( mute )
            audioinfo.record.port &= ~MixerSunPortMasks[devnum];
         else
            audioinfo.record.port |= MixerSunPortMasks[devnum];
         break;

      default :
         return Mixer::ERR_READ;
   }

   //
   // Now that we've updated the audioinfo struct, write it back to the hardware
   //
   if ( ioctl( fd, AUDIO_SETINFO, &audioinfo ) < 0 )
   {
      return( Mixer::ERR_WRITE );
   }
   else
   {
      return 0;
   }*/
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

