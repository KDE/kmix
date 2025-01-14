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

#include "qtpamainloop.h"

#include <QStringBuilder>

#include <klocalizedstring.h>

#include "core/mixer.h"
#include "core/ControlManager.h"
#include "settings.h"

#include <pulse/ext-stream-restore.h>

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


typedef QMap<uint8_t,Volume::ChannelID> chanIDMap;

struct devinfo
{
    int index;
    int device_index;
    QString name;
    QString description;
    QString icon_name;
    pa_cvolume volume;
    pa_channel_map channel_map;
    bool mute;
    QString stream_restore_rule;

    Volume::ChannelMask chanMask;
    chanIDMap chanIDs;
    unsigned int priority;
};


static unsigned int refcount = 0;
static pa_context *s_context = nullptr;
static enum { UNKNOWN, ACTIVE, INACTIVE } s_pulseActive = UNKNOWN;
static int s_outstandingRequests = 0;

static QMap<int,Mixer_PULSE *> s_mixers;

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
          qCDebug(KMIX_LOG) <<  "Reconnected to PulseAudio";
    }
}

static void translateMasksAndMaps(devinfo& dev)
{
    dev.chanMask = Volume::MNONE;
    dev.chanIDs.clear();

    if (dev.channel_map.channels != dev.volume.channels) {
        qCCritical(KMIX_LOG) << "Hideous Channel mixup map says " << dev.channel_map.channels << ", volume says: " << dev.volume.channels;
        return;
    }
    if (1 == dev.channel_map.channels && PA_CHANNEL_POSITION_MONO == dev.channel_map.map[0]) {
        // We just use the left channel to represent this.
        dev.chanMask |= Volume::MLEFT;
        dev.chanIDs[0] = Volume::LEFT;
    } else {
        for (uint8_t i = 0; i < dev.channel_map.channels; ++i) {
            switch (dev.channel_map.map[i]) {
                case PA_CHANNEL_POSITION_MONO:
                    qCWarning(KMIX_LOG) << "Channel Map contains a MONO element but has >1 channel - we can't handle this.";
                    return;

                case PA_CHANNEL_POSITION_FRONT_LEFT:
                    dev.chanMask |= Volume::MLEFT;
                    dev.chanIDs[i] = Volume::LEFT;
                    break;
                case PA_CHANNEL_POSITION_FRONT_RIGHT:
                    dev.chanMask |= Volume::MRIGHT;
                    dev.chanIDs[i] = Volume::RIGHT;
                    break;
                case PA_CHANNEL_POSITION_FRONT_CENTER:
                    dev.chanMask |= Volume::MCENTER;
                    dev.chanIDs[i] = Volume::CENTER;
                    break;
                case PA_CHANNEL_POSITION_REAR_CENTER:
                    dev.chanMask |= Volume::MREARCENTER;
                    dev.chanIDs[i] = Volume::REARCENTER;
                    break;
                case PA_CHANNEL_POSITION_REAR_LEFT:
                    dev.chanMask |= Volume::MSURROUNDLEFT;
                    dev.chanIDs[i] = Volume::SURROUNDLEFT;
                    break;
                case PA_CHANNEL_POSITION_REAR_RIGHT:
                    dev.chanMask |= Volume::MSURROUNDRIGHT;
                    dev.chanIDs[i] = Volume::SURROUNDRIGHT;
                    break;
                case PA_CHANNEL_POSITION_LFE:
                    dev.chanMask |= Volume::MWOOFER;
                    dev.chanIDs[i] = Volume::WOOFER;
                    break;
                case PA_CHANNEL_POSITION_SIDE_LEFT:
                    dev.chanMask |= Volume::MREARSIDELEFT;
                    dev.chanIDs[i] = Volume::REARSIDELEFT;
                    break;
                case PA_CHANNEL_POSITION_SIDE_RIGHT:
                    dev.chanMask |= Volume::MREARSIDERIGHT;
                    dev.chanIDs[i] = Volume::REARSIDERIGHT;
                    break;
                default:
                    qCWarning(KMIX_LOG) << "Channel Map contains a pa_channel_position we cannot handle " << dev.channel_map.map[i];
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

        qCWarning(KMIX_LOG) << "Sink callback failure";
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

    s.priority = 0;
    if (i->active_port != nullptr)
        s.priority = i->active_port->priority;

    translateMasksAndMaps(s);

    bool is_new = !outputDevices.contains(s.index);
    outputDevices[s.index] = s;
//     qCDebug(KMIX_LOG) << "Got some info about sink: " << s.description;

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

        qCWarning(KMIX_LOG) << "Source callback failure";
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
//        qCDebug(KMIX_LOG) << "Ignoring Monitor Source: " << i->description;
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
//     qCDebug(KMIX_LOG) << "Got some info about source: " << s.description;

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

        qCWarning(KMIX_LOG) << "Client callback failure";
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);
        return;
    }

    clients[i->index] = QString::fromUtf8(i->name);
    //qCDebug(KMIX_LOG) << "Got some info about client: " << clients[i->index];
}

static void sink_input_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *) {

    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
            return;

        qCWarning(KMIX_LOG) << "Sink Input callback failure";
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
            //qCDebug(KMIX_LOG) << "Ignoring sink-input due to it being designated as an event and thus handled by the Event slider";
            return;
        }
    }

    QString appname = i18n("Unknown Application");
    if (clients.contains(i->client))
        appname = clients.value(i->client);

    devinfo s;
    s.index = i->index;
    s.device_index = i->sink;
    s.description = appname % QLatin1String(": ") % QString::fromUtf8(i->name);
    s.name = QString("stream:") + QString::number(i->index); //appname.replace(' ', '_').toLower();
    s.icon_name = getIconNameFromProplist(i->proplist);
    s.channel_map = i->channel_map;
    s.volume = i->volume;
    s.mute = !!i->mute;
    s.stream_restore_rule = QString::fromUtf8(t);

    translateMasksAndMaps(s);

    bool is_new = !outputStreams.contains(s.index);
    outputStreams[s.index] = s;
