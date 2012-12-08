/*
 *              KMix -- KDE's full featured mini mixer
 *              Alsa 0.9x and 1.0 - Based on original alsamixer code
 *              from alsa-project ( www/alsa-project.org )
 *
 *
 * Copyright (C) 2002 Helio Chissini de Castro <helio@conectiva.com.br>
 *               2004 Christian Esken <esken@kde.org>
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


// KMix
#include "mixer_alsa.h"
#include "core/kmixdevicemanager.h"
#include "core/mixer.h"
#include "core/volume.h"

// KDE
#include <kdebug.h>
#include <klocale.h>

// Qt
#include <QList>

// STD Headers
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <qsocketnotifier.h>

//#include "core/mixer.h"
//class Mixer;

// #define if you want MUCH debugging output
//#define ALSA_SWITCH_DEBUG
//#define KMIX_ALSA_VOLUME_DEBUG

Mixer_Backend*
ALSA_getMixer(Mixer *mixer, int device )
{

   Mixer_Backend *l_mixer;

   l_mixer = new Mixer_ALSA(mixer,  device );
   return l_mixer;
}

Mixer_ALSA::Mixer_ALSA( Mixer* mixer, int device ) : Mixer_Backend(mixer,  device )
{
   m_fds = 0;
   _handle = 0;
   ctl_handle = 0;
   _initialUpdate = true;
}

Mixer_ALSA::~Mixer_ALSA()
{
	kDebug() << "Destruct " << this;
   close();
}

int Mixer_ALSA::identify( snd_mixer_selem_id_t *sid )
{
   QString name = snd_mixer_selem_id_get_name( sid );
// kDebug() << name;
   if (name.contains("master"     , Qt::CaseInsensitive)) return MixDevice::VOLUME;
   if (name.contains("master mono", Qt::CaseInsensitive)) return MixDevice::VOLUME;
   if (name.contains("front"      , Qt::CaseInsensitive) &&
      !name.contains("mic"        , Qt::CaseInsensitive)) return MixDevice::VOLUME;
   if (name.contains("pc speaker" , Qt::CaseInsensitive)) return MixDevice::SPEAKER;
   if (name.contains("capture"    , Qt::CaseInsensitive)) return MixDevice::RECMONITOR;
   if (name.contains("music"      , Qt::CaseInsensitive)) return MixDevice::MIDI;
   if (name.contains("Synth"      , Qt::CaseInsensitive)) return MixDevice::MIDI;
   if (name.contains("FM"         , Qt::CaseInsensitive)) return MixDevice::MIDI;
   if (name.contains("headphone"  , Qt::CaseInsensitive)) return MixDevice::HEADPHONE;
   if (name.contains("bass"       , Qt::CaseInsensitive)) return MixDevice::BASS;
   if (name.contains("treble"     , Qt::CaseInsensitive)) return MixDevice::TREBLE;
   if (name.contains("cd"         , Qt::CaseInsensitive)) return MixDevice::CD;
   if (name.contains("video"      , Qt::CaseInsensitive)) return MixDevice::VIDEO;
   if (name.contains("pcm"        , Qt::CaseInsensitive)) return MixDevice::AUDIO;
   if (name.contains("Wave"       , Qt::CaseInsensitive)) return MixDevice::AUDIO;
   if (name.contains("surround"   , Qt::CaseInsensitive)) return MixDevice::SURROUND_BACK;
   if (name.contains("center"     , Qt::CaseInsensitive)) return MixDevice::SURROUND_CENTERFRONT;
   if (name.contains("ac97"       , Qt::CaseInsensitive)) return MixDevice::AC97;
   if (name.contains("coaxial"    , Qt::CaseInsensitive)) return MixDevice::DIGITAL;
   if (name.contains("optical"    , Qt::CaseInsensitive)) return MixDevice::DIGITAL;
   if (name.contains("iec958"     , Qt::CaseInsensitive)) return MixDevice::DIGITAL;
   if (name.contains("digital"    , Qt::CaseInsensitive)) return MixDevice::DIGITAL;
   if (name.contains("mic boost"  , Qt::CaseInsensitive)) return MixDevice::MICROPHONE_BOOST;
   if (name.contains("Mic Front"  , Qt::CaseInsensitive)) return MixDevice::MICROPHONE_FRONT;
   if (name.contains("Front Mic"  , Qt::CaseInsensitive)) return MixDevice::MICROPHONE_FRONT;
   if (name.contains("mic"        , Qt::CaseInsensitive)) return MixDevice::MICROPHONE;
   if (name.contains("lfe"        , Qt::CaseInsensitive)) return MixDevice::SURROUND_LFE;
   if (name.contains("monitor"    , Qt::CaseInsensitive)) return MixDevice::RECMONITOR;
   if (name.contains("3d"         , Qt::CaseInsensitive)) return MixDevice::SURROUND;
   if (name.contains("side"       , Qt::CaseInsensitive)) return MixDevice::SURROUND_BACK;

   return MixDevice::EXTERNAL;
}

int Mixer_ALSA::open()
{
    int masterChosenQuality = 0;
    int err;

    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca( &sid );

    // Determine a card name
    if( m_devnum < -1 || m_devnum > 31 )
        devName = "default";
    else
        devName = QString( "hw:%1" ).arg( m_devnum );

    // Open the card
    err = openAlsaDevice(devName);
    if ( err != 0 ) {
        return err;
    }

    _udi = KMixDeviceManager::instance()->getUDI_ALSA(m_devnum);
    if ( _udi.isEmpty() ) {
        QString msg("No UDI found for '");
        msg += devName;
        msg += "'. Hotplugging not possible";
        kDebug() << msg;
    }
    // Run a loop over all controls of the card
    unsigned int idx = 0;
    for ( elem = snd_mixer_first_elem( _handle ); elem; elem = snd_mixer_elem_next( elem ) )
    {
        // If element is not active, just skip
        if ( ! snd_mixer_selem_is_active ( elem ) ) {
            continue;
        }

        /* --- Create basic control structures: snd_mixer_selem_id_t*, ID, ... --------- */
        // snd_mixer_selem_id_t*
        // I believe we must malloc it ourself (just guessing due to missing ALSA documentation)
        snd_mixer_selem_id_malloc ( &sid ); // !! Return code should be checked. Ressoure must be freed when unplugging card
        snd_mixer_selem_get_id( elem, sid );
        // Generate ID
        QString mdID("%1:%2");
        mdID = mdID.arg(snd_mixer_selem_id_get_name ( sid ) )
                    .arg(snd_mixer_selem_id_get_index( sid ) );
        mdID.replace(' ','_'); // Any key/ID we use, must not uses spaces (rule)

        MixDevice::ChannelType ct = (MixDevice::ChannelType)identify( sid );

        /* ------------------------------------------------------------------------------- */

        Volume* volPlay    = 0;
        Volume* volCapture = 0;
        QList<QString*> enumList;

        if ( snd_mixer_selem_is_enumerated(elem) ) {
            // --- Enumerated ---
            addEnumerated(elem, enumList);
        }
        else
        {
			volPlay    = addVolume(elem, false);
			volCapture = addVolume(elem, true );
        }


        QString readableName;
        readableName = snd_mixer_selem_id_get_name( sid );
        int controlInstanceIndex = snd_mixer_selem_id_get_index( sid );
        if ( controlInstanceIndex > 0 ) {
            // Add a number to the control name, like "PCM 2", when the index is > 0
            QString idxString;
            idxString.setNum(1+controlInstanceIndex);
            readableName += ' ';
            readableName += idxString;
        }

		// There can be an Enum-Control with the same name as a regular control. So we append a ".[cp]enum" prefix to always create a unique ID
        QString finalMixdeviceID = mdID;
        if ( ! enumList.isEmpty() )
        {
        	if (snd_mixer_selem_is_enum_capture ( elem ) )
        		finalMixdeviceID = mdID + ".cenum"; // capture enum
        	else
        		finalMixdeviceID = mdID + ".penum"; // playback enum
        }
        //if ( true || readableName.startsWith("Input Source") ) kDebug() << "Input Source! id=" << finalMixdeviceID << "readableName=" << readableName << ", idx=" << idx;

        m_id2numHash[finalMixdeviceID] = idx;
        //kDebug() << "m_id2numHash[mdID] mdID=" << mdID << " idx=" << idx;
        mixer_elem_list.append( elem );
        mixer_sid_list.append( sid );
        idx++;


        MixDevice* mdNew = new MixDevice(_mixer, finalMixdeviceID, readableName, ct );

        if ( volPlay    != 0      ) mdNew->addPlaybackVolume(*volPlay);
        if ( volCapture != 0      ) mdNew->addCaptureVolume (*volCapture);
       	if ( !enumList.isEmpty()  ) mdNew->addEnums(enumList);

       	shared_ptr<MixDevice> md = mdNew->addToPool();
        m_mixDevices.append( md );
         
        qDeleteAll(enumList); // clear temporary list

        // --- Recommended master ----------------------------------------
        if ( md->playbackVolume().hasVolume() )
        {
          if ( mdID == "Master:0" && masterChosenQuality < 100 ) {
              kDebug() << "Setting m_recommendedMaster to " << mdID;
              m_recommendedMaster = md;
              masterChosenQuality = 100;
          }
          else if ( mdID == "PCM:0" && masterChosenQuality < 80) {
              kDebug() << "Setting m_recommendedMaster to " << mdID;
              m_recommendedMaster = md;
              masterChosenQuality = 80;
          }
          else if ( mdID == "Front:0" && masterChosenQuality < 60) {
              kDebug() << "Setting m_recommendedMaster to " << mdID;
              m_recommendedMaster = md;
              masterChosenQuality = 60;
          }
          else if ( mdID == "DAC:0" && masterChosenQuality < 50) {
              kDebug() << "Setting m_recommendedMaster to " << mdID;
              m_recommendedMaster = md;
              masterChosenQuality = 50;
          }
          else if ( mdID == "Headphone:0" && masterChosenQuality < 40) {
              kDebug() << "Setting m_recommendedMaster to " << mdID;
              m_recommendedMaster = md;
              masterChosenQuality = 40;
          }
          else if ( mdID == "Master Mono:0" && masterChosenQuality < 30) {
              kDebug() << "Setting m_recommendedMaster to " << mdID;
              m_recommendedMaster = md;
              masterChosenQuality = 30;
          }

          //kDebug() << "ALSA create MDW, vol= " << *vol;
          delete volPlay;
          delete volCapture;
        }
    } // for all elems



    m_isOpen = true; // return with success

    setupAlsaPolling();  // For updates
    return 0;
}


