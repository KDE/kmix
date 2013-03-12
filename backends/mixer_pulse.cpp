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

#include "mixer_pulse.h"

#include <cstdlib>
#include <QtCore/QAbstractEventDispatcher>
#include <QTimer>

#include <klocale.h>

#include "core/mixer.h"
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"

#include <pulse/glib-mainloop.h>
#include <pulse/ext-stream-restore.h>
#if defined(HAVE_CANBERRA)
#  include <canberra.h>
#endif

// PA_VOLUME_UI_MAX landed in pulseaudio-0.9.23, so this can be removed when/if
// minimum requirement is ever bumped up (from 0.9.12 currently)
#ifndef PA_VOLUME_UI_MAX
#define PA_VOLUME_UI_MAX (pa_sw_volume_from_dB(+11.0))
#endif

#define HAVE_SOURCE_OUTPUT_VOLUMES PA_CHECK_VERSION(1,0,0)

#define KMIXPA_PLAYBACK     0
#define KMIXPA_CAPTURE      1
#define KMIXPA_APP_PLAYBACK 2
#define KMIXPA_APP_CAPTURE  3
#define KMIXPA_WIDGET_MAX KMIXPA_APP_CAPTURE

#define KMIXPA_EVENT_KEY "sink-input-by-media-role:event"

static unsigned int refcount = 0;
static pa_glib_mainloop *s_mainloop = NULL;
static pa_context *s_context = NULL;
static enum { UNKNOWN, ACTIVE, INACTIVE } s_pulseActive = UNKNOWN;
static int s_outstandingRequests = 0;

#if defined(HAVE_CANBERRA)
static ca_context *s_ccontext = NULL;
#endif

QMap<int,Mixer_PULSE*> s_mixers;

typedef QMap<int,devinfo> devmap;
static devmap outputDevices;
static devmap captureDevices;
static QMap<int,QString> clients;
static devmap outputStreams;
static devmap captureStreams;
static devmap outputRoles;

typedef struct {
    pa_channel_map channel_map;
    pa_cvolume volume;
    bool mute;
    QString device;
} restoreRule;
static QMap<QString,restoreRule> s_RestoreRules;

static void dec_outstanding(pa_context *c) {
    if (s_outstandingRequests <= 0)
        return;

    if (--s_outstandingRequests == 0)
    {
        s_pulseActive = ACTIVE;

        // If this is our probe phase, exit our context immediately
        if (s_context != c) {
            pa_context_disconnect(c);
        } else
          kDebug(67100) <<  "Reconnected to PulseAudio";
    }
}

static void translateMasksAndMaps(devinfo& dev)
{
    dev.chanMask = Volume::MNONE;
    dev.chanIDs.clear();

    if (dev.channel_map.channels != dev.volume.channels) {
        kError() << "Hiddeous Channel mixup map says " << dev.channel_map.channels << ", volume says: " << dev.volume.channels;
        return;
    }
    if (1 == dev.channel_map.channels && PA_CHANNEL_POSITION_MONO == dev.channel_map.map[0]) {
        // We just use the left channel to represent this.
        dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MLEFT);
        dev.chanIDs[0] = Volume::LEFT;
    } else {
        for (uint8_t i = 0; i < dev.channel_map.channels; ++i) {
            switch (dev.channel_map.map[i]) {
                case PA_CHANNEL_POSITION_MONO:
                    kWarning(67100) << "Channel Map contains a MONO element but has >1 channel - we can't handle this.";
                    return;

                case PA_CHANNEL_POSITION_FRONT_LEFT:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MLEFT);
                    dev.chanIDs[i] = Volume::LEFT;
                    break;
                case PA_CHANNEL_POSITION_FRONT_RIGHT:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MRIGHT);
                    dev.chanIDs[i] = Volume::RIGHT;
                    break;
                case PA_CHANNEL_POSITION_FRONT_CENTER:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MCENTER);
                    dev.chanIDs[i] = Volume::CENTER;
                    break;
                case PA_CHANNEL_POSITION_REAR_CENTER:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MREARCENTER);
                    dev.chanIDs[i] = Volume::REARCENTER;
                    break;
                case PA_CHANNEL_POSITION_REAR_LEFT:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MSURROUNDLEFT);
                    dev.chanIDs[i] = Volume::SURROUNDLEFT;
                    break;
                case PA_CHANNEL_POSITION_REAR_RIGHT:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MSURROUNDRIGHT);
                    dev.chanIDs[i] = Volume::SURROUNDRIGHT;
                    break;
                case PA_CHANNEL_POSITION_LFE:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MWOOFER);
                    dev.chanIDs[i] = Volume::WOOFER;
                    break;
                case PA_CHANNEL_POSITION_SIDE_LEFT:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MREARSIDELEFT);
                    dev.chanIDs[i] = Volume::REARSIDELEFT;
                    break;
                case PA_CHANNEL_POSITION_SIDE_RIGHT:
                    dev.chanMask = (Volume::ChannelMask)( dev.chanMask | Volume::MREARSIDERIGHT);
                    dev.chanIDs[i] = Volume::REARSIDERIGHT;
                    break;
                default:
                    kWarning(67100) << "Channel Map contains a pa_channel_position we cannot handle " << dev.channel_map.map[i];
                    break;
            }
        }
    }
}