//     qCDebug(KMIX_LOG) << "Got some info about sink input (playback stream): " << s.description;

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

        qCWarning(KMIX_LOG) << "Source Output callback failure";
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
        qCDebug(KMIX_LOG) << "Source Output refers to a Source we don't have any info for (probably just a peak meter or similar)";
        return;
    }

    QString appname = i18n("Unknown Application");
    if (clients.contains(i->client))
        appname = clients.value(i->client);

    devinfo s;
    s.index = i->index;
    s.device_index = i->source;
    s.description = appname % QLatin1String(": ") % QString::fromUtf8(i->name);
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
//     qCDebug(KMIX_LOG) << "Got some info about source output (capture stream): " << s.description;

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


/**
 * Helper for performing an operation on the device specified by @p id.
 * Search for the device by name in the specified @p devices map, perform
 * the function @c func on it, and return the result of that.  If the
 * device is not found then do nothing.
 */
static int doForDevice(const QString &id, const devmap *devices, shared_ptr<MixDevice> md,
                int (*func)(const devinfo &, shared_ptr<MixDevice>))
{
    for (devmap::const_iterator iter = devices->constBegin(); iter!=devices->constEnd(); ++iter)
    {
        if (iter->name==id) return ((*func)(*iter, md));
    }

    qCDebug(KMIX_LOG) << "Device" << id << "not in map";
    return (Mixer::OK);
}


/**
 * Helper for checking the result of a PulseAudio operation.
 * If the operation @p op failed, log a message identifying the
 * call @p explain, then return @c false.  Return @c true if the
 * operation succeeeded, after dereferencing the operation.
 */
static bool checkOpResult(pa_operation *op, const char *explain)
{
    if (op==nullptr)
    {
        qCWarning(KMIX_LOG) << "PulseAudio operation" << explain << "failed,"
                            << pa_strerror(pa_context_errno(s_context));
        return (false);
    }

    pa_operation_unref(op);
    return (true);
}


void ext_stream_restore_read_cb(pa_context *c, const pa_ext_stream_restore_info *i, int eol, void *)
{
    if (eol < 0) {
        dec_outstanding(c);
        qCWarning(KMIX_LOG) << "Failed to initialize stream_restore extension," << pa_strerror(pa_context_errno(s_context));
        return;
    }

    if (eol > 0) {
        dec_outstanding(c);

        // Special case: ensure that our media events exists.
        // On first login by a new user this won't be in our
        // database, so we should create it.
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
            qCDebug(KMIX_LOG) << "Initializing restore rule for 'Event Sounds'";
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
//     qCDebug(KMIX_LOG) << QString("Got some info about restore rule: '%1' (Device: %2)").arg(name).arg(i->device ? i->device : "None");
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

static void ext_stream_restore_subscribe_cb(pa_context *c, void *)
{
    Q_ASSERT(c == s_context);

    pa_operation *op = pa_ext_stream_restore_read(c, ext_stream_restore_read_cb, nullptr);
    checkOpResult(op, "pa_ext_stream_restore_read");
}


static void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *)
{
    Q_ASSERT(c == s_context);

    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)
    {
        case PA_SUBSCRIPTION_EVENT_SINK:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                if (s_mixers.contains(KMIXPA_PLAYBACK))
                    s_mixers[KMIXPA_PLAYBACK]->removeWidget(index);
            } else {
                pa_operation *op = pa_context_get_sink_info_by_index(c, index, sink_cb, nullptr);
                checkOpResult(op, "pa_context_get_sink_info_by_index");
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SOURCE:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                if (s_mixers.contains(KMIXPA_CAPTURE))
                    s_mixers[KMIXPA_CAPTURE]->removeWidget(index);
            } else {
                pa_operation *op = pa_context_get_source_info_by_index(c, index, source_cb, nullptr);
                checkOpResult(op, "pa_context_get_source_info_by_index");
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                if (s_mixers.contains(KMIXPA_APP_PLAYBACK))
                    s_mixers[KMIXPA_APP_PLAYBACK]->removeWidget(index);
            } else {
                pa_operation *op = pa_context_get_sink_input_info(c, index, sink_input_cb, nullptr);
                checkOpResult(op, "pa_context_get_sink_input_info");
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                if (s_mixers.contains(KMIXPA_APP_CAPTURE))
                    s_mixers[KMIXPA_APP_CAPTURE]->removeWidget(index);
            } else {
                pa_operation *op = pa_context_get_source_output_info(c, index, source_output_cb, nullptr);
                checkOpResult(op, "pa_context_get_source_output_info");
            }
            break;

        case PA_SUBSCRIPTION_EVENT_CLIENT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                clients.remove(index);
            } else {
                pa_operation *op = pa_context_get_client_info(c, index, client_cb, nullptr);
                checkOpResult(op, "pa_context_get_client_info");
            }
            break;
    }
}