/**
 * This opens a ALSA device for further interaction.
 * As this is "slightly" more complicated than calling ::open(),  it is put in a separate method.
 */
int Mixer_ALSA::openAlsaDevice(const QString& devName)
{
    int err;

    QString probeMessage;
    probeMessage += "Trying ALSA Device '" + devName + "': ";

    if ( ( err = snd_ctl_open ( &ctl_handle, devName.toAscii().data(), 0 ) ) < 0 )
    {
        kDebug() << probeMessage << "not found: snd_ctl_open err=" << snd_strerror(err);
        return Mixer::ERR_OPEN;
    }


    // Mixer name
    snd_ctl_card_info_t *hw_info;
    snd_ctl_card_info_alloca(&hw_info);
    if ( ( err = snd_ctl_card_info ( ctl_handle, hw_info ) ) < 0 )
    {
        kDebug() << probeMessage << "not found: snd_ctl_card_info err=" << snd_strerror(err);
        //_stateMessage = errorText( Mixer::ERR_READ );
        snd_ctl_close( ctl_handle );
        return Mixer::ERR_READ;
    }
    const char* mixer_card_name =  snd_ctl_card_info_get_name( hw_info );
    m_mixerName = mixer_card_name;

    snd_ctl_close( ctl_handle );

    /* open mixer device */
    if ( ( err = snd_mixer_open ( &_handle, 0 ) ) < 0 )
    {
        kDebug() << probeMessage << "not found: snd_mixer_open err=" << snd_strerror(err);
        _handle = 0;
        return Mixer::ERR_OPEN; // if we cannot open the mixer, we have no devices
    }

    if ( ( err = snd_mixer_attach ( _handle, devName.toAscii().data() ) ) < 0 )
    {
        kDebug() << probeMessage << "not found: snd_mixer_attach err=" << snd_strerror(err);
        return Mixer::ERR_OPEN;
    }

    if ( ( err = snd_mixer_selem_register ( _handle, NULL, NULL ) ) < 0 )
    {
        kDebug() << probeMessage << "not found: snd_mixer_selem_register err=" << snd_strerror(err);
        return Mixer::ERR_READ;
    }

    if ( ( err = snd_mixer_load ( _handle ) ) < 0 )
    {
        kDebug() << probeMessage << "not found: snd_mixer_load err=" << snd_strerror(err);
        close();
        return Mixer::ERR_READ;
    }

    kDebug() << probeMessage << "found";

    return 0;
}


