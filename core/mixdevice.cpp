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

#include "core/mixdevice.h"

#include <qregexp.h>

#include <klocalizedstring.h>

#include "core/mixer.h"
#include "dbus/dbuscontrolwrapper.h"
#include "gui/guiprofile.h"
#include "core/volume.h"

static const QString channelTypeToIconName( MixDevice::ChannelType type )
{
    switch (type) {
        case MixDevice::AUDIO:
            return "mixer-pcm";
        case MixDevice::BASS:
        case MixDevice::SURROUND_LFE: // "LFE" SHOULD have an own icon
            return "mixer-lfe";
        case MixDevice::CD:
            return "mixer-cd";
        case MixDevice::EXTERNAL:
            return "mixer-line";
        case MixDevice::MICROPHONE:
            return "mixer-microphone";
        case MixDevice::MIDI:
            return "mixer-midi";
        case MixDevice::RECMONITOR:
            return "mixer-capture";
        case MixDevice::TREBLE:
            return "mixer-pcm-default";
        case MixDevice::UNKNOWN:
            return "mixer-front";
        case MixDevice::VOLUME:
            return "mixer-master";
        case MixDevice::VIDEO:
            return "mixer-video";
        case MixDevice::SURROUND:
        case MixDevice::SURROUND_BACK:
            return "mixer-surround";
        case MixDevice::SURROUND_CENTERFRONT:
        case MixDevice::SURROUND_CENTERBACK:
            return "mixer-surround-center";
        case MixDevice::HEADPHONE:
            return "mixer-headset";
        case MixDevice::DIGITAL:
            return "mixer-digital";
        case MixDevice::AC97:
            return "mixer-ac97";
        case MixDevice::SPEAKER:
            return "mixer-pc-speaker";
        case MixDevice::MICROPHONE_BOOST:
            return "mixer-microphone-boost";
        case MixDevice::MICROPHONE_FRONT_BOOST:
            return "mixer-microphone-front-boost";
        case MixDevice::MICROPHONE_FRONT:
            return "mixer-microphone-front";
        case MixDevice::KMIX_COMPOSITE:
            return "mixer-line";

        // Icon names for known MPRIS2 applications are now taken from the application's
        // desktop file, for those that provide one.  If the application does not
        // provide a desktop file or icon information then there is a fallback list
        // in Mixer_MPRIS2::getIconNameFromPlayerId().

        case MixDevice::APPLICATION_STREAM:
            return "mixer-pcm";

    }
    return "mixer-front";
}


/**
 * Constructs a MixDevice. A MixDevice represents one channel or control of
 * the mixer hardware. A MixDevice has a type (e.g. PCM), a descriptive name
 * (for example "Master" or "Headphone" or "IEC 958 Output"),
 * can have a volume level (2 when stereo), can be recordable and muted.
 * The ChannelType tells which kind of control the MixDevice is.
 */
MixDevice::MixDevice(  Mixer* mixer, const QString& id, const QString& name, ChannelType type )
    : _profControl(0)
{
    init(mixer, id, name, channelTypeToIconName(type), nullptr);
}

MixDevice::MixDevice(  Mixer* mixer, const QString& id, const QString& name, const QString& iconName, MixSet* moveDestinationMixSet )
    : _profControl(0)
{
    init(mixer, id, name, iconName, moveDestinationMixSet);
}

void MixDevice::init(  Mixer* mixer, const QString& id, const QString& name, const QString& iconName, MixSet* moveDestinationMixSet )
{
    _artificial = false;
    _applicationStream = false;
    _dbusControlWrapper = nullptr;			// will be set in addToPool()
    _mixer = mixer;
    _id = id;
    _enumCurrentId = 0;

    _mediaController = new MediaController(_id);
    if( name.isEmpty() )
        _name = i18n("unknown");
    else
        _name = name;
    if ( iconName.isEmpty() )
        _iconName = "mixer-front";
    else
        _iconName = iconName;
    _moveDestinationMixSet = moveDestinationMixSet;
    if ( _id.contains(' ') ) {
        // The key is used in the config file. IdbusControlWrappert MUST NOT contain spaces
        qCCritical(KMIX_LOG) << "MixDevice::setId(\"" << id << "\") . Invalid key - it must not contain spaces";
        _id.replace(' ', '_');
    }
//    qCDebug(KMIX_LOG) << "MixDevice::init() _id=" << _id;
}


/*
 * When a MixDevice shall be finally discarded, you must use this method to free its resources.
 * You must not use this MixDevice after calling close().
 * <br>
 * The necessity stems from a memory leak due to object cycle (MixDevice<->DBusControlWrapper), so the reference
 * counting shared_ptr has no chance to clean up. See Bug 309464 for background information.
 */
void MixDevice::close()
{
	delete _dbusControlWrapper;
	_dbusControlWrapper = nullptr;
}


