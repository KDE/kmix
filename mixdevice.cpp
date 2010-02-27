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

#include <kdebug.h>
#include <klocale.h>

#include "mixdevice.h"
#include "volume.h"

/**
 * Constructs a MixDevice. A MixDevice represents one channel or control of
 * the mixer hardware. A MixDevice has a type (e.g. PCM), a descriptive name
 * (for example "Master" or "Headphone" or "IEC 958 Output"),
 * can have a volume level (2 when stereo), can be recordable and muted.
 * The category tells which kind of control the MixDevice is.
 *
 * Hints: Meaning of "category" has changed. In future the MixDevice might contain two
 * Volume objects, one for Output (Playback volume) and one for Input (Record volume).
 */
MixDevice::MixDevice(  Mixer* mixer, const QString& id, const QString& name, ChannelType type ) :
    _mixer(mixer), _type( type ), _id( id )
{
    if( name.isEmpty() )
        _name = i18n("unknown");
    else
        _name = name;
    if ( _id.contains(' ') ) {
        // The key is used in the config file. It MUST NOT contain spaces
        kError(67100) << "MixDevice::setId(\"" << id << "\") . Invalid key - it might not contain spaces" << endl;
        _id.replace(' ', '_');
    }
}

void MixDevice::addPlaybackVolume(Volume &playbackVol)
{
   // Hint: "_playbackVolume" gets COPIED from "playbackVol", because the copy-constructor actually copies the volume levels.
   _playbackVolume = playbackVol;
   _playbackVolume.setSwitchType(Volume::PlaybackSwitch);
}

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
}


MixDevice::~MixDevice() {
    _enumValues.clear(); // The QString's inside will be auto-deleted, as they get unref'ed
}

Volume& MixDevice::playbackVolume()
{
    return _playbackVolume;
}

Volume& MixDevice::captureVolume()
{
    return _captureVolume;
}


void MixDevice::setEnumId(int enumId)
{
   if ( enumId < _enumValues.count() ) {
      _enumCurrentId = enumId;
   }
}

unsigned int MixDevice::enumId()
{
   return _enumCurrentId;
}

QList<QString>& MixDevice::enumValues() {
   return _enumValues;
}


const QString& MixDevice::id() const {
   return _id;
}

bool MixDevice::operator==(const MixDevice& other) const
{
   return ( _id == other._id );
}

/**
 * This methhod is currently only called on "kmixctrl --restore"
 *
 * Normally we have a working _volume object already, which is very important,
 * because we need to read the minimum and maximum volume levels.
 * (Another solution would be to "equip" volFromConfig with maxInt and minInt values).
 */
void MixDevice::read( KConfig *config, const QString& grp )
{
    QString devgrp;
    devgrp.sprintf( "%s.Dev%s", grp.toAscii().data(), _id.toAscii().data() );
    KConfigGroup cg = config->group( devgrp );
    //kDebug(67100) << "MixDevice::read() of group devgrp=" << devgrp;

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
}

void MixDevice::readPlaybackOrCapture(const KConfigGroup& config, bool capture)
{
    //Volume::ChannelMask chMask = Volume::MNONE;
    
    Volume& volume = capture ? captureVolume() : playbackVolume();

    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
       Volume::ChannelID chid = (Volume::ChannelID)i;
       QString volstr (Volume::ChannelNameForPersistence[ chid ]);
       if ( capture ) volstr += "Capture";
       if ( config.hasKey(volstr) ) {
          long volCfg = config.readEntry(volstr, 0);
          //chMask |= _channelMaskEnum[i];

          volume.setVolume(chid, volCfg);
       } // if saved channel exists
    } // for all channels    
}

/**
 *  called on "kmixctrl --save" and from the GUI's (currently only on exit)
 */
void MixDevice::write( KConfig *config, const QString& grp )
{
   QString devgrp;
   devgrp.sprintf( "%s.Dev%s", grp.toAscii().data(), _id.toAscii().data() );
   KConfigGroup cg = config->group(devgrp);
   // kDebug(67100) << "MixDevice::write() of group devgrp=" << devgrp;

    writePlaybackOrCapture(cg, false);
    writePlaybackOrCapture(cg, true );

    cg.writeEntry("is_muted" , isMuted() );
    cg.writeEntry("is_recsrc", isRecSource() );
    cg.writeEntry("name", _name);
    if ( isEnum() ) {
        cg.writeEntry("enum_id", enumId() );
    }    
}

void MixDevice::writePlaybackOrCapture(KConfigGroup& config, bool capture)
{
    Volume& volume = capture ? captureVolume() : playbackVolume();

    for ( int i=0; i<= Volume::CHIDMAX; i++ ) {
       if ( volume._chmask & Volume::_channelMaskEnum[i] ) {
           Volume::ChannelID chid = (Volume::ChannelID)i;

           volume.getVolume( chid );
	   QString volstr (Volume::ChannelNameForPersistence[ chid ]);
	   if ( capture ) volstr += "Capture";
	   config.writeEntry(volstr , (int)volume.getVolume( chid ) );
       } // if supported channel
    } // for all channels

}


#include "mixdevice.moc"