/* setup for select on stdin and the mixer fd */
int Mixer_ALSA::setupAlsaPolling()
{
	// --- Step 1: Retrieve FD's from ALSALIB
	int err;
	int countNew = 0;
	if ((countNew = snd_mixer_poll_descriptors_count(_handle)) < 0) {
		kDebug() << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" <<  countNew << "\n";
		return Mixer::ERR_OPEN;
	}

	//if ( countNew != m_sns.size() )
	if (true)
	{
		// Redo everything if count of FD's have changed (emulating alsamixer behaviour here)
		 while (!m_sns.isEmpty())
		     delete m_sns.takeFirst();


		free(m_fds);
		m_fds = (struct pollfd*)calloc(countNew, sizeof(struct pollfd));
		if (m_fds == NULL) {
			kDebug() << "Mixer_ALSA::poll() , calloc() = null" << "\n";
			return Mixer::ERR_OPEN;
		}


		if ((err = snd_mixer_poll_descriptors(_handle, m_fds, countNew)) < 0) {
			kDebug() << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" <<  err << "\n";
			return Mixer::ERR_OPEN;
		}
		if (err != countNew) {
			kDebug() << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" << err << " m_count=" <<  countNew << "\n";
			return Mixer::ERR_OPEN;
		}


		// --- Step 2: Create QSocketNotifier's for the FD's
		//m_sns = new QSocketNotifier*[m_count];
		for ( int i = 0; i < countNew; ++i )
		{
			//kDebug() << "socket " << i;
			QSocketNotifier* qsn = new QSocketNotifier(m_fds[i].fd, QSocketNotifier::Read);
			m_sns.append(qsn);
			connect(m_sns[i], SIGNAL(activated(int)), SLOT(readSetFromHW()), Qt::QueuedConnection);
		}
	}

	return 0;
}

