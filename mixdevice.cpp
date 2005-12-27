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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
MixDevice::MixDevice( int num, Volume &vol, bool recordable, bool mute,
		      QString name, ChannelType type, DeviceCategory category ) :
    _volume( vol ), _type( type ), _num( num ), _recordable( recordable ),
    _mute( mute ), _category( category )
{
    // Hint: "_volume" gets COPIED from "vol" due to the fact that the copy-constructor actually copies the volume levels.
    _switch = false;
    _recSource = false;
    if( name.isEmpty() )
	_name = i18n("unknown");
    else
	_name = name;

    _pk.setNum(num);


    if( category == MixDevice::SWITCH )
	_switch = true;
}

MixDevice::MixDevice(const MixDevice &md) : QObject()
{
   _name = md._name;
   _volume = md._volume;
   _type = md._type;
   _num = md._num;
   _pk = md._pk;
   _recordable = md._recordable;
   _recSource  = md._recSource;
   _category = md._category;
   _switch = md._switch;
   _mute = md._mute;
   _enumValues = md._enumValues;
}

MixDevice::~MixDevice() {
  // Clear MixDevices enum Strings (switch on auto-delete, so the QString's inside will be cleared)
  _enumValues.setAutoDelete(true);
  _enumValues.clear();
}

Volume& MixDevice::getVolume()
{
   return _volume;
}

long MixDevice::getVolume(Volume::ChannelID chid) {
    return _volume.getVolume(chid);
}

long MixDevice::maxVolume() {
    return _volume.maxVolume();
}

long MixDevice::minVolume() {
    return _volume.minVolume();
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

QPtrList<QString>& MixDevice::enumValues() {
  return _enumValues;
}


// @todo Used only at mixdevicewidget.cpp:625 . Replace that ASAP  !!!
void MixDevice::setVolume( int channel, int volume )
{
  _volume.setVolume( (Volume::ChannelID)channel /* ARGH! */, volume );
}

QString& MixDevice::getPK() {
    return _pk;
}

void MixDevice::setPK(QString &PK) {
    _pk = PK;
    // The key is used in the config file. It should not contain spaces
    _pk.replace(' ', '_');
}

/**
 * This mehtod is currently only called on "kmixctrl --restore"
 *
 * Normally we have a working _volume object already, which is very important,
 * because we need to read the minimum and maximum volume levels.
 * (Another solutien would be to "equip" volFromConfig with maxInt and minInt values).
 */
void MixDevice::read( KConfig *config, const QString& grp )
{
   QString devgrp;
   devgrp.sprintf( "%s.Dev%i", grp.ascii(), _num );
   config->setGroup( devgrp );
   //kdDebug(67100) << "MixDevice::read() of group devgrp=" << devgrp << endl;

   char *nameLeftVolume, *nameRightVolume;
   if ( _volume.isCapture() ) {
		nameLeftVolume = "volumeLCapture";
		nameRightVolume = "volumeRCapture";
   } else {
		nameLeftVolume = "volumeL";
		nameRightVolume = "volumeR";
   }
   Volume::ChannelMask chMask = Volume::MNONE;
   int vl = config->readNumEntry(nameLeftVolume, -1);
   if (vl!=-1) {
        chMask = (Volume::ChannelMask)(chMask | Volume::MLEFT);
   }
   int vr = config->readNumEntry(nameRightVolume, -1);
   if (vr!=-1) {
       chMask = (Volume::ChannelMask)(chMask | Volume::MRIGHT);
   }

   /*
    * Now start construction a temporary Volume object.
    * We take the maxvol and minvol values from _volume, which is already constructed.
    * Otherwise we would have to wildly guess those values
    */
   Volume *volFromConfig = new Volume(chMask, _volume._maxVolume, _volume._minVolume);
   if (vl!=-1) {
       volFromConfig->setVolume(Volume::LEFT , vl);
   }
   if (vr!=-1) {
       volFromConfig->setVolume(Volume::RIGHT, vr);
   }
   // commit the read config
   _volume.setVolume(*volFromConfig);
   delete volFromConfig;

   int mute = config->readNumEntry("is_muted", -1);
   if ( mute!=-1 ) {
        _volume.setMuted( mute!=0 );
   }

   int recsrc = config->readNumEntry("is_recsrc", -1);
   if ( recsrc!=-1 ) {
        setRecSource( recsrc!=0 );
   }

   int enumId = config->readNumEntry("enum_id", -1);
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
   devgrp.sprintf( "%s.Dev%i", grp.ascii(), _num );
   config->setGroup(devgrp);
   // kdDebug(67100) << "MixDevice::write() of group devgrp=" << devgrp << endl;
   char *nameLeftVolume, *nameRightVolume;
   if ( _volume.isCapture() ) {
		nameLeftVolume = "volumeLCapture";
		nameRightVolume = "volumeRCapture";
   } else {
		nameLeftVolume = "volumeL";
		nameRightVolume = "volumeR";
   }
   config->writeEntry(nameLeftVolume, getVolume( Volume::LEFT ) );
   config->writeEntry(nameRightVolume, getVolume( Volume::RIGHT ) );
   config->writeEntry("is_muted", (int)_volume.isMuted() );
   config->writeEntry("is_recsrc", (int)isRecSource() );
   config->writeEntry("name", _name);
   if ( isEnum() ) {
      config->writeEntry("enum_id", enumId() );
   }
}

#include "mixdevice.moc"