static QString getIconNameFromProplist(pa_proplist *l) {
    const char *t;

    if ((t = pa_proplist_gets(l, PA_PROP_MEDIA_ICON_NAME)))
        return QString::fromUtf8(t);

    if ((t = pa_proplist_gets(l, PA_PROP_WINDOW_ICON_NAME)))
        return QString::fromUtf8(t);

    if ((t = pa_proplist_gets(l, PA_PROP_APPLICATION_ICON_NAME)))
        return QString::fromUtf8(t);

    if ((t = pa_proplist_gets(l, PA_PROP_MEDIA_ROLE))) {

        if (strcmp(t, "video") == 0 || strcmp(t, "phone") == 0)
            return QString::fromUtf8(t);

        if (strcmp(t, "music") == 0)
            return "audio";

        if (strcmp(t, "game") == 0)
            return "applications-games";

        if (strcmp(t, "event") == 0)
            return "dialog-information";
    }

    return "";
}

static void sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *) {

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Sink callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);
        if (s_mixers.contains(KMIXPA_PLAYBACK))
            s_mixers[KMIXPA_PLAYBACK]->triggerUpdate();
        return;
    }

    devinfo s;
    s.index = s.device_index = i->index;
    s.name = QString::fromUtf8(i->name).replace(' ', '_');
    s.description = QString::fromUtf8(i->description);
    s.icon_name = QString::fromUtf8(pa_proplist_gets(i->proplist, PA_PROP_DEVICE_ICON_NAME));
    s.volume = i->volume;
    s.channel_map = i->channel_map;
    s.mute = !!i->mute;
    s.stream_restore_rule = "";

    translateMasksAndMaps(s);

    bool is_new = !outputDevices.contains(s.index);
    outputDevices[s.index] = s;
//     kDebug(67100) << "Got some info about sink: " << s.description;

    if (s_mixers.contains(KMIXPA_PLAYBACK)) {
        if (is_new)
            s_mixers[KMIXPA_PLAYBACK]->addWidget(s.index);
        else {
            int mid = s_mixers[KMIXPA_PLAYBACK]->id2num(s.name);
            if (mid >= 0) {
                MixSet *ms = s_mixers[KMIXPA_PLAYBACK]->getMixSet();
                (*ms)[mid]->setReadableName(s.description);
            }
        }
    }
}

static void source_cb(pa_context *c, const pa_source_info *i, int eol, void *) {

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Source callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);
        if (s_mixers.contains(KMIXPA_CAPTURE))
            s_mixers[KMIXPA_CAPTURE]->triggerUpdate();
        return;
    }

    // Do something....
    if (PA_INVALID_INDEX != i->monitor_of_sink)
    {
        kDebug(67100) << "Ignoring Monitor Source: " << i->description;
        return;
    }

    devinfo s;
    s.index = s.device_index = i->index;
    s.name = QString::fromUtf8(i->name).replace(' ', '_');
    s.description = QString::fromUtf8(i->description);
    s.icon_name = QString::fromUtf8(pa_proplist_gets(i->proplist, PA_PROP_DEVICE_ICON_NAME));
    s.volume = i->volume;
    s.channel_map = i->channel_map;
    s.mute = !!i->mute;
    s.stream_restore_rule = "";

    translateMasksAndMaps(s);

    bool is_new = !captureDevices.contains(s.index);
    captureDevices[s.index] = s;
//     kDebug(67100) << "Got some info about source: " << s.description;

    if (s_mixers.contains(KMIXPA_CAPTURE)) {
        if (is_new)
            s_mixers[KMIXPA_CAPTURE]->addWidget(s.index);
        else {
            int mid = s_mixers[KMIXPA_CAPTURE]->id2num(s.name);
            if (mid >= 0) {
                MixSet *ms = s_mixers[KMIXPA_CAPTURE]->getMixSet();
                (*ms)[mid]->setReadableName(s.description);
            }
        }
    }
}

static void client_cb(pa_context *c, const pa_client_info *i, int eol, void *) {

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Client callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);
        return;
    }

    clients[i->index] = QString::fromUtf8(i->name);
    //kDebug(67100) << "Got some info about client: " << clients[i->index];
}

static void sink_input_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *) {

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Sink Input callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);
        if (s_mixers.contains(KMIXPA_APP_PLAYBACK))
            s_mixers[KMIXPA_APP_PLAYBACK]->triggerUpdate();
        return;
    }

    const char *t;
    if ((t = pa_proplist_gets(i->proplist, "module-stream-restore.id"))) {
        if (strcmp(t, KMIXPA_EVENT_KEY) == 0) {
            kWarning(67100) << "Ignoring sink-input due to it being designated as an event and thus handled by the Event slider";
            return;
        }
    }

    QString appname = i18n("Unknown Application");
    if (clients.contains(i->client))
        appname = clients[i->client];

    QString prefix = QString("%1: ").arg(appname);

    devinfo s;
    s.index = i->index;
    s.device_index = i->sink;
    s.description = prefix + QString::fromUtf8(i->name);
    s.name = QString("stream:") + QString::number(i->index); //appname.replace(' ', '_').toLower();
    s.icon_name = getIconNameFromProplist(i->proplist);
    s.channel_map = i->channel_map;
    s.volume = i->volume;
    s.mute = !!i->mute;
    s.stream_restore_rule = QString::fromUtf8(t);

    translateMasksAndMaps(s);

    bool is_new = !outputStreams.contains(s.index);
    outputStreams[s.index] = s;
//     kDebug(67100) << "Got some info about sink input (playback stream): " << s.description;

    if (s_mixers.contains(KMIXPA_APP_PLAYBACK)) {
        if (is_new)
            s_mixers[KMIXPA_APP_PLAYBACK]->addWidget(s.index, true);
        else {
            int mid = s_mixers[KMIXPA_APP_PLAYBACK]->id2num(s.name);
            if (mid >= 0) {
                MixSet *ms = s_mixers[KMIXPA_APP_PLAYBACK]->getMixSet();
                (*ms)[mid]->setReadableName(s.description);
            }
        }
    }
}