shared_ptr<MixDevice> MixDevice::addToPool()
{
//	qCDebug(KMIX_LOG) << "id=" <<  _mixer->id() << ":" << _id;
    shared_ptr<MixDevice> thisSharedPtr(this);
    _dbusControlWrapper = new DBusControlWrapper(thisSharedPtr, dbusPath());
    return (thisSharedPtr);
}


/**
 * Changes the internal state of this MixDevice.
 * It does not commit the change to the hardware.
 *
 * You might want to call something like m_mixdevice->mixer()->commitVolumeChange(m_mixdevice); after calling this method.
 */
void MixDevice::increaseOrDecreaseVolume(bool decrease, Volume::VolumeTypeFlag volumeType)
{
	bool debugme = false;
//	bool debugme =  id() == "PCM:0" ;
	if (volumeType & Volume::Playback)
	{
		Volume &volP = playbackVolume();
		long inc = volP.volumeStep(decrease);

		if (debugme)
		  qCDebug(KMIX_LOG) << ( decrease ? "decrease by " : "increase by " ) << inc ;

		if (isMuted())
		{
			setMuted(false);
		}
		else
		{
			volP.changeAllVolumes(inc);
			if (debugme)
				qCDebug(KMIX_LOG) << (decrease ? "decrease by " : "increase by ") << inc;
		}
	}

	if (volumeType & Volume::Capture)
	{
		if (debugme)
			qCDebug(KMIX_LOG) << "VolumeType=" << volumeType << "   c";

		Volume& volC = captureVolume();
		long inc = volC.volumeStep(decrease);
		volC.changeAllVolumes(inc);
	}

}



/**
 * Returns the name of the config group
 * @param Prefix of the group, e.g. "View_ALSA_USB_01"
 * @returns The config group name in the format "prefix.mixerId.controlId"
 */
QString MixDevice::configGroupName(QString prefix) const
{
	 QString devgrp = QString("%1.%2.%3").arg(prefix, mixer()->id(), id());
	 return devgrp;
}

/**
 * Returns a fully qualified id of this control, as a String in the form "controlId@mixerId"
 * @return
 */
QString MixDevice::fullyQualifiedId() const
{
	QString fqId = QString("%1@%2").arg(_id, _mixer->id());
	return fqId;
}

/**
 * Creates a deep copy of the given Volume, and adds it to this MixDevice.
 *
 * @param playbackVol
 */
void MixDevice::addPlaybackVolume(Volume &playbackVol)
{
   // Hint: "_playbackVolume" gets COPIED from "playbackVol", because the copy-constructor actually copies the volume levels.
   _playbackVolume = playbackVol;
   _playbackVolume.setSwitchType(Volume::PlaybackSwitch);
}

/**
 * Creates a deep copy of the given Volume, and adds it to this MixDevice.
 *
 * @param captureVol
 */
void MixDevice::addCaptureVolume (Volume &captureVol)
{
   _captureVolume = captureVol;
   _captureVolume.setSwitchType(Volume::CaptureSwitch);
}

void MixDevice::addEnums(QList<QString*>& ref_enumList)
{
   if ( ref_enumList.count() > 0 ) {
      int maxEnumId = ref_enumList.count();
      for (int i=0; i<maxEnumId; i++ ) {
            // we have an enum. Lets set the names of the enum items in the MixDevice
            // the enum names are assumed to be static!
            _enumValues.append( *(ref_enumList.at(i)) );
      }
   }
   _enumCurrentId = 0; // default is first entry (used if we don't get a value via backend or volume restore)
}


MixDevice::~MixDevice()
{
    _enumValues.clear(); // The QString's inside will be auto-deleted, as they get unref'ed
    delete _dbusControlWrapper;
    delete _mediaController;
}

void MixDevice::setEnumId(int enumId)
{
   if ( enumId < _enumValues.count() ) {
      _enumCurrentId = enumId;
   }
}

const QString MixDevice::dbusPath() const
{
   QString controlPath = _id;
   controlPath.replace(QRegExp("[^a-zA-Z0-9_]"), "_");
   controlPath.replace(QLatin1String("//"), QLatin1String("/"));

   if ( controlPath.endsWith( '/' ) )
   {
	   controlPath.chop(1);
   }

   return (_mixer->dbusPath()+'/'+controlPath);
}


bool MixDevice::isMuted() const			{ return (!_playbackVolume.isSwitchActivated()); }
bool MixDevice::isVirtuallyMuted() const	{ return (!hasPhysicalMuteSwitch() && isMuted()); }
void MixDevice::setMuted(bool mute)		{ _playbackVolume.setSwitch(!mute); }
void MixDevice::toggleMute()			{ setMuted( _playbackVolume.isSwitchActivated()); }

bool MixDevice::hasMuteSwitch() const		{ return (_playbackVolume.hasVolume() || _playbackVolume.hasSwitch()); }
bool MixDevice::hasPhysicalMuteSwitch() const	{ return (_playbackVolume.hasSwitch()); }

bool MixDevice::isRecSource() const		{ return (_captureVolume.hasSwitch() && _captureVolume.isSwitchActivated()); }
bool MixDevice::isNotRecSource() const		{ return (_captureVolume.hasSwitch() && !_captureVolume.isSwitchActivated()); }
void MixDevice::setRecSource(bool value)	{ _captureVolume.setSwitch( value ); }