static void context_state_callback(pa_context *c, void *)
{
    pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_READY) {
        // Attempt to load things up
        pa_operation *op;

        // 1. Register for the stream changes (except during probe)
        if (s_context == c) {
            pa_context_set_subscribe_callback(c, subscribe_cb, nullptr);

            op = pa_context_subscribe(c, static_cast<pa_subscription_mask_t>(
                                            PA_SUBSCRIPTION_MASK_SINK|
                                            PA_SUBSCRIPTION_MASK_SOURCE|
                                            PA_SUBSCRIPTION_MASK_CLIENT|
                                            PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                            PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT), nullptr, nullptr);
            if (!checkOpResult(op, "pa_context_subscribe")) return;
        }

        op = pa_context_get_sink_info_list(c, sink_cb, nullptr);
        if (!checkOpResult(op, "pa_context_get_sink_info_list")) return;
        s_outstandingRequests++;

        op = pa_context_get_source_info_list(c, source_cb, nullptr);
        if (!checkOpResult(op, "pa_context_get_source_info_list")) return;
        s_outstandingRequests++;

        op = pa_context_get_client_info_list(c, client_cb, nullptr);
        if (!checkOpResult(op, "pa_context_client_info_list")) return;
        s_outstandingRequests++;

        op = pa_context_get_sink_input_info_list(c, sink_input_cb, nullptr);
        if (!checkOpResult(op, "pa_context_get_sink_input_info_list")) return;
        s_outstandingRequests++;

        op = pa_context_get_source_output_info_list(c, source_output_cb, nullptr);
        if (!checkOpResult(op, "pa_context_get_source_output_info_list")) return;
        s_outstandingRequests++;

        /* These calls are not always supported */
        op = pa_ext_stream_restore_read(c, ext_stream_restore_read_cb, nullptr);
        if (checkOpResult(op, "pa_ext_stream_restore_read"))
        {
            s_outstandingRequests++;

            pa_ext_stream_restore_set_subscribe_cb(c, ext_stream_restore_subscribe_cb, nullptr);
            pa_ext_stream_restore_subscribe(c, 1, nullptr, nullptr);
        }
    } else if (!PA_CONTEXT_IS_GOOD(state)) {
        // If this is our probe phase, exit our context immediately
        if (s_context != c) {
            pa_context_disconnect(c);
        } else {
            pa_context_unref(s_context);
            s_context = nullptr;

            // Remove all GUI elements
            QMap<int,Mixer_PULSE*>::iterator it;
            for (it = s_mixers.begin(); it != s_mixers.end(); ++it) {
                (*it)->removeAllWidgets();
            }
            // This one is not handled above.
            clients.clear();

            if (s_mixers.contains(KMIXPA_PLAYBACK)) {
                qCWarning(KMIX_LOG) << "Connection to PulseAudio daemon closed. Attempting reconnection.";
                s_pulseActive = UNKNOWN;
                QTimer::singleShot(50, s_mixers[KMIXPA_PLAYBACK], &Mixer_PULSE::reinit);
            }
        }
    }
}

static void setVolumeFromPulse(Volume& volume, const devinfo& dev)
{
    chanIDMap::const_iterator iter;
    for (iter = dev.chanIDs.begin(); iter != dev.chanIDs.end(); ++iter)
    {
        //qCDebug(KMIX_LOG) <<  "Setting volume for channel " << iter.value() << " to " << (long)dev.volume.values[iter.key()] << " (" << ((100*(long)dev.volume.values[iter.key()]) / PA_VOLUME_NORM) << "%)";
        volume.setVolume(iter.value(), static_cast<long>(dev.volume.values[iter.key()]));
    }
}