static void source_output_cb(pa_context *c, const pa_source_output_info *i, int eol, void *) {

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        kWarning(67100) << "Source Output callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);
        if (s_mixers.contains(KMIXPA_APP_CAPTURE))
            s_mixers[KMIXPA_APP_CAPTURE]->triggerUpdate();
        return;
    }

    /* NB Until Source Outputs support volumes, we just use the volume of the source itself */
    if (!captureDevices.contains(i->source)) {
        kDebug(67100) << "Source Output refers to a Source we don't have any info for (probably just a peak meter or similar)";
        return;
    }

    QString appname = i18n("Unknown Application");
    if (clients.contains(i->client))
        appname = clients[i->client];

    QString prefix = QString("%1: ").arg(appname);

    devinfo s;
    s.index = i->index;
    s.device_index = i->source;
    s.description = prefix + QString::fromUtf8(i->name);
    s.name = QString("stream:") + QString::number(i->index); //appname.replace(' ', '_').toLower();
    s.icon_name = getIconNameFromProplist(i->proplist);
    s.channel_map = i->channel_map;
#if HAVE_SOURCE_OUTPUT_VOLUMES
    s.volume = i->volume;
    s.mute = !!i->mute;
#else
    s.volume = captureDevices[i->source].volume;
    s.mute = captureDevices[i->source].mute;
#endif
    s.stream_restore_rule = QString::fromUtf8(pa_proplist_gets(i->proplist, "module-stream-restore.id"));

    translateMasksAndMaps(s);

    bool is_new = !captureStreams.contains(s.index);
    captureStreams[s.index] = s;
//     kDebug(67100) << "Got some info about source output (capture stream): " << s.description;

    if (s_mixers.contains(KMIXPA_APP_CAPTURE)) {
        if (is_new)
            s_mixers[KMIXPA_APP_CAPTURE]->addWidget(s.index, true);
        else {
            int mid = s_mixers[KMIXPA_APP_CAPTURE]->id2num(s.name);
            if (mid >= 0) {
                MixSet *ms = s_mixers[KMIXPA_APP_CAPTURE]->getMixSet();
                (*ms)[mid]->setReadableName(s.description);
            }
        }
    }
}


static devinfo create_role_devinfo(QString name) {

    Q_ASSERT(s_RestoreRules.contains(name));

    devinfo s;
    s.index = s.device_index = PA_INVALID_INDEX;
    s.description = i18n("Event Sounds");
    s.name = QString("restore:") + name;
    s.icon_name = "dialog-information";
    s.channel_map = s_RestoreRules[name].channel_map;
    s.volume = s_RestoreRules[name].volume;
    s.mute = s_RestoreRules[name].mute;
    s.stream_restore_rule = name;

    translateMasksAndMaps(s);
    return s;
}


void ext_stream_restore_read_cb(pa_context *c, const pa_ext_stream_restore_info *i, int eol, void *) {

    if (eol < 0) {
        dec_outstanding(c);
        kWarning(67100) << "Failed to initialize stream_restore extension: " << pa_strerror(pa_context_errno(s_context));
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);

        // Special case: ensure that our media events exists.
        // On first login by a new users, this wont be in our database so we should create it.
        if (!s_RestoreRules.contains(KMIXPA_EVENT_KEY)) {
            // Create a fake rule
            restoreRule rule;
            rule.channel_map.channels = 1;
            rule.channel_map.map[0] = PA_CHANNEL_POSITION_MONO;
            rule.volume.channels = 1;
            rule.volume.values[0] = PA_VOLUME_NORM;
            rule.mute = false;
            rule.device = "";
            s_RestoreRules[KMIXPA_EVENT_KEY] = rule;
            kDebug(67100) << "Initialising restore rule for new user: " << i18n("Event Sounds");
        }

        if (s_mixers.contains(KMIXPA_APP_PLAYBACK)) {
            // If we have rules, it will be created below... but if no rules
            // then we add it here.
            if (!outputRoles.contains(PA_INVALID_INDEX)) {
                devinfo s = create_role_devinfo(KMIXPA_EVENT_KEY);
                outputRoles[s.index] = s;

                s_mixers[KMIXPA_APP_PLAYBACK]->addWidget(s.index);
            }

            s_mixers[KMIXPA_APP_PLAYBACK]->triggerUpdate();
        }

        return;
    }


    QString name = QString::fromUtf8(i->name);
//     kDebug(67100) << QString("Got some info about restore rule: '%1' (Device: %2)").arg(name).arg(i->device ? i->device : "None");
    restoreRule rule;
    rule.channel_map = i->channel_map;
    rule.volume = i->volume;
    rule.mute = !!i->mute;
    rule.device = i->device;

    if (rule.channel_map.channels < 1 && name == KMIXPA_EVENT_KEY) {
        // Stream restore rules may not have valid volumes/channel maps (as these are optional)
        // but we need a valid volume+channelmap for our events sounds so fix it up.
        rule.channel_map.channels = 1;
        rule.channel_map.map[0] = PA_CHANNEL_POSITION_MONO;
        rule.volume.channels = 1;
        rule.volume.values[0] = PA_VOLUME_NORM;
    }

    s_RestoreRules[name] = rule;

    if (s_mixers.contains(KMIXPA_APP_PLAYBACK)) {
        // We only want to know about Sound Events for now...
        if (name == KMIXPA_EVENT_KEY) {
            devinfo s = create_role_devinfo(name);
            bool is_new = !outputRoles.contains(s.index);
            outputRoles[s.index] = s;

            if (is_new)
                s_mixers[KMIXPA_APP_PLAYBACK]->addWidget(s.index, true);
        }
    }
}

