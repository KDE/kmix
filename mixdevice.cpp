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
MixDevice::MixDevice( const QString& id, Volume &playbackVol, Volume &captureVol, const QString& name, ChannelType type ) :
    _playbackVolume( playbackVol ), _captureVolume(captureVol), _type( type ), _id( id )
{
    // Hint: "_volume" gets COPIED from "vol" due to the fact that the copy-constructor actually copies the volume levels.
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

MixDevice::MixDevice(const MixDevice &md) : QObject()
{
    _name           = md._name;
    _playbackVolume = md._playbackVolume;
    _captureVolume  = md._captureVolume;
    _type           = md._type;
    _id             = md._id;
    _enumValues     = md._enumValues;
}

MixDevice::~MixDevice() {
    // Clear MixDevices enum Strings (switch on auto-delete, so the QString's inside will be cleared)
    // Not needed anymore, as we store QString's, not pointers
    //_enumValues.setAutoDelete(true);
    _enumValues.clear();
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
    config->setGroup( devgrp );
    //kDebug(67100) << "MixDevice::read() of group devgrp=" << devgrp << endl;

    readPlaybackOrCapture(config, "volumeL"       , "volumeR"       , false);
    readPlaybackOrCapture(config, "volumeLCapture", "volumeRCapture", true );
}

void MixDevice::readPlaybackOrCapture(KConfig *config, const char* nameLeftVolume, const char* nameRightVolume, bool capture)
{
    Volume::ChannelMask chMask = Volume::MNONE;
    int vl = config->readEntry(nameLeftVolume, -1);
    if (vl!=-1) {
        chMask = (Volume::ChannelMask)(chMask | Volume::MLEFT);
    }
    int vr = config->readEntry(nameRightVolume, -1);
    if (vr!=-1) {
        chMask = (Volume::ChannelMask)(chMask | Volume::MRIGHT);
    }

    Volume& volume = capture ? captureVolume() : playbackVolume();
    /*
     * Now start construction a temporary Volume object.
     * We take the maxvol and minvol values from _volume, which is already constructed.
     * Otherwise we would have to wildly guess those values
     */
    //Volume *volFromConfig = new Volume(chMask, volume.maxVolume(), volume.minVolume() );
    if (vl!=-1) {
        volume.setVolume(Volume::LEFT , vl);
    }
    if (vr!=-1) {
        volume.setVolume(Volume::RIGHT, vr);
    }
    // commit the read config
    //volume.setVolume(*volFromConfig); 
    //delete volFromConfig;
    
    int mute = config->readEntry("is_muted", -1);
    if ( mute!=-1 ) {
        playbackVolume().setSwitch( mute!=0 );
    }
    
    int recsrc = config->readEntry("is_recsrc", -1);
    if ( recsrc!=-1 ) {
        captureVolume().setSwitch( recsrc!=0 );
    }
    
    int enumId = config->readEntry("enum_id", -1);
    if ( enumId != -1 ) {
        setEnumId( enumId );
    }
}

/**
 *  called on "kmixctrl --save" and from the GUI's (currently only on exit)
 */
void MixDevice::write( KConfig *config, const QString& grp )
{
   QString devgrp;
   devgrp.sprintf( "%s.Dev%s", grp.toAscii().data(), _id.toAscii().data() );
   config->setGroup(devgrp);
   // kDebug(67100) << "MixDevice::write() of group devgrp=" << devgrp << endl;

    writePlaybackOrCapture(config, "volumeL"       , "volumeR"       , false);
    writePlaybackOrCapture(config, "volumeLCapture", "volumeRCapture", true );
}

void MixDevice::writePlaybackOrCapture(KConfig *config, const char* nameLeftVolume, const char* nameRightVolume, bool capture)
{
#ifdef __GNUC__
#warning Must remove the two (int) casts, once KConfig can write long in writeEntry() again
#endif
    Volume& volume = capture ? captureVolume() : playbackVolume();

    config->writeEntry(nameLeftVolume , (int)volume.getVolume( Volume::LEFT ) );
    config->writeEntry(nameRightVolume, (int)volume.getVolume( Volume::RIGHT ) );

    config->writeEntry("is_muted" , (int)isMuted() );
    config->writeEntry("is_recsrc", (int)isRecSource() );
    config->writeEntry("name", _name);
    if ( isEnum() ) {
        config->writeEntry("enum_id", enumId() );
    }
}


#include "mixdevice.moc"