void Mixer_ALSA::addEnumerated(snd_mixer_elem_t *elem, QList<QString*>& enumList)
{
   // --- get Enum names START ---
   int numEnumitems = snd_mixer_selem_get_enum_items(elem);
   if ( numEnumitems > 0 ) {
      // OK. no error
      for (int iEnum = 0; iEnum<numEnumitems; iEnum++ ) {
         char buffer[100];
         int ret = snd_mixer_selem_get_enum_item_name(elem, iEnum, 99, buffer);
         buffer[99] = 0; // protect from overflow
         if ( ret == 0 ) {
            QString* enumName = new QString(buffer); // these QString* items are deleted above (search fo "clear temporary list")
            enumList.append( enumName);
         } // enumName could be read successfully
      } // for all enum items of this device
   } // no error in reading enum list
   else {
      // 0 items or Error code => ignore this entry
   }
}


Volume* Mixer_ALSA::addVolume(snd_mixer_elem_t *elem, bool capture)
{
    Volume* vol = 0;
    long maxVolume = 0, minVolume = 0;

    // Add volumes
    if ( !capture && snd_mixer_selem_has_playback_volume(elem) ) {
        snd_mixer_selem_get_playback_volume_range( elem, &minVolume, &maxVolume );
    }
    else if ( capture && snd_mixer_selem_has_capture_volume(elem) ) {
       snd_mixer_selem_get_capture_volume_range( elem, &minVolume, &maxVolume );
    }


    // Check if this control has at least one volume control
    bool hasVolume = snd_mixer_selem_has_playback_volume(elem) || snd_mixer_selem_has_capture_volume(elem);

    // Check if a appropriate switch is present (appropriate means, based o nthe "capture" parameer)
    bool hasCommonSwitch = snd_mixer_selem_has_common_switch ( elem );

    bool hasSwitch = hasCommonSwitch |
        capture
        ? snd_mixer_selem_has_capture_switch ( elem )
        : snd_mixer_selem_has_playback_switch ( elem );

    if ( hasVolume || hasSwitch ) {
        //kDebug() << "Add somthing with chn=" << chn << ", capture=" << capture;
        vol = new Volume( maxVolume, minVolume, hasSwitch, capture);
	
	    // Add volumes
      if ( !capture && snd_mixer_selem_has_playback_volume(elem) ) {
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_FRONT_LEFT  )) vol->addVolumeChannel(VolumeChannel(Volume::LEFT));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_FRONT_RIGHT )) vol->addVolumeChannel(VolumeChannel(Volume::RIGHT));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_FRONT_CENTER)) vol->addVolumeChannel(VolumeChannel(Volume::CENTER));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_REAR_LEFT   )) vol->addVolumeChannel(VolumeChannel(Volume::SURROUNDLEFT));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_REAR_RIGHT  )) vol->addVolumeChannel(VolumeChannel(Volume::SURROUNDRIGHT));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_REAR_CENTER )) vol->addVolumeChannel(VolumeChannel(Volume::REARCENTER));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_WOOFER      )) vol->addVolumeChannel(VolumeChannel(Volume::WOOFER));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_SIDE_LEFT   )) vol->addVolumeChannel(VolumeChannel(Volume::REARSIDELEFT));
	  if ( snd_mixer_selem_has_playback_channel(elem,SND_MIXER_SCHN_SIDE_RIGHT  )) vol->addVolumeChannel(VolumeChannel(Volume::REARSIDERIGHT));
      }
      else if ( capture && snd_mixer_selem_has_capture_volume(elem) ) {
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_FRONT_LEFT  )) vol->addVolumeChannel(VolumeChannel(Volume::LEFT));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_FRONT_RIGHT )) vol->addVolumeChannel(VolumeChannel(Volume::RIGHT));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_FRONT_CENTER)) vol->addVolumeChannel(VolumeChannel(Volume::CENTER));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_REAR_LEFT   )) vol->addVolumeChannel(VolumeChannel(Volume::SURROUNDLEFT));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_REAR_RIGHT  )) vol->addVolumeChannel(VolumeChannel(Volume::SURROUNDRIGHT));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_REAR_CENTER )) vol->addVolumeChannel(VolumeChannel(Volume::REARCENTER));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_WOOFER      )) vol->addVolumeChannel(VolumeChannel(Volume::WOOFER));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_SIDE_LEFT   )) vol->addVolumeChannel(VolumeChannel(Volume::REARSIDELEFT));
	  if ( snd_mixer_selem_has_capture_channel(elem,SND_MIXER_SCHN_SIDE_RIGHT  )) vol->addVolumeChannel(VolumeChannel(Volume::REARSIDERIGHT));
      }
    }

    return vol;
}