static void ext_stream_restore_subscribe_cb(pa_context *c, void *) {

    Q_ASSERT(c == s_context);

    pa_operation *o;
    if (!(o = pa_ext_stream_restore_read(c, ext_stream_restore_read_cb, NULL))) {
        kWarning(67100) << "pa_ext_stream_restore_read() failed";
        return;
    }

    pa_operation_unref(o);
}


static void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *) {

    Q_ASSERT(c == s_context);

    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                if (s_mixers.contains(KMIXPA_PLAYBACK))
                    s_mixers[KMIXPA_PLAYBACK]->removeWidget(index);
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
                if (s_mixers.contains(KMIXPA_CAPTURE))
                    s_mixers[KMIXPA_CAPTURE]->removeWidget(index);
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_source_info_by_index(c, index, source_cb, NULL))) {
                    kWarning(67100) << "pa_context_get_source_info_by_index() failed";
                    return;
                }
                pa_operation_unref(o);
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                if (s_mixers.contains(KMIXPA_APP_PLAYBACK))
                    s_mixers[KMIXPA_APP_PLAYBACK]->removeWidget(index);
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_sink_input_info(c, index, sink_input_cb, NULL))) {
                    kWarning(67100) << "pa_context_get_sink_input_info() failed";
                    return;
                }
                pa_operation_unref(o);
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                if (s_mixers.contains(KMIXPA_APP_CAPTURE))
                    s_mixers[KMIXPA_APP_CAPTURE]->removeWidget(index);
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_source_output_info(c, index, source_output_cb, NULL))) {
                    kWarning(67100) << "pa_context_get_sink_input_info() failed";
                    return;
                }
                pa_operation_unref(o);
            }
            break;

        case PA_SUBSCRIPTION_EVENT_CLIENT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                clients.remove(index);
            } else {
                pa_operation *o;
                if (!(o = pa_context_get_client_info(c, index, client_cb, NULL))) {
                    kWarning(67100) << "pa_context_get_client_info() failed";
                    return;
                }
                pa_operation_unref(o);
            }
            break;

    }
}


static void context_state_callback(pa_context *c, void *)
{
    pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_READY) {
        // Attempt to load things up
        pa_operation *o;

        // 1. Register for the stream changes (except during probe)
        if (s_context == c) {
            pa_context_set_subscribe_callback(c, subscribe_cb, NULL);

            if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)
                                           (PA_SUBSCRIPTION_MASK_SINK|
                                            PA_SUBSCRIPTION_MASK_SOURCE|
                                            PA_SUBSCRIPTION_MASK_CLIENT|
                                            PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                            PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT), NULL, NULL))) {
                kWarning(67100) << "pa_context_subscribe() failed";
                return;
            }
            pa_operation_unref(o);
        }

        if (!(o = pa_context_get_sink_info_list(c, sink_cb, NULL))) {
            kWarning(67100) << "pa_context_get_sink_info_list() failed";
            return;
        }
        pa_operation_unref(o);
        s_outstandingRequests++;

        if (!(o = pa_context_get_source_info_list(c, source_cb, NULL))) {
            kWarning(67100) << "pa_context_get_source_info_list() failed";
            return;
        }
        pa_operation_unref(o);
        s_outstandingRequests++;


        if (!(o = pa_context_get_client_info_list(c, client_cb, NULL))) {
            kWarning(67100) << "pa_context_client_info_list() failed";
            return;
        }
        pa_operation_unref(o);
        s_outstandingRequests++;

        if (!(o = pa_context_get_sink_input_info_list(c, sink_input_cb, NULL))) {
            kWarning(67100) << "pa_context_get_sink_input_info_list() failed";
            return;
        }
        pa_operation_unref(o);
        s_outstandingRequests++;

        if (!(o = pa_context_get_source_output_info_list(c, source_output_cb, NULL))) {
            kWarning(67100) << "pa_context_get_source_output_info_list() failed";
            return;
        }
        pa_operation_unref(o);
        s_outstandingRequests++;

        /* These calls are not always supported */
        if ((o = pa_ext_stream_restore_read(c, ext_stream_restore_read_cb, NULL))) {
            pa_operation_unref(o);
            s_outstandingRequests++;

            pa_ext_stream_restore_set_subscribe_cb(c, ext_stream_restore_subscribe_cb, NULL);

            if ((o = pa_ext_stream_restore_subscribe(c, 1, NULL, NULL)))
                pa_operation_unref(o);
        } else {
            kWarning(67100) << "Failed to initialize stream_restore extension: " << pa_strerror(pa_context_errno(s_context));
        }
    } else if (!PA_CONTEXT_IS_GOOD(state)) {
        // If this is our probe phase, exit our context immediately
        if (s_context != c) {
            pa_context_disconnect(c);
        } else {
            // If we're not probing, it means we've been disconnected from our
            // glib context
            pa_context_unref(s_context);
            s_context = NULL;

            // Remove all GUI elements
            QMap<int,Mixer_PULSE*>::iterator it;
            for (it = s_mixers.begin(); it != s_mixers.end(); ++it) {
                (*it)->removeAllWidgets();
            }
            // This one is not handled above.
            clients.clear();

            if (s_mixers.contains(KMIXPA_PLAYBACK)) {
                kWarning(67100) << "Connection to PulseAudio daemon closed. Attempting reconnection.";
                s_pulseActive = UNKNOWN;
                QTimer::singleShot(50, s_mixers[KMIXPA_PLAYBACK], SLOT(reinit()));
            }
        }
    }
}