static pa_cvolume genVolumeForPulse(const devinfo& dev, Volume& volume)
{
    pa_cvolume cvol = dev.volume;

    chanIDMap::const_iterator iter;
    for (iter = dev.chanIDs.begin(); iter != dev.chanIDs.end(); ++iter)
    {
        cvol.values[iter.key()] = static_cast<uint32_t>(volume.getVolume(iter.value()));
        //qCDebug(KMIX_LOG) <<  "Setting volume for channel " << iter.value() << " to " << cvol.values[iter.key()] << " (" << ((100*cvol.values[iter.key()]) / PA_VOLUME_NORM) << "%)";
    }
    return cvol;
}

static devmap* get_widget_map(int type, QString id = QString())
{
    Q_ASSERT(type >= 0 && type <= KMIXPA_WIDGET_MAX);

    if (KMIXPA_PLAYBACK == type)
        return &outputDevices;
    else if (KMIXPA_CAPTURE == type)
        return &captureDevices;
    else if (KMIXPA_APP_PLAYBACK == type) {
        if (id.startsWith(QLatin1String("restore:")))
            return &outputRoles;
        return &outputStreams;
    } else if (KMIXPA_APP_CAPTURE == type)
        return &captureStreams;

    Q_ASSERT(0);
    return nullptr;
}

static devmap* get_widget_map(int type, int index)
{
    if (static_cast<uint32_t>(index)==PA_INVALID_INDEX) return (get_widget_map(type, "restore:"));
    return (get_widget_map(type));
}

void Mixer_PULSE::emitControlsReconfigured()
{
	//	emit controlsReconfigured(_mixer->id());
    // Do not emit directly to ensure all connected slots are executed
    // in their own event loop.

	/*
	 * Bug 309464:
	 *
	 * Comment by cesken: I am not really sure what the comment above means.
	 *  1) IIRC coling told me "otherwise KMix crashes".
	 *  2) There are also bug reports that heavily indicate the crash when operation the "move stream" from a popup
	 *     menu.
	 *  3) I don't know what the "executed in their own event loop" means. Are we in a "wrong" thread here (PA),
	 *     which is not suitable for GUI code?!?
	 *
	 * Work note: Ouch. it means PA thread makes direct calls via announce(), and do even GUI code. OUCH. Redo this comments!
	 *
	 *  Conclusions:
	 *  a) It seems there seems to be some object deletion hazard with a QMenu (the one for "move stream")
	 *  b)  I do not see why executing it Queued is better, because you can never know when it is actually being
	 *      executed: it could be "right now". It looks like Qt currently executes it after the QMenu hazard has
	 *      resolved itself miraculously.
	 *  c) I am definitely strongly opposed on this "execute later" approach. It is pure gambling IMO and might be
	 *     broken any time (from DEBUG to RELEASE build, or by a new Qt or KDE version).
	 *
	 *     TODO Somebody with more Qt and PA internal insight might help to clear up things here.
	 *
	 *  Temporary solution: Do the QueuedConnection until we really know hat is going on. But the called code
	 *                      pulseControlsReconfigured() will then do the standard announce() so that every part of
	 *                      KMix automatically gets updated.
	 *
	 */
    QMetaObject::invokeMethod(this,
                              "pulseControlsReconfigured",
                              Qt::QueuedConnection);

//    QMetaObject::invokeMethod(this,
//                              "pulseControlsReconfigured",
//                              Qt::QueuedConnection,
//                              Q_ARG(QString, _mixer->id()));
}

void Mixer_PULSE::pulseControlsReconfigured()
{
	qCDebug(KMIX_LOG) << "Reconfigure " << _mixer->id();
    ControlManager::instance()->announce(_mixer->id(), ControlManager::ControlList, getDriverName());
}

// void Mixer_PULSE::pulseControlsReconfigured(QString mixerId)
// {
// 	qCDebug(KMIX_LOG) << "Reconfigure " << mixerId;
//     ControlManager::instance()->announce(mixerId, ControlManager::ControlList, getDriverName());
// }

void Mixer_PULSE::updateRecommendedMaster(const devmap *map)
{
    unsigned int prio = 0;
    shared_ptr<MixDevice> res;

    for (MixSet::iterator iter = m_mixDevices.begin(); iter != m_mixDevices.end(); ++iter)
    {
        unsigned int devprio = map->value( id2num((*iter)->id()) ).priority;
        if (( devprio > prio ) || !res ) {
            prio = devprio;
            res = *iter;
        }
    }

    if (res)
         qCDebug(KMIX_LOG) << "Selecting master " << res->id()
                       << " for type " << m_devnum;
    m_recommendedMaster = res;
}

void Mixer_PULSE::addWidget(int index, bool isAppStream)
{
    devmap* map = get_widget_map(m_devnum, index);

    if (!map->contains(index)) {
        qCWarning(KMIX_LOG) << "New " << m_devnum << " widget notified for index "
                        << index << " but I cannot find it in my list :s";
        return;
    }

    if (addDevice((*map)[index], isAppStream))
        updateRecommendedMaster(map);
    emitControlsReconfigured();
}