void Mixer_ALSA::deinitAlsaPolling()
{
	if ( m_fds )
		free( m_fds );
	m_fds = 0;

	while (!m_sns.isEmpty())
		delete m_sns.takeFirst();
}

int
Mixer_ALSA::close()
{
	kDebug() << "close " << this;

  int ret=0;
  m_isOpen = false;

  if ( ctl_handle != 0)
  {
	  //snd_ctl_close( ctl_handle );
	  ctl_handle = 0;
  }

  if ( _handle != 0 )
  {
    //kDebug() << "IN  Mixer_ALSA::close()";
    snd_mixer_free ( _handle );
    if ( ( ret = snd_mixer_detach ( _handle, devName.toAscii().data() ) ) < 0 )
    {
        kDebug() << "snd_mixer_detach err=" << snd_strerror(ret);
    }
    int ret2 = 0;
    if ( ( ret2 = snd_mixer_close ( _handle ) ) < 0 )
    {
        kDebug() << "snd_mixer_close err=" << snd_strerror(ret2);
	if ( ret == 0 ) ret = ret2; // no error before => use current error code
    }

    _handle = 0;
    //kDebug() << "OUT Mixer_ALSA::close()";

  }

  mixer_elem_list.clear();
  mixer_sid_list.clear();
  m_id2numHash.clear();

  deinitAlsaPolling();

  closeCommon();
  return ret;
}


/**
 * Resolve index to a control (snd_mixer_elem_t*)
 * @par idx Index to query. For any invalid index (including -1) returns a 0 control.
 */
snd_mixer_elem_t* Mixer_ALSA::getMixerElem(int idx) {
	snd_mixer_elem_t* elem = 0;
        if ( ! m_isOpen ) return elem; // unplugging guard

	if ( idx == -1 ) {
		return elem;
	}
	if ( int( mixer_sid_list.count() ) > idx ) {
		snd_mixer_selem_id_t * sid = mixer_sid_list[ idx ];
		// The next line (hopefully) only finds selem's, not elem's.
		elem = snd_mixer_find_selem(_handle, sid);

		if ( elem == 0 ) {
			// !! Check, whether the warning should be omitted. Probably
			//    Route controls are non-simple elements.
			kDebug() << "Error finding mixer element " << idx;
		}
	}
	return elem;

/*
 I would have liked to use the following trivial implementation instead of the
 code above. But it will also return elem's. which are not selem's. As there is
 no way to check an elem's type (e.g. elem->type == SND_MIXER_ELEM_SIMPLE), callers
 of getMixerElem() cannot check the type. :-(
	snd_mixer_elem_t* elem = mixer_elem_list[ devnum ];
	return elem;
 */
}

int Mixer_ALSA::id2num(const QString& id) {
   //kDebug() << "id2num() id=" << id;
   int num = -1;
   if ( m_id2numHash.contains(id) ) {
      num = m_id2numHash[id];
   }
   //kDebug() << "id2num() num=" << num;
   return num;
}

bool Mixer_ALSA::prepareUpdateFromHW() {
    if ( !m_fds || !m_isOpen )
        return false;

    setupAlsaPolling();
    // Poll on fds with 10ms timeout
    // Hint: alsamixer has an infinite timeout, but we cannot do this because we would block
    // the X11 event handling (Qt event loop) with this.
    int finished = poll(m_fds, m_sns.size(), 10);

    bool updated = false;

    if (finished > 0) {
        //kDebug() << "Mixer_ALSA::prepareUpdate() 5\n";

        unsigned short revents;

        if (snd_mixer_poll_descriptors_revents(_handle, m_fds, m_sns.size(), &revents) >= 0)
        {
        //kDebug() << "Mixer_ALSA::prepareUpdate() 6\n";

            if (revents & POLLNVAL) {
                /* Bug 127294 shows, that we receive POLLNVAL when the user
                    unplugs an USB soundcard. Lets close the card. */
                kDebug() << "Mixer_ALSA::poll() , Error: poll() returns POLLNVAL\n";
                close();  // Card was unplugged (unplug, driver unloaded)
                return false;
            }
            if (revents & POLLERR) {
                kDebug() << "Mixer_ALSA::poll() , Error: poll() returns POLLERR\n";
                return false;
            }
            if (revents & POLLIN) {
                //kDebug() << "Mixer_ALSA::prepareUpdate() 7\n";
                snd_mixer_handle_events(_handle);
                updated = true;
            }
        }
    }

    //kDebug() << "Mixer_ALSA::prepareUpdate() 8\n";
    return updated;
}