static void setVolumeFromPulse(Volume& volume, const devinfo& dev)
{
    chanIDMap::const_iterator iter;
    for (iter = dev.chanIDs.begin(); iter != dev.chanIDs.end(); ++iter)
    {
        //kDebug(67100) <<  "Setting volume for channel " << iter.value() << " to " << (long)dev.volume.values[iter.key()] << " (" << ((100*(long)dev.volume.values[iter.key()]) / PA_VOLUME_NORM) << "%)";
        volume.setVolume(iter.value(), (long)dev.volume.values[iter.key()]);
    }
}

static pa_cvolume genVolumeForPulse(const devinfo& dev, Volume& volume)
{
    pa_cvolume cvol = dev.volume;

    chanIDMap::const_iterator iter;
    for (iter = dev.chanIDs.begin(); iter != dev.chanIDs.end(); ++iter)
    {
        cvol.values[iter.key()] = (uint32_t)volume.getVolume(iter.value());
        //kDebug(67100) <<  "Setting volume for channel " << iter.value() << " to " << cvol.values[iter.key()] << " (" << ((100*cvol.values[iter.key()]) / PA_VOLUME_NORM) << "%)";
    }
    return cvol;
}

static devmap* get_widget_map(int type, QString id = "")
{
    Q_ASSERT(type >= 0 && type <= KMIXPA_WIDGET_MAX);

    if (KMIXPA_PLAYBACK == type)
        return &outputDevices;
    else if (KMIXPA_CAPTURE == type)
        return &captureDevices;
    else if (KMIXPA_APP_PLAYBACK == type) {
        if (id.startsWith("restore:"))
            return &outputRoles;
        return &outputStreams;
    } else if (KMIXPA_APP_CAPTURE == type)
        return &captureStreams;

    Q_ASSERT(0);
    return NULL;
}
static devmap* get_widget_map(int type, int index)
{
    if (PA_INVALID_INDEX == (uint32_t)index)
        return get_widget_map(type, "restore:");
    return get_widget_map(type);
}

void Mixer_PULSE::emitControlsReconfigured()
{
    ControlManager::instance().announce(_mixer->id(), ControlChangeType::ControlList, getDriverName());
}

void Mixer_PULSE::addWidget(int index, bool isAppStream)
{
    devmap* map = get_widget_map(m_devnum, index);

    if (!map->contains(index)) {
        kWarning(67100) <<  "New " << m_devnum << " widget notified for index " << index << " but I cannot find it in my list :s";
        return;
    }
    addDevice((*map)[index], isAppStream);
    emitControlsReconfigured();
}

void Mixer_PULSE::removeWidget(int index)
{
    devmap* map = get_widget_map(m_devnum);

    if (!map->contains(index)) {
        //kWarning(67100) <<  "Removing " << m_devnum << " widget notified for index " << index << " but I cannot find it in my list :s";
        // Sometimes we ignore things (e.g. event sounds) so don't be too noisy here.
        return;
    }

    QString id = (*map)[index].name;
    map->remove(index);

    // We need to find the MixDevice that goes with this widget and remove it.
    MixSet::iterator iter;
    for (iter = m_mixDevices.begin(); iter != m_mixDevices.end(); ++iter)
    {
        if ((*iter)->id() == id)
        {
			shared_ptr<MixDevice> md = m_mixDevices.get(id);
			kDebug() << "MixDevice 1 useCount=" << md.use_count();
			md->close();
			kDebug() << "MixDevice 2 useCount=" << md.use_count();

            m_mixDevices.erase(iter);
			kDebug() << "MixDevice 3 useCount=" << md.use_count();
            emitControlsReconfigured();
			kDebug() << "MixDevice 4 useCount=" << md.use_count();
            return;
        }
    }
}

void Mixer_PULSE::removeAllWidgets()
{
    devmap* map = get_widget_map(m_devnum);
    map->clear();

    // Special case
    if (KMIXPA_APP_PLAYBACK == m_devnum)
        outputRoles.clear();

    freeMixDevices();
    emitControlsReconfigured();
}

void Mixer_PULSE::addDevice(devinfo& dev, bool isAppStream)
{
    if (dev.chanMask != Volume::MNONE) {
        MixSet *ms = 0;
        if (m_devnum == KMIXPA_APP_PLAYBACK && s_mixers.contains(KMIXPA_PLAYBACK))
            ms = s_mixers[KMIXPA_PLAYBACK]->getMixSet();
        else if (m_devnum == KMIXPA_APP_CAPTURE && s_mixers.contains(KMIXPA_CAPTURE))
            ms = s_mixers[KMIXPA_CAPTURE]->getMixSet();

        int maxVol = GlobalConfig::instance().volumeOverdrive ? PA_VOLUME_UI_MAX : PA_VOLUME_NORM;
        Volume v(maxVol, PA_VOLUME_MUTED, true, false);
        v.addVolumeChannels(dev.chanMask);
        setVolumeFromPulse(v, dev);
        MixDevice* md = new MixDevice( _mixer, dev.name, dev.description, dev.icon_name, ms);
        if (isAppStream)
            md->setApplicationStream(true);

        kDebug() << "Adding Pulse volume " << dev.name << ", isCapture= " << (m_devnum == KMIXPA_CAPTURE || m_devnum == KMIXPA_APP_CAPTURE) << ", isAppStream= " << isAppStream << "=" << md->isApplicationStream() << ", devnum=" << m_devnum;
        md->addPlaybackVolume(v);
        md->setMuted(dev.mute);
        m_mixDevices.append(md->addToPool());
    }
}

Mixer_Backend* PULSE_getMixer( Mixer *mixer, int devnum )
{
   Mixer_Backend *l_mixer;
   l_mixer = new Mixer_PULSE( mixer, devnum );
   return l_mixer;
}