void Mixer_PULSE::removeWidget(int index)
{
    devmap* map = get_widget_map(m_devnum);

    if (!map->contains(index)) {
        qCDebug(KMIX_LOG) << "Removing " << m_devnum << " widget notified for index "
                      << index << " but I cannot find it in my list :s";
        // Sometimes we ignore things (e.g. event sounds) so don't be too noisy here.
        return;
    }

    QString id = (*map)[index].name;
    map->remove(index);

    // We need to find the MixDevice that goes with this widget and remove it.
    MixSet::iterator iter;
    shared_ptr<MixDevice> md;
    for (iter = m_mixDevices.begin(); iter != m_mixDevices.end(); ++iter)
    {
        if ((*iter)->id() == id)
        {
            md = m_mixDevices.get(id);
            qCDebug(KMIX_LOG) << "MixDevice 1 useCount=" << md.use_count();
            md->close();
            qCDebug(KMIX_LOG) << "MixDevice 2 useCount=" << md.use_count();
            m_mixDevices.erase(iter);
            qCDebug(KMIX_LOG) << "MixDevice 3 useCount=" << md.use_count();
            break;
        }
    }

    if (md)
        updateRecommendedMaster(map);
    emitControlsReconfigured();
    qCDebug(KMIX_LOG) << "MixDevice 4 useCount=" << md.use_count();
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

bool Mixer_PULSE::addDevice(devinfo& dev, bool isAppStream)
{
    if (dev.chanMask==Volume::MNONE) return (false);

    MixSet *ms = nullptr;
    if (m_devnum==KMIXPA_APP_PLAYBACK && s_mixers.contains(KMIXPA_PLAYBACK))
    {
        ms = s_mixers[KMIXPA_PLAYBACK]->getMixSet();
    }
    else if (m_devnum==KMIXPA_APP_CAPTURE && s_mixers.contains(KMIXPA_CAPTURE))
    {
        ms = s_mixers[KMIXPA_CAPTURE]->getMixSet();
    }

    const bool isCapture = (m_devnum==KMIXPA_APP_CAPTURE || m_devnum==KMIXPA_CAPTURE);

    const int maxVol = (!isCapture && Settings::volumeOverdrive()) ? PA_VOLUME_UI_MAX : PA_VOLUME_NORM;
    Volume v(maxVol, PA_VOLUME_MUTED, true, false);
    v.addVolumeChannels(dev.chanMask);
    setVolumeFromPulse(v, dev);

    MixDevice *md = new MixDevice( _mixer, dev.name, dev.description, dev.icon_name, ms);
    if (isAppStream) md->setApplicationStream(true);
    md->setHardwareId(md->id().toLocal8Bit());

    //qCDebug(KMIX_LOG) << "Adding Pulse volume" << dev.name
    //                  << "isCapture" << isCapture
    //                  << "isAppStream" << isAppStream << "=" << md->isApplicationStream()
    //                  << "devnum" << m_devnum;

    if (isCapture)
    {
        md->addCaptureVolume(v);
        md->setRecSource(!dev.mute);
    }
    else
    {
        md->addPlaybackVolume(v);
        md->setMuted(dev.mute);
    }

    m_mixDevices.append(md->addToPool());
    return (true);
}

MixerBackend* PULSE_getMixer( Mixer *mixer, int devnum )
{
   MixerBackend *l_mixer;
   l_mixer = new Mixer_PULSE( mixer, devnum );
   return l_mixer;
}

bool Mixer_PULSE::connectToDaemon()
{
    Q_ASSERT(nullptr == s_context);

    qCDebug(KMIX_LOG) <<  "Attempting connection to PulseAudio sound daemon";

    // No need to create this until necessary
    if (!m_mainloop) {
        // When bumping c++ requirement to c++14,
        // replace with `= std::make_unique<QtPaMainLoop>();`
        m_mainloop.reset(new QtPaMainLoop);
    }
    s_context = pa_context_new(&m_mainloop->pa_vtable, "KMix");
    Q_ASSERT(s_context);

    if (pa_context_connect(s_context, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0) {
        pa_context_unref(s_context);
        s_context = nullptr;
        return false;
    }
    pa_context_set_state_callback(s_context, &context_state_callback, nullptr);
    return true;
}


Mixer_PULSE::Mixer_PULSE(Mixer *mixer, int devnum)
    : MixerBackend(mixer, devnum)
{
    if (devnum==-1) m_devnum = 0;
    if (qEnvironmentVariableIntValue("KMIX_PULSEAUDIO_DISABLE")) s_pulseActive = INACTIVE;

    ++refcount;
    if (INACTIVE != s_pulseActive && 1 == refcount)
    {
        // First of all connect to PA via simple/blocking means and if that succeeds,
        // use a fully async integrated mainloop method to connect and get proper support.
        pa_mainloop *p_test_mainloop = pa_mainloop_new();
        if (p_test_mainloop==nullptr)
        {
            qCDebug(KMIX_LOG) << "PulseAudio support disabled, unable to create mainloop";
            s_pulseActive = INACTIVE;
            goto endconstruct;
        }

        pa_context *p_test_context = pa_context_new(pa_mainloop_get_api(p_test_mainloop), "kmix-probe");
        if (p_test_context==nullptr)
        {
            qCDebug(KMIX_LOG) << "PulseAudio support disabled, unable to create context";
            pa_mainloop_free(p_test_mainloop);
            s_pulseActive = INACTIVE;
            goto endconstruct;
        }

        qCDebug(KMIX_LOG) << "Probing for PulseAudio...";
        // (cg) Convert to PA_CONTEXT_NOFLAGS when PulseAudio 0.9.19 is required
        if (pa_context_connect(p_test_context, nullptr, static_cast<pa_context_flags_t>(0), nullptr) < 0)
        {
            qCDebug(KMIX_LOG) << QString("PulseAudio support disabled, %1").arg(pa_strerror(pa_context_errno(p_test_context)));
            pa_context_disconnect(p_test_context);
            pa_context_unref(p_test_context);
            pa_mainloop_free(p_test_mainloop);
            s_pulseActive = INACTIVE;
            goto endconstruct;
        }

        // Assume we are inactive, it will be set to active if appropriate
        s_pulseActive = INACTIVE;
        pa_context_set_state_callback(p_test_context, &context_state_callback, nullptr);
        for (;;) {
          pa_mainloop_iterate(p_test_mainloop, 1, nullptr);

          if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(p_test_context))) {
            qCDebug(KMIX_LOG) << "PulseAudio probe complete.";
            break;
          }
        }
        pa_context_disconnect(p_test_context);
        pa_context_unref(p_test_context);
        pa_mainloop_free(p_test_mainloop);

        // Reconnect via integrated mainloop
        if (s_pulseActive!=INACTIVE) connectToDaemon();

        qCDebug(KMIX_LOG) <<  "PulseAudio status: " << (s_pulseActive==UNKNOWN ? "Unknown (bug)" : (s_pulseActive==ACTIVE ? "Active" : "Inactive"));
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
        if (refcount==0)
        {
            if (s_context!=nullptr)
            {
                pa_context_unref(s_context);
                s_context = nullptr;
            }
        }
    }

    closeCommon();
}