bool MixDevice::isEnum() const			{ return (!_enumValues.isEmpty()); }

int MixDevice::mediaPlay()			{ return (mixer()->mediaPlay(_id)); }
int MixDevice::mediaPrev()			{ return (mixer()->mediaPrev(_id)); }
int MixDevice::mediaNext()			{ return (mixer()->mediaNext(_id)); }


bool MixDevice::operator==(const MixDevice& other) const
{
   return ( _id == other._id );
}

/**
 * This method is currently only called on "kmixctrl --restore"
 *
 * Normally we have a working _volume object already, which is very important,
 * because we need to read the minimum and maximum volume levels.
 * (Another solution would be to "equip" volFromConfig with maxInt and minInt values).
 */
bool MixDevice::read(const KConfig *config, const QString &grp)
{
    if ( _mixer->isDynamic() || isArtificial() ) {
        qCDebug(KMIX_LOG) << "MixDevice::read(): This MixDevice does not permit volume restoration (i.e. because it is handled lower down in the audio stack). Ignoring.";
        return false;
    }

    QString devgrp = QString("%1.Dev%2").arg(grp, _id);
    const KConfigGroup cg = config->group( devgrp );
    //qCDebug(KMIX_LOG) << "MixDevice::read() of group devgrp=" << devgrp;

    readPlaybackOrCapture(cg, false);
    readPlaybackOrCapture(cg, true );

    bool mute = cg.readEntry("is_muted", false);
    setMuted( mute );

    bool recsrc = cg.readEntry("is_recsrc", false);
    setRecSource( recsrc );

    int enumId = cg.readEntry("enum_id", -1);
    if ( enumId != -1 ) {
        setEnumId( enumId );
    }
    return true;
}

void MixDevice::readPlaybackOrCapture(const KConfigGroup& config, bool capture)
{
    Volume& volume = capture ? captureVolume() : playbackVolume();

    for ( Volume::ChannelID chid=Volume::CHIDMIN; chid<= Volume::CHIDMAX;  )
    {
      QString volstr = getVolString(chid,capture);
       if ( config.hasKey(volstr) ) {
          volume.setVolume(chid, config.readEntry(volstr, 0));
       } // if saved channel exists
       // TODO: ugly, so find a better way (implement Volume::ChannelID::operator++)
       chid = static_cast<Volume::ChannelID>(1+static_cast<int>(chid));
    } // for all channels
}

/**
 *  called on "kmixctrl --save" and from the GUI's (currently only on exit)
 */
bool MixDevice::write(KConfig *config, const QString &grp)
{
    if (_mixer->isDynamic() || isArtificial()) {
//        qCDebug(KMIX_LOG) << "MixDevice::write(): This MixDevice does not permit volume saving (i.e. because it is handled lower down in the audio stack). Ignoring.";
        return false;
    }

    QString devgrp = QString("%1.Dev%2").arg(grp, _id);
    KConfigGroup cg = config->group(devgrp);
    // qCDebug(KMIX_LOG) << "MixDevice::write() of group devgrp=" << devgrp;

    writePlaybackOrCapture(cg, false);
    writePlaybackOrCapture(cg, true);

    cg.writeEntry("is_muted" , isMuted());
    cg.writeEntry("is_recsrc", isRecSource());
    cg.writeEntry("name", _name);
    if (isEnum()) cg.writeEntry("enum_id", enumId());
    return true;
}

void MixDevice::writePlaybackOrCapture(KConfigGroup& config, bool capture)
{
    Volume& volume = capture ? captureVolume() : playbackVolume();
    const QMap<Volume::ChannelID, VolumeChannel> volumes = volume.getVolumes();
    for (const VolumeChannel &vc : volumes)
    {							// for all channels
        config.writeEntry(getVolString(vc.chid, capture), static_cast<int>(vc.volume));
    }
}

QString MixDevice::getVolString(Volume::ChannelID chid, bool capture)
{
       QString volstr = Volume::channelNameForPersistence(chid);
       if ( capture ) volstr += "Capture";
       return volstr;
}

/**
 * Returns the playback volume level in percent. If the volume is muted, 0 is returned.
 * If the given MixDevice contains no playback volume, the capture volume is used
 * instead, and 0 is returned if capturing is disabled for the given MixDevice.
 *
 * @returns The volume level in percent
 */
int MixDevice::userVolumeLevel() const
{
	bool usePlayback = _playbackVolume.hasVolume();
	const Volume &vol = usePlayback ? _playbackVolume : _captureVolume;
	bool isActive = usePlayback ? !isMuted() : isRecSource();
	int val = isActive ? vol.getAvgVolumePercent(Volume::MALL) : 0;
	return (val);
}


void MixDevice::setIconName(const QString &newName)
{
    qDebug() << "for" << readableName() << "icon" << newName;
    _iconName = newName;
    emit iconNameChanged(newName);
}