bool Mixer_ALSA::isRecsrcHW( const QString& id )
{
    int devnum = id2num(id);
    bool isCurrentlyRecSrc = false;
    snd_mixer_elem_t *elem = getMixerElem( devnum );

    if ( !elem ) {
        return false;
    }

    if ( snd_mixer_selem_has_capture_switch( elem ) ) {
        // Has a on-off switch
        // Yes, this element can be record source. But the user can switch it off, so lets see if it is switched on.
        int swLeft;
        int ret = snd_mixer_selem_get_capture_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, &swLeft );
        if ( ret != 0 ) kDebug() << "snd_mixer_selem_get_capture_switch() failed 1\n";

        if (snd_mixer_selem_has_capture_switch_joined( elem ) ) {
            isCurrentlyRecSrc = (swLeft != 0);
#ifdef ALSA_SWITCH_DEBUG
            kDebug() << "has_switch joined: #" << devnum << " >>> " << swLeft << " : " << isCurrentlyRecSrc;
#endif
        }
        else {
            int swRight;
            snd_mixer_selem_get_capture_switch( elem, SND_MIXER_SCHN_FRONT_RIGHT, &swRight );
            isCurrentlyRecSrc = ( (swLeft != 0) || (swRight != 0) );
#ifdef ALSA_SWITCH_DEBUG
            kDebug() << "has_switch non-joined, state " << isCurrentlyRecSrc;
#endif
        }
    }
    else {
        // Has no on-off switch
        if ( snd_mixer_selem_has_capture_volume( elem ) ) {
            // Has a volume, but has no OnOffSwitch => We assume that this is a fixed record source (always on). (esken)
            isCurrentlyRecSrc = true;
#ifdef ALSA_SWITCH_DEBUG
            kDebug() << "has_no_switch, state " << isCurrentlyRecSrc;
#endif
        }
    }

    return isCurrentlyRecSrc;
}


/**
 * Sets the ID of the currently selected Enum entry.
 * Warning: ALSA supports to have different enums selected on each channel
 *          of the SAME snd_mixer_elem_t. KMix does NOT support that and
 *          always sets both channels (0 and 1).
 */
void Mixer_ALSA::setEnumIdHW(const QString& id, unsigned int idx) {
    //kDebug() << "Mixer_ALSA::setEnumIdHW() id=" << id << " , idx=" << idx << ") 1\n";
    int devnum = id2num(id);
    snd_mixer_elem_t *elem = getMixerElem( devnum );

    for (int i = 0; i <= SND_MIXER_SCHN_LAST; ++i)
    {
		int ret = snd_mixer_selem_set_enum_item(elem, (snd_mixer_selem_channel_id_t)i,idx);
		if (ret < 0 && i == 0)
		{
			// Log errors only for one channel. This should be enough, and another reason is that I also do not check which channels are supported at all.
			kError() << "Mixer_ALSA::setEnumIdHW(" << devnum << "), errno=" << ret << "\n";
		}
	}

    return;
}

/**
 * Return the ID of the currently selected Enum entry.
 * Warning: ALSA supports to have different enums selected on each channel
 *          of the SAME snd_mixer_elem_t. KMix does NOT support that and
 *          always shows the value of the first channel.
 */
unsigned int Mixer_ALSA::enumIdHW(const QString& id) {
    int devnum = id2num(id);
    snd_mixer_elem_t *elem = getMixerElem( devnum );
    unsigned int idx = 0;

    if ( elem != 0 && snd_mixer_selem_is_enumerated(elem) )
    {
        int ret = snd_mixer_selem_get_enum_item(elem,SND_MIXER_SCHN_FRONT_LEFT,&idx);
        if (ret < 0) {
            idx = 0;
            kError() << "Mixer_ALSA::enumIdHW(" << devnum << "), errno=" << ret << "\n";
        }
    }
    return idx;
}