int Mixer_PULSE::open()
{
    //qCDebug(KMIX_LOG) <<  "Trying Pulse sink";

    if (ACTIVE == s_pulseActive && m_devnum <= KMIXPA_APP_CAPTURE)
    {
        // Make sure the GUI layers know we are dynamic so as to always paint us
        _mixer->setDynamic();

        devmap::iterator iter;
        if (KMIXPA_PLAYBACK == m_devnum)
        {
        	_id = "Playback Devices";
        	registerCard(i18n("Playback Devices"));
            for (iter = outputDevices.begin(); iter != outputDevices.end(); ++iter)
                addDevice(*iter);
            updateRecommendedMaster(&outputDevices);
        }
        else if (KMIXPA_CAPTURE == m_devnum)
        {
        	_id = "Capture Devices";
        	registerCard(i18n("Capture Devices"));
            for (iter = captureDevices.begin(); iter != captureDevices.end(); ++iter)
                addDevice(*iter);
            updateRecommendedMaster(&outputDevices);
        }
        else if (KMIXPA_APP_PLAYBACK == m_devnum)
        {
        	_id = "Playback Streams";
        	registerCard(i18n("Playback Streams"));
            for (iter = outputRoles.begin(); iter != outputRoles.end(); ++iter)
                addDevice(*iter, true);
            updateRecommendedMaster(&outputRoles);
            for (iter = outputStreams.begin(); iter != outputStreams.end(); ++iter)
                addDevice(*iter, true);
            updateRecommendedMaster(&outputStreams);
        }
        else if (KMIXPA_APP_CAPTURE == m_devnum)
        {
        	_id = "Capture Streams";
            registerCard(i18n("Capture Streams"));
            for (iter = captureStreams.begin(); iter != captureStreams.end(); ++iter)
                addDevice(*iter);
            updateRecommendedMaster(&captureStreams);
        }

        qCDebug(KMIX_LOG) <<  "Using PulseAudio for mixer: " << getName();
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
    int num = -1;
    // todo: Store this in a hash or similar
    int i;
    for (i = 0; i < m_mixDevices.size(); ++i) {
        if (m_mixDevices[i]->id() == id) {
            num = i;
            break;
        }
    }
    //qCDebug(KMIX_LOG) << "id2num() num=" << num;
    return num;
}


int Mixer_PULSE::readVolumeFromHW(const QString &id, shared_ptr<MixDevice> md)
{
    const devmap *map = get_widget_map(m_devnum, id);
    return (doForDevice(id, map, md,
                        [](const devinfo &dev, shared_ptr<MixDevice> md) -> int
                        {
                            setVolumeFromPulse(md->playbackVolume(), dev);
                            md->setMuted(dev.mute);			// to cover both playback
                            md->setRecSource(!dev.mute);		// and capture channels
                            return (0);
                        }));
}


int Mixer_PULSE::writeVolumeToHW(const QString &id, shared_ptr<MixDevice> md)
{
    switch (m_devnum)
    {
case KMIXPA_PLAYBACK:
        return (doForDevice(id, &outputDevices, md,
                            [](const devinfo &dev, shared_ptr<MixDevice> md) -> int
                            {
                                pa_cvolume volume = genVolumeForPulse(dev, md->playbackVolume());
                                pa_operation *op = pa_context_set_sink_volume_by_index(s_context, dev.index, &volume, nullptr, nullptr);
                                if (!checkOpResult(op,"pa_context_set_sink_volume_by_index")) return (Mixer::ERR_WRITE);

                                op = pa_context_set_sink_mute_by_index(s_context, dev.index, (md->isMuted() ? 1 : 0), nullptr, nullptr);
                                if (!checkOpResult(op, "pa_context_set_sink_mute_by_index")) return (Mixer::ERR_WRITE);

                                return (Mixer::OK);
                            }));

case KMIXPA_CAPTURE:
        return (doForDevice(id, &captureDevices, md,
                            [](const devinfo &dev, shared_ptr<MixDevice> md) -> int
                            {
                                pa_cvolume volume = genVolumeForPulse(dev, md->captureVolume());
                                pa_operation *op = pa_context_set_source_volume_by_index(s_context, dev.index, &volume, nullptr, nullptr);
                                if (!checkOpResult(op, "pa_context_set_source_volume_by_index")) return (Mixer::ERR_WRITE);

                                op = pa_context_set_source_mute_by_index(s_context, dev.index, (md->isRecSource() ? 0 : 1), nullptr, nullptr);
                                if (!checkOpResult(op, "pa_context_set_source_mute_by_index")) return (Mixer::ERR_WRITE);

                                return (Mixer::OK);
                            }));

case KMIXPA_APP_PLAYBACK:
        if (id.startsWith(QLatin1String("stream:")))
        {
            return (doForDevice(id, &outputStreams, md,
                                [](const devinfo &dev, shared_ptr<MixDevice> md) -> int
                                {
                                    pa_cvolume volume = genVolumeForPulse(dev, md->playbackVolume());
                                    pa_operation *op = pa_context_set_sink_input_volume(s_context, dev.index, &volume, nullptr, nullptr);
                                    if (!checkOpResult(op, "pa_context_set_sink_input_volume")) return (Mixer::ERR_WRITE);

                                    op = pa_context_set_sink_input_mute(s_context, dev.index, (md->isMuted() ? 1 : 0), nullptr, nullptr);
                                    if (!checkOpResult(op, "pa_context_set_sink_input_mute")) return (Mixer::ERR_WRITE);

                                    return (Mixer::OK);
                                }));
        }
        else if (id.startsWith(QLatin1String("restore:")))
        {
            return (doForDevice(id, &outputRoles, md,
                                [](const devinfo &dev, shared_ptr<MixDevice> md) -> int
                                {
                                    restoreRule &rule = s_RestoreRules[dev.stream_restore_rule];
                                    pa_ext_stream_restore_info info;
                                    info.name = dev.stream_restore_rule.toUtf8().constData();
                                    info.channel_map = rule.channel_map;
                                    info.volume = genVolumeForPulse(dev, md->playbackVolume());
                                    info.device = rule.device.isEmpty() ? nullptr : rule.device.toUtf8().constData();
                                    info.mute = (md->isMuted() ? 1 : 0);

                                    pa_operation *op = pa_ext_stream_restore_write(s_context, PA_UPDATE_REPLACE, &info, 1, true, nullptr, nullptr);
                                    if (!checkOpResult(op, "pa_ext_stream_restore_write")) return (Mixer::ERR_WRITE);

                                    return (Mixer::OK);
                                }));
        }
        else return (Mixer::OK);

case KMIXPA_APP_CAPTURE:
        return (doForDevice(id, &captureStreams, md,
                            [](const devinfo &dev, shared_ptr<MixDevice> md) -> int
                            {
#if HAVE_SOURCE_OUTPUT_VOLUMES
                                pa_cvolume volume = genVolumeForPulse(dev, md->captureVolume());
                                pa_operation *op = pa_context_set_source_output_volume(s_context, dev.index, &volume, nullptr, nullptr);
                                if (!checkOpResult(op, "pa_context_set_source_output_volume")) return (Mixer::ERR_WRITE);

                                op = pa_context_set_source_output_mute(s_context, dev.index, (md->isRecSource() ? 0 : 1), nullptr, nullptr);
                                if (!checkOpResult(op, "pa_context_set_source_output_mute")) return (Mixer::ERR_WRITE);
#else                
                                // Note that this is different from APP_PLAYBACK in that
                                // we set the volume on the source itself.
                                pa_cvolume volume = genVolumeForPulse(dev, md->captureVolume());
                                pa_operation *op = pa_context_set_source_volume_by_index(s_context, dev.device_index, &volume, nullptr, nullptr);
                                if (!checkOpResult(op, "pa_context_set_source_volume_by_index")) return (Mixer::ERR_WRITE);

                                op = pa_context_set_source_mute_by_index(s_context, dev.device_index, (md->isRecSource() ? 0 : 1), nullptr, nullptr);
                                if (!checkOpResult(op, "pa_context_set_source_mute_by_index")) return (Mixer::ERR_WRITE);
#endif
                                return (Mixer::OK);
                            }));

default:
        qCWarning(KMIX_LOG) << "Unknown device index" << m_devnum;
    }

    return (Mixer::OK);
}


static const devinfo *getStreamInfo(int devnum, const QString &id)
{
    //qCDebug(KMIX_LOG) <<  "dev" << devnum << "id" << id;
    const devmap *map = get_widget_map(devnum);
    for (devmap::const_iterator iter = map->constBegin(); iter!=map->constEnd(); ++iter)
    {
        // I think this will work!  '*iter' dereferences the iterator and obtains
        // a reference to the actual map item.  '&' then takes the address of that
        // and returns a pointer.
        if (iter->name==id) return (&*iter);
    }

    qCWarning(KMIX_LOG) <<  "Cannot find stream index for" << id;
    return (nullptr);
}


/**
 * Move the stream to a new destination.
 */
bool Mixer_PULSE::moveStream(const QString &id, const QString &destId)
{
    Q_ASSERT(m_devnum==KMIXPA_APP_PLAYBACK || m_devnum==KMIXPA_APP_CAPTURE);
    qCDebug(KMIX_LOG) <<  "dev" << m_devnum << "move" << id << "->" << destId;

    const devinfo *info = getStreamInfo(m_devnum, id);
    if (info==nullptr) return (false);

    if (destId.isEmpty())
    {
        // Reset the stream to its automatic destination, using the previously saved
        // restore rule.

        const QString stream_restore_rule = info->stream_restore_rule;
        if (stream_restore_rule.isEmpty() || !s_RestoreRules.contains(stream_restore_rule))
        {
            qCWarning(KMIX_LOG) <<  "Stream has no restore rule";
        }
        else
        {
            // We want to remove any specific device in the stream restore rule.
            const restoreRule &rule = s_RestoreRules[stream_restore_rule];
            pa_ext_stream_restore_info info;
            info.name = stream_restore_rule.toUtf8().constData();
            info.channel_map = rule.channel_map;
            info.volume = rule.volume;
            info.device = nullptr;
            info.mute = rule.mute ? 1 : 0;

            pa_operation *op = pa_ext_stream_restore_write(s_context, PA_UPDATE_REPLACE, &info, 1, true, nullptr, nullptr);
            if (!checkOpResult(op, "pa_ext_stream_restore_write")) return (false);
        }
    }
    else
    {
        // Move the stream to the specified destination.

        const uint32_t stream_index = info->index;
        if (m_devnum==KMIXPA_APP_PLAYBACK)
        {
            pa_operation *op = pa_context_move_sink_input_by_name(s_context, stream_index, destId.toUtf8().constData(), nullptr, nullptr);
            if (!checkOpResult(op, "pa_context_move_sink_input_by_name")) return (false);
        }
        else
        {
            pa_operation *op = pa_context_move_source_output_by_name(s_context, stream_index, destId.toUtf8().constData(), nullptr, nullptr);
            if (!checkOpResult(op, "pa_context_move_source_output_by_name")) return (false);
        }
    }

    return (true);
}


QString Mixer_PULSE::currentStreamDevice(const QString &id) const
{
    Q_ASSERT(m_devnum==KMIXPA_APP_PLAYBACK || m_devnum==KMIXPA_APP_CAPTURE);
    //qCDebug(KMIX_LOG) <<  "dev" << m_devnum << "id" << id;

    const devinfo *info = getStreamInfo(m_devnum, id);
    if (info==nullptr) return (QString());

    const int dev = info->device_index;
    // Look in 'outputDevices' for KMIXPA_APP_PLAYBACK, or 'captureDevices' for KMIXPA_APP_CAPTURE
    const devmap *deviceMap = get_widget_map(m_devnum==KMIXPA_APP_PLAYBACK ? KMIXPA_PLAYBACK : KMIXPA_CAPTURE);
    if (deviceMap->contains(dev))
    {
        const devinfo &s = deviceMap->value(dev);
        //qDebug() << "  dev" << s.device_index << "desc" << s.description << "->" << s.name;
        return (s.name);
    }
    //else qDebug() << "  no device info";

    return (QString());
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

const char *PULSE_driverName = "PulseAudio";

QString Mixer_PULSE::getDriverName()
{
    return (PULSE_driverName);
}