bool Mixer_PULSE::connectToDaemon()
{
    Q_ASSERT(NULL == s_context);

    kDebug(67100) <<  "Attempting connection to PulseAudio sound daemon";
    pa_mainloop_api *api = pa_glib_mainloop_get_api(s_mainloop);
    Q_ASSERT(api);

    s_context = pa_context_new(api, "KMix");
    Q_ASSERT(s_context);

    if (pa_context_connect(s_context, NULL, PA_CONTEXT_NOFAIL, 0) < 0) {
        pa_context_unref(s_context);
        s_context = NULL;
        return false;
    }
    pa_context_set_state_callback(s_context, &context_state_callback, NULL);
    return true;
}


Mixer_PULSE::Mixer_PULSE(Mixer *mixer, int devnum) : Mixer_Backend(mixer, devnum)
{
    if ( devnum == -1 )
        m_devnum = 0;

    QString pulseenv = qgetenv("KMIX_PULSEAUDIO_DISABLE");
    if (pulseenv.toInt())
        s_pulseActive = INACTIVE;

    // We require a glib event loop
    if (!QByteArray(QAbstractEventDispatcher::instance()->metaObject()->className()).contains("EventDispatcherGlib")) {
        kDebug(67100) << "Disabling PulseAudio integration for lack of GLib event loop";
        s_pulseActive = INACTIVE;
    }


    ++refcount;
    if (INACTIVE != s_pulseActive && 1 == refcount)
    {
        // First of all conenct to PA via simple/blocking means and if that succeeds,
        // use a fully async integrated mainloop method to connect and get proper support.
        pa_mainloop *p_test_mainloop;
        if (!(p_test_mainloop = pa_mainloop_new())) {
            kDebug(67100) << "PulseAudio support disabled: Unable to create mainloop";
            s_pulseActive = INACTIVE;
            goto endconstruct;
        }

        pa_context *p_test_context;
        if (!(p_test_context = pa_context_new(pa_mainloop_get_api(p_test_mainloop), "kmix-probe"))) {
            kDebug(67100) << "PulseAudio support disabled: Unable to create context";
            pa_mainloop_free(p_test_mainloop);
            s_pulseActive = INACTIVE;
            goto endconstruct;
        }

        kDebug(67100) << "Probing for PulseAudio...";
        // (cg) Convert to PA_CONTEXT_NOFLAGS when PulseAudio 0.9.19 is required
        if (pa_context_connect(p_test_context, NULL, static_cast<pa_context_flags_t>(0), NULL) < 0) {
            kDebug(67100) << QString("PulseAudio support disabled: %1").arg(pa_strerror(pa_context_errno(p_test_context)));
            pa_context_disconnect(p_test_context);
            pa_context_unref(p_test_context);
            pa_mainloop_free(p_test_mainloop);
            s_pulseActive = INACTIVE;
            goto endconstruct;
        }

        // Assume we are inactive, it will be set to active if appropriate
        s_pulseActive = INACTIVE;
        pa_context_set_state_callback(p_test_context, &context_state_callback, NULL);
        for (;;) {
          pa_mainloop_iterate(p_test_mainloop, 1, NULL);

          if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(p_test_context))) {
            kDebug(67100) << "PulseAudio probe complete.";
            break;
          }
        }
        pa_context_disconnect(p_test_context);
        pa_context_unref(p_test_context);
        pa_mainloop_free(p_test_mainloop);


        if (INACTIVE != s_pulseActive)
        {
            // Reconnect via integrated mainloop
            s_mainloop = pa_glib_mainloop_new(NULL);
            Q_ASSERT(s_mainloop);

            connectToDaemon();

#if defined(HAVE_CANBERRA)
            int ret = ca_context_create(&s_ccontext);
            if (ret < 0) {
                kDebug(67100) << "Disabling Sound Feedback. Canberra context failed.";
                s_ccontext = NULL;
            } else
                ca_context_set_driver(s_ccontext, "pulse");
#endif
        }

        kDebug(67100) <<  "PulseAudio status: " << (s_pulseActive==UNKNOWN ? "Unknown (bug)" : (s_pulseActive==ACTIVE ? "Active" : "Inactive"));
    }

endconstruct:
    s_mixers[m_devnum] = this;
}

Mixer_PULSE::~Mixer_PULSE()
{
    s_mixers.remove(m_devnum);

    if (refcount > 0)
    {
        --refcount;
        if (0 == refcount)
        {
#if defined(HAVE_CANBERRA)
            if (s_ccontext) {
                ca_context_destroy(s_ccontext);
                s_ccontext = NULL;
            }
#endif

            if (s_context) {
                pa_context_unref(s_context);
                s_context = NULL;
            }

            if (s_mainloop) {
                pa_glib_mainloop_free(s_mainloop);
                s_mainloop = NULL;
            }
        }
    }

    closeCommon();
}