int
Mixer_ALSA::readVolumeFromHW( const QString& id, shared_ptr<MixDevice> md )
{
    Volume& volumePlayback = md->playbackVolume();
    Volume& volumeCapture  = md->captureVolume();
    int devnum = id2num(id);
    int elem_sw;
    long vol;

    snd_mixer_elem_t *elem = getMixerElem( devnum );
    if ( !elem )
    {
        return Mixer::OK_UNCHANGED;
    }

    vol = Volume::MNONE;
    // --- playback volume
    if ( snd_mixer_selem_has_playback_volume( elem ) )
    {
    	if ( md->isVirtuallyMuted() )
    	{
    		// Special code path for controls w/o physical mute switch. Doing it in all backends is not perfect,
    		// but it saves a lot of code and removes a lot of complexity in the Volume and MixDevice classes.

    		// Don't feed back the actual 0 volume back from the device to KMix. Just do nothing!
    	}
    	else
    	{
        foreach (VolumeChannel vc, volumePlayback.getVolumes() )
        {
               int ret = 0;
               switch(vc.chid) {
                   case Volume::LEFT         : ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_FRONT_LEFT  , &vol); break;
                   case Volume::RIGHT        : ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_FRONT_RIGHT , &vol); break;
                   case Volume::CENTER       : ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_FRONT_CENTER, &vol); break;
                   case Volume::SURROUNDLEFT : ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_REAR_LEFT   , &vol); break;
                   case Volume::SURROUNDRIGHT: ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_REAR_RIGHT  , &vol); break;
                   case Volume::REARCENTER   : ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_REAR_CENTER , &vol); break;
                   case Volume::WOOFER       : ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_WOOFER      , &vol); break;
                   case Volume::REARSIDELEFT : ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_SIDE_LEFT   , &vol); break;
                   case Volume::REARSIDERIGHT: ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_SIDE_RIGHT  , &vol); break;
                   default: kDebug() << "FATAL: Unknown channel type for playback << " << vc.chid << " ... please report this";  break;
              }
              if ( ret != 0 )
		kDebug() << "readVolumeFromHW(" << devnum << ") [get_playback_volume] failed, errno=" << ret;
              else
		volumePlayback.setVolume( vc.chid, vol);
              //if (id== "Master:0" || id== "PCM:0" ) { kDebug() << "volumePlayback control=" << id << ", chid=" << i << ", vol=" << vol; }
       }
    	}
    } // has playback volume

    // --- playback switch
    // TODO: What about has_common_switch()
    if ( snd_mixer_selem_has_playback_switch( elem ) )
    {
        snd_mixer_selem_get_playback_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, &elem_sw );
        md->setMuted( elem_sw == 0 );
    }

    vol = Volume::MNONE;
    // --- capture volume
    if ( snd_mixer_selem_has_capture_volume ( elem ) )
    {
        foreach (VolumeChannel vc, volumeCapture.getVolumes() )
		{
               int ret = 0;
               switch(vc.chid) {
                   case Volume::LEFT         : ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_FRONT_LEFT  , &vol); break;
                   case Volume::RIGHT        : ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_FRONT_RIGHT , &vol); break;
                   case Volume::CENTER       : ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_FRONT_CENTER, &vol); break;
                   case Volume::SURROUNDLEFT : ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_REAR_LEFT   , &vol); break;
                   case Volume::SURROUNDRIGHT: ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_REAR_RIGHT  , &vol); break;
                   case Volume::REARCENTER   : ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_REAR_CENTER , &vol); break;
                   case Volume::WOOFER       : ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_WOOFER      , &vol); break;
                   case Volume::REARSIDELEFT : ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_SIDE_LEFT   , &vol); break;
                   case Volume::REARSIDERIGHT: ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_SIDE_RIGHT  , &vol);     break;
                   default: kDebug() << "FATAL: Unknown channel type for capture << " << vc.chid << " ... please report this";  break;
              }
              if ( ret != 0 ) kDebug() << "readVolumeFromHW(" << devnum << ") [get_capture_volume] failed, errno=" << ret;
              else volumeCapture.setVolume( vc.chid, vol);
       }
    } // has capture volume

    // --- capture switch
    // TODO: What about has_common_switch()
    if ( snd_mixer_selem_has_capture_switch( elem ) )
    {
        snd_mixer_selem_get_capture_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, &elem_sw );
        md->setRecSource( elem_sw == 1 );

        // Refresh the capture switch information of *all* controls of this card.
        // Doing it for all is necessary, because enabling one record source often
        // automatically disables another record source (due to the hardware design)
        foreach ( shared_ptr<MixDevice> md, m_mixDevices )
        {
            bool isRecsrc =  isRecsrcHW( md->id() );
            // kDebug() << "Mixer::setRecordSource(): isRecsrcHW(" <<  md->id() << ") =" <<  isRecsrc;
            md->setRecSource( isRecsrc );
        }

    }

    // The state Mixer::OK_UNCHANGED is not implemented. It is not strictly required for
    //  non-pollling backends. 
    return Mixer::OK;
}

