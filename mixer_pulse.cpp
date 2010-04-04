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

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <pulse/ext-stream-restore.h>

static unsigned int refcount = 0;
static pa_context *context = NULL;
static pa_glib_mainloop *mainloop = NULL;
static QEventLoop *s_connectionEventloop = NULL;
static bool s_pulseActive = true;
static int s_OutstandingRequests = 0;

static void dec_outstanding() {
    if (s_OutstandingRequests <= 0)
        return;

    if (--s_OutstandingRequests <= 0)
    {
        if (s_connectionEventloop) {
            s_connectionEventloop->exit(0);
            s_connectionEventloop = NULL;
        }
    }
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

    // Do something....
    kDebug(67100) << "Got some info about sink: " << i->name;
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
    kDebug(67100) << "Got some info about source: " << i->name;
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
            s_pulseActive = false;
            if (s_connectionEventloop) {
                s_connectionEventloop->exit(0);
                s_connectionEventloop = NULL;
            }
            break;

        case PA_CONTEXT_TERMINATED:
        default:
            s_pulseActive = false;
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

   if (0 == refcount++)
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
    if (0 == --refcount)
    {
        pa_context_unref(context);
        pa_glib_mainloop_free(mainloop);
    }
}

int Mixer_PULSE::open()
{
    kDebug(67100) <<  "Trying Pulse sink";
      //return Mixer::ERR_OPEN;
 
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

    m_mixerName = "PULSE Audio Mixer";

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