int Mixer_PULSE::open()
{
    //kDebug(67100) <<  "Trying Pulse sink";

    if (ACTIVE == s_pulseActive && m_devnum <= KMIXPA_APP_CAPTURE)
    {
        // Make sure the GUI layers know we are dynamic so as to always paint us
        _mixer->setDynamic();

        devmap::iterator iter;
        if (KMIXPA_PLAYBACK == m_devnum)
        {
        	_id = "Playback Devices";
            m_mixerName = i18n("Playback Devices");
            for (iter = outputDevices.begin(); iter != outputDevices.end(); ++iter)
                addDevice(*iter);
        }
        else if (KMIXPA_CAPTURE == m_devnum)
        {
        	_id = "Capture Devices";
            m_mixerName = i18n("Capture Devices");
            for (iter = captureDevices.begin(); iter != captureDevices.end(); ++iter)
                addDevice(*iter);
        }
        else if (KMIXPA_APP_PLAYBACK == m_devnum)
        {
        	_id = "Playback Streams";
            m_mixerName = i18n("Playback Streams");
            for (iter = outputRoles.begin(); iter != outputRoles.end(); ++iter)
                addDevice(*iter, true);
            for (iter = outputStreams.begin(); iter != outputStreams.end(); ++iter)
                addDevice(*iter, true);
        }
        else if (KMIXPA_APP_CAPTURE == m_devnum)
        {
        	_id = "Capture Streams";
            m_mixerName = i18n("Capture Streams");
            for (iter = captureStreams.begin(); iter != captureStreams.end(); ++iter)
                addDevice(*iter);
        }

        kDebug(67100) <<  "Using PulseAudio for mixer: " << m_mixerName;
        m_isOpen = true;
    }

    return 0;
}

int Mixer_PULSE::close()
{
	closeCommon();
    return 1;
}

int Mixer_PULSE::id2num(const QString& id) {
    //kDebug(67100) << "id2num() id=" << id;
    int num = -1;
    // todo: Store this in a hash or similar
    int i;
    for (i = 0; i < m_mixDevices.size(); ++i) {
        if (m_mixDevices[i]->id() == id) {
            num = i;
            break;
        }
    }
    //kDebug(67100) << "id2num() num=" << num;
    return num;
}

int Mixer_PULSE::readVolumeFromHW( const QString& id, shared_ptr<MixDevice> md )
{
    devmap *map = get_widget_map(m_devnum, id);

    devmap::iterator iter;
    for (iter = map->begin(); iter != map->end(); ++iter)
    {
        if (iter->name == id)
        {
            setVolumeFromPulse(md->playbackVolume(), *iter);
            md->setMuted(iter->mute);
            break;
        }
    }

    return 0;
}