int
Mixer_ALSA::writeVolumeToHW( const QString& id, shared_ptr<MixDevice> md )
{
    Volume& volumePlayback = md->playbackVolume();
    Volume& volumeCapture  = md->captureVolume();

    int devnum = id2num(id);

    snd_mixer_elem_t *elem = getMixerElem( devnum );
    if ( !elem )
    {
        return 0;
    }


    // --- playback switch
    bool hasPlaybackSwitch = snd_mixer_selem_has_playback_switch( elem ) || snd_mixer_selem_has_common_switch  ( elem );
	if (hasPlaybackSwitch)
	{
		int sw = 0;
		if (!md->isMuted())
			sw = !sw; // invert all bits
		snd_mixer_selem_set_playback_switch_all(elem, sw);
	}


    // --- playback volume
    if ( snd_mixer_selem_has_playback_volume( elem ) )
    {
    	kDebug() << "phys=" << md->hasPhysicalMuteSwitch() << ", muted=" << md->isMuted();
    	if ( md->isVirtuallyMuted() )
    	{
    		// Special code path for controls w/o physical mute switch. Doing it in all backends is not perfect,
    		// but it saves a lot of code and removes a lot of complexity in the Volume and MixDevice classes.
    		int ret = snd_mixer_selem_set_playback_volume_all( elem, (long)0);
            if ( ret != 0 )
          	  kDebug() << "writeVolumeToHW(" << devnum << ") [set_playback_volume] failed, errno=" << ret;
    	}
    	else
    	{
      foreach (VolumeChannel vc, volumePlayback.getVolumes() )
      {
               int ret = 0;
               switch(vc.chid)
               {
                   case Volume::LEFT         : ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_FRONT_LEFT  , vc.volume); break;
                   case Volume::RIGHT        : ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_FRONT_RIGHT , vc.volume); break;
                   case Volume::CENTER       : ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_FRONT_CENTER, vc.volume); break;
                   case Volume::SURROUNDLEFT : ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_REAR_LEFT   , vc.volume); break;
                   case Volume::SURROUNDRIGHT: ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_REAR_RIGHT  , vc.volume); break;
                   case Volume::REARCENTER   : ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_REAR_CENTER , vc.volume); break;
                   case Volume::WOOFER       : ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_WOOFER      , vc.volume); break;
                   case Volume::REARSIDELEFT : ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_SIDE_LEFT   , vc.volume); break;
                   case Volume::REARSIDERIGHT: ret = snd_mixer_selem_set_playback_volume( elem, SND_MIXER_SCHN_SIDE_RIGHT  , vc.volume); break;
                   default: kDebug() << "FATAL: Unknown channel type for playback << " << vc.chid << " ... please report this";  break;
              }
              if ( ret != 0 )
            	  kDebug() << "writeVolumeToHW(" << devnum << ") [set_playback_volume] failed, errno=" << ret;
              //if (id== "Master:0" || id== "PCM:0" ) { kDebug() << "volumePlayback control=" << id << ", chid=" << vc.chid << ", vol=" << vc.volume; }
          }
    	}
    } // has playback volume



    // --- capture volume
    if ( snd_mixer_selem_has_capture_volume ( elem ) )
    {
      foreach (VolumeChannel vc, volumeCapture.getVolumes() )
      {
               int ret = 0;
               switch(vc.chid) {
                   case Volume::LEFT         : ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_FRONT_LEFT  , vc.volume); break;
                   case Volume::RIGHT        : ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_FRONT_RIGHT , vc.volume); break;
                   case Volume::CENTER       : ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_FRONT_CENTER, vc.volume); break;
                   case Volume::SURROUNDLEFT : ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_REAR_LEFT   , vc.volume); break;
                   case Volume::SURROUNDRIGHT: ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_REAR_RIGHT  , vc.volume); break;
                   case Volume::REARCENTER   : ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_REAR_CENTER , vc.volume); break;
                   case Volume::WOOFER       : ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_WOOFER      , vc.volume); break;
                   case Volume::REARSIDELEFT : ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_SIDE_LEFT   , vc.volume); break;
                   case Volume::REARSIDERIGHT: ret = snd_mixer_selem_set_capture_volume( elem, SND_MIXER_SCHN_SIDE_RIGHT  , vc.volume); break;
                   default: kDebug() << "FATAL: Unknown channel type for capture << " << vc.chid << " ... please report this";  break;
              }
              if ( ret != 0 ) kDebug() << "writeVolumeToHW(" << devnum << ") [set_capture_volume] failed, errno=" << ret;
              //if (id== "Master:0" || id== "PCM:0" ) { kDebug() << "volumecapture control=" << id << ", chid=" << i << ", vol=" << vc.volume; }
          }
    } // has capture volume

    // --- capture switch
    if ( snd_mixer_selem_has_capture_switch( elem ) ) {
        //  Hint: snd_mixer_selem_has_common_switch() is already covered in the playback .
        //     switch. This is probably enough. It would be helpful, if the ALSA project would
        //     write documentation. Until then, I need to continue guessing semantics.
        int sw = 0;
        if ( md->isRecSource())
            sw = !sw; // invert all bits
        snd_mixer_selem_set_capture_switch_all( elem, sw );
    }

    return 0;
}

QString
Mixer_ALSA::errorText( int mixer_error )
{
	QString l_s_errmsg;
	switch ( mixer_error )
	{
		case Mixer::ERR_PERM:
			l_s_errmsg = i18n("You do not have permission to access the alsa mixer device.\n" \
					"Please verify if all alsa devices are properly created.");
      break;
		case Mixer::ERR_OPEN:
			l_s_errmsg = i18n("Alsa mixer cannot be found.\n" \
					"Please check that the soundcard is installed and the\n" \
					"soundcard driver is loaded.\n" );
			break;
		default:
			l_s_errmsg = Mixer_Backend::errorText( mixer_error );
	}
	return l_s_errmsg;
}


QString
ALSA_getDriverName()
{
	return "ALSA";
}

QString Mixer_ALSA::getDriverName()
{
	return "ALSA";
}