int Mixer_PULSE::writeVolumeToHW( const QString& id, shared_ptr<MixDevice> md )
{
    devmap::iterator iter;
    if (KMIXPA_PLAYBACK == m_devnum)
    {
        for (iter = outputDevices.begin(); iter != outputDevices.end(); ++iter)
        {
            if (iter->name == id)
            {
                pa_operation *o;

                pa_cvolume volume = genVolumeForPulse(*iter, md->playbackVolume());
                if (!(o = pa_context_set_sink_volume_by_index(s_context, iter->index, &volume, NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_sink_volume_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);

                if (!(o = pa_context_set_sink_mute_by_index(s_context, iter->index, (md->isMuted() ? 1 : 0), NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_sink_mute_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);

#if defined(HAVE_CANBERRA)
                if (s_ccontext && Mixer::getBeepOnVolumeChange() ) {
                    int playing = 0;
                    int cindex = 2; // Note "2" is simply the index we've picked. It's somewhat irrelevant.

                    
                    ca_context_playing(s_ccontext, cindex, &playing);

                    // NB Depending on how this is desired to work, we may want to simply
                    // skip playing, or cancel the currently playing sound and play our
                    // new one... for now, let's do the latter.
                    if (playing) {
                        ca_context_cancel(s_ccontext, cindex);
                        playing = 0;
                    }
                    
                    if (!playing) {
                        char dev[64];

                        snprintf(dev, sizeof(dev), "%lu", (unsigned long) iter->index);
                        ca_context_change_device(s_ccontext, dev);

                        // Ideally we'd use something like ca_gtk_play_for_widget()...
                        ca_context_play(
                            s_ccontext,
                            cindex,
                            CA_PROP_EVENT_DESCRIPTION, i18n("Volume Control Feedback Sound").toUtf8().constData(),
                            CA_PROP_EVENT_ID, "audio-volume-change",
                            CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                            CA_PROP_CANBERRA_ENABLE, "1",
                            NULL
                        );

                        ca_context_change_device(s_ccontext, NULL);
                    }
                }
#endif

                return 0;
            }
        }
    }
    else if (KMIXPA_CAPTURE == m_devnum)
    {
        for (iter = captureDevices.begin(); iter != captureDevices.end(); ++iter)
        {
            if (iter->name == id)
            {
                pa_operation *o;

                pa_cvolume volume = genVolumeForPulse(*iter, md->playbackVolume());
                if (!(o = pa_context_set_source_volume_by_index(s_context, iter->index, &volume, NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_source_volume_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);

                if (!(o = pa_context_set_source_mute_by_index(s_context, iter->index, (md->isMuted() ? 1 : 0), NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_source_mute_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);

                return 0;
            }
        }
    }
    else if (KMIXPA_APP_PLAYBACK == m_devnum)
    {
        if (id.startsWith("stream:"))
        {
            for (iter = outputStreams.begin(); iter != outputStreams.end(); ++iter)
            {
                if (iter->name == id)
                {
                    pa_operation *o;

                    pa_cvolume volume = genVolumeForPulse(*iter, md->playbackVolume());
                    if (!(o = pa_context_set_sink_input_volume(s_context, iter->index, &volume, NULL, NULL))) {
                        kWarning(67100) <<  "pa_context_set_sink_input_volume() failed";
                        return Mixer::ERR_READ;
                    }
                    pa_operation_unref(o);

                    if (!(o = pa_context_set_sink_input_mute(s_context, iter->index, (md->isMuted() ? 1 : 0), NULL, NULL))) {
                        kWarning(67100) <<  "pa_context_set_sink_input_mute() failed";
                        return Mixer::ERR_READ;
                    }
                    pa_operation_unref(o);

                    return 0;
                }
            }
        }
        else if (id.startsWith("restore:"))
        {
            for (iter = outputRoles.begin(); iter != outputRoles.end(); ++iter)
            {
                if (iter->name == id)
                {
                    restoreRule &rule = s_RestoreRules[iter->stream_restore_rule];
                    pa_ext_stream_restore_info info;
                    info.name = iter->stream_restore_rule.toUtf8().constData();
                    info.channel_map = rule.channel_map;
                    info.volume = genVolumeForPulse(*iter, md->playbackVolume());
                    info.device = rule.device.isEmpty() ? NULL : rule.device.toUtf8().constData();
                    info.mute = (md->isMuted() ? 1 : 0);

                    pa_operation* o;
                    if (!(o = pa_ext_stream_restore_write(s_context, PA_UPDATE_REPLACE, &info, 1, true, NULL, NULL))) {
                        kWarning(67100) <<  "pa_ext_stream_restore_write() failed" << info.channel_map.channels << info.volume.channels << info.name;
                        return Mixer::ERR_READ;
                    }
                    pa_operation_unref(o);

                    return 0;
                }
            }
        }
    }
    else if (KMIXPA_APP_CAPTURE == m_devnum)
    {
        for (iter = captureStreams.begin(); iter != captureStreams.end(); ++iter)
        {
            if (iter->name == id)
            {
                pa_operation *o;

#if HAVE_SOURCE_OUTPUT_VOLUMES
                pa_cvolume volume = genVolumeForPulse(*iter, md->playbackVolume());
                if (!(o = pa_context_set_source_output_volume(s_context, iter->index, &volume, NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_source_output_volume_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);

                if (!(o = pa_context_set_source_output_mute(s_context, iter->index, (md->isMuted() ? 1 : 0), NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_source_output_mute_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);
#else                
                // NB Note that this is different from APP_PLAYBACK in that we set the volume on the source itself.
                pa_cvolume volume = genVolumeForPulse(*iter, md->playbackVolume());
                if (!(o = pa_context_set_source_volume_by_index(s_context, iter->device_index, &volume, NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_source_volume_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);

                if (!(o = pa_context_set_source_mute_by_index(s_context, iter->device_index, (md->isMuted() ? 1 : 0), NULL, NULL))) {
                    kWarning(67100) <<  "pa_context_set_source_mute_by_index() failed";
                    return Mixer::ERR_READ;
                }
                pa_operation_unref(o);
#endif

                return 0;
            }
        }
    }

    return 0;
}

/**
* Move the stream to a new destination
*/
bool Mixer_PULSE::moveStream( const QString& id, const QString& destId ) {
    Q_ASSERT(KMIXPA_APP_PLAYBACK == m_devnum || KMIXPA_APP_CAPTURE == m_devnum);

    kDebug(67100) <<  "Mixer_PULSE::moveStream(): Move Stream Requested - Stream: " << id << ", Destination: " << destId;

    // Lookup the stream index.
    uint32_t stream_index = PA_INVALID_INDEX;
    QString stream_restore_rule = "";
    devmap::iterator iter;
    devmap *map = get_widget_map(m_devnum);
    for (iter = map->begin(); iter != map->end(); ++iter) {
        if (iter->name == id) {
            stream_index = iter->index;
            stream_restore_rule = iter->stream_restore_rule;
            break;
        }
    }

    if (PA_INVALID_INDEX == stream_index) {
        kError(67100) <<  "Mixer_PULSE::moveStream(): Cannot find stream index";
        return false;
    }

    if (destId.isEmpty()) {
        // We want to remove any specific device in the stream restore rule.
        if (stream_restore_rule.isEmpty() || !s_RestoreRules.contains(stream_restore_rule)) {
            kWarning(67100) <<  "Mixer_PULSE::moveStream(): Trying to set Automatic on a stream with no rule";
        } else {
            restoreRule &rule = s_RestoreRules[stream_restore_rule];
            pa_ext_stream_restore_info info;
            info.name = stream_restore_rule.toUtf8().constData();
            info.channel_map = rule.channel_map;
            info.volume = rule.volume;
            info.device = NULL;
            info.mute = rule.mute ? 1 : 0;

            pa_operation* o;
            if (!(o = pa_ext_stream_restore_write(s_context, PA_UPDATE_REPLACE, &info, 1, true, NULL, NULL))) {
                kWarning(67100) <<  "pa_ext_stream_restore_write() failed" << info.channel_map.channels << info.volume.channels << info.name;
                return Mixer::ERR_READ;
            }
            pa_operation_unref(o);
        }
    } else {
        pa_operation* o;
        if (KMIXPA_APP_PLAYBACK == m_devnum) {
            if (!(o = pa_context_move_sink_input_by_name(s_context, stream_index, destId.toUtf8().constData(), NULL, NULL))) {
                kWarning(67100) <<  "pa_context_move_sink_input_by_name() failed";
                return false;
            }
        } else {
            if (!(o = pa_context_move_source_output_by_name(s_context, stream_index, destId.toUtf8().constData(), NULL, NULL))) {
                kWarning(67100) <<  "pa_context_move_source_output_by_name() failed";
                return false;
            }
        }
        pa_operation_unref(o);
    }

    return true;
}

void Mixer_PULSE::reinit()
{
    // We only support reinit on our primary mixer.
    Q_ASSERT(KMIXPA_PLAYBACK == m_devnum);
    connectToDaemon();
}

void Mixer_PULSE::triggerUpdate()
{
    readSetFromHWforceUpdate();
    readSetFromHW();
}

// Please see KMixWindow::initActionsAfterInitMixer(), it uses the driverName

QString PULSE_getDriverName() {
        return "PulseAudio";
}

QString Mixer_PULSE::getDriverName()
{
        return "PulseAudio";
}

