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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// STD Headers
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <qsocketnotifier.h>

extern "C"
{
	#include <alsa/asoundlib.h>
}

// KDE Headers
#include <klocale.h>
#include <kdebug.h>

// Local Headers
#include "mixer_alsa.h"
//#include "mixer.h"
#include "volume.h"
// #define if you want MUCH debugging output
//#define ALSA_SWITCH_DEBUG
//#define KMIX_ALSA_VOLUME_DEBUG

Mixer_Backend*
ALSA_getMixer( int device )
{
	Mixer_Backend *l_mixer;
        l_mixer = new Mixer_ALSA( device );
	return l_mixer;
}

Mixer_ALSA::Mixer_ALSA( int device ) : Mixer_Backend( device )
{
    m_fds = 0;
    m_sns = 0;
    _handle = 0;
    _initialUpdate = true;
}

Mixer_ALSA::~Mixer_ALSA()
{
  close();
}

int
Mixer_ALSA::identify( snd_mixer_selem_id_t *sid )
{
	QString name = snd_mixer_selem_id_get_name( sid );

	if ( name == "Master" ) return MixDevice::VOLUME;
        if ( name == "Capture" ) return MixDevice::RECMONITOR;
	if ( name == "Master Mono" ) return MixDevice::VOLUME;
        if ( name == "PC Speaker" ) return MixDevice::VOLUME;
        if ( name == "Music" || name == "Synth" || name == "FM" ) return MixDevice::MIDI;
	if ( name.find( "Headphone", 0, false ) != -1 ) return MixDevice::HEADPHONE;
	if ( name == "Bass" ) return MixDevice::BASS;
	if ( name == "Treble" ) return MixDevice::TREBLE;
	if ( name == "CD" ) return MixDevice::CD;
	if ( name == "Video" ) return MixDevice::VIDEO;
	if ( name == "PCM" || name == "Wave" ) return MixDevice::AUDIO;
	if ( name == "Surround" ) return MixDevice::SURROUND_BACK;
	if ( name == "Center" ) return MixDevice::SURROUND_CENTERFRONT;
	if ( name.find( "ac97", 0, false ) != -1 ) return MixDevice::AC97;
	if ( name.find( "coaxial", 0, false ) != -1 ) return MixDevice::DIGITAL;
	if ( name.find( "optical", 0, false ) != -1 ) return MixDevice::DIGITAL;
	if ( name.find( "IEC958", 0, false ) != -1 ) return MixDevice::DIGITAL;
	if ( name.find( "Mic" ) != -1 ) return MixDevice::MICROPHONE;
	if ( name.find( "LFE" ) != -1 ) return MixDevice::SURROUND_LFE;
        if ( name.find( "Monitor" ) != -1 ) return MixDevice::RECMONITOR;
	if ( name.find( "3D", 0, false ) != -1 ) return MixDevice::SURROUND;  // Should be probably some own icon

	return MixDevice::EXTERNAL;
}

int
Mixer_ALSA::open()
{
    bool virginOpen = m_mixDevices.isEmpty();
    bool validDevice = false;
    bool masterChosen = false;
    int err;

    snd_ctl_t *ctl_handle;
    snd_ctl_card_info_t *hw_info;
    snd_ctl_card_info_alloca(&hw_info);

    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca( &sid );

    // Card information
    if( m_devnum == -1 )
        m_devnum = 0;
    if ( (unsigned)m_devnum > 31 )
	devName = "default";
    else
	devName = QString( "hw:%1" ).arg( m_devnum );

    QString probeMessage;

    if (virginOpen)
	probeMessage += "Trying ALSA Device '" + devName + "': ";

    if ( ( err = snd_ctl_open ( &ctl_handle, devName.latin1(), 0 ) ) < 0 )
    {
	kdDebug(67100) << probeMessage << "not found: snd_ctl_open err=" << snd_strerror(err) << endl;
	//_stateMessage = errorText( Mixer::ERR_NODEV );
	return Mixer::ERR_OPEN;
    }

    if ( ( err = snd_ctl_card_info ( ctl_handle, hw_info ) ) < 0 )
    {
	kdDebug(67100) << probeMessage << "not found: snd_ctl_card_info err=" << snd_strerror(err) << endl;
	//_stateMessage = errorText( Mixer::ERR_READ );
	snd_ctl_close( ctl_handle );
	return Mixer::ERR_READ;
    }

    // Device and mixer names
    const char* mixer_card_name =  snd_ctl_card_info_get_name( hw_info );
    //mixer_device_name = snd_ctl_card_info_get_mixername( hw_info );
    // Copy the name of kmix mixer from card name (mixername is rumoured to be not that good)
    m_mixerName = mixer_card_name;

    snd_ctl_close( ctl_handle );

    /* open mixer device */

    //kdDebug(67100) << "IN  Mixer_ALSA snd_mixer_open()" << endl;
    if ( ( err = snd_mixer_open ( &_handle, 0 ) ) < 0 )
    {
	kdDebug(67100) << probeMessage << "not found: snd_mixer_open err=" << snd_strerror(err) << endl;
	//errormsg( Mixer::ERR_NODEV );
	_handle = 0;
	return Mixer::ERR_NODEV; // if we cannot open the mixer, we have no devices
    }
    //kdDebug(67100) << "OUT Mixer_ALSA snd_mixer_open()" << endl;

    if ( ( err = snd_mixer_attach ( _handle, devName.latin1() ) ) < 0 )
    {
	kdDebug(67100) << probeMessage << "not found: snd_mixer_attach err=" << snd_strerror(err) << endl;
	//errormsg( Mixer::ERR_PERM );
	return Mixer::ERR_OPEN;
    }

    if ( ( err = snd_mixer_selem_register ( _handle, NULL, NULL ) ) < 0 )
    {
	kdDebug(67100) << probeMessage << "not found: snd_mixer_selem_register err=" << snd_strerror(err) << endl;
	//errormsg( Mixer::ERR_READ );
	return Mixer::ERR_READ;
    }

    if ( ( err = snd_mixer_load ( _handle ) ) < 0 )
    {
	kdDebug(67100) << probeMessage << "not found: snd_mixer_load err=" << snd_strerror(err) << endl;
	//errormsg( Mixer::ERR_READ );
	close();
	return Mixer::ERR_READ;
    }

    kdDebug(67100) << probeMessage << "found" << endl;

    unsigned int mixerIdx = 0;
    for ( elem = snd_mixer_first_elem( _handle ); elem; elem = snd_mixer_elem_next( elem ), mixerIdx++ )
    {
	// If element is not active, just skip
	if ( ! snd_mixer_selem_is_active ( elem ) ) {
	    // ...but we still want to insert a null value into our mixer element
	    // list so that the list indexes match up.
	    mixer_elem_list.append( 0 );
	    mixer_sid_list.append( 0 );
	    continue;
	}


	sid = (snd_mixer_selem_id_t*)malloc(snd_mixer_selem_id_sizeof());  // I believe *we* must malloc it for ourself
	snd_mixer_selem_get_id( elem, sid );

	bool canRecord = false;
	bool canMute = false;
	bool canCapture = false;
	long maxVolumePlay= 0, minVolumePlay= 0;
	long maxVolumeRec = 0, minVolumeRec = 0;
	validDevice = true;

	snd_mixer_selem_get_playback_volume_range( elem, &minVolumePlay, &maxVolumePlay );
	snd_mixer_selem_get_capture_volume_range( elem, &minVolumeRec , &maxVolumeRec  );
	// New mix device
	MixDevice::ChannelType ct = (MixDevice::ChannelType)identify( sid );
/*
        if (!masterChosen && ct==MixDevice::VOLUME) {
           // Determine a nicer MasterVolume
	   m_masterDevice = mixerIdx;
           masterChosen = true;
        }
*/
	if( virginOpen )
	{
	    MixDevice::DeviceCategory cc = MixDevice::UNDEFINED;

		//kdDebug() << "--- Loop: name=" << snd_mixer_selem_id_get_name( sid ) << " , mixerIdx=" << mixerIdx << "------------" << endl;

	    Volume* volPlay = 0, *volCapture = 0;
	    QPtrList<QString> enumList;
	    if ( snd_mixer_selem_is_enumerated(elem) ) {
		cc = MixDevice::ENUM;
		volPlay = new Volume(); // Dummy, unused
		volCapture = new Volume();
		mixer_elem_list.append( elem );
		mixer_sid_list.append( sid );

		// --- get Enum names START ---
		int numEnumitems = snd_mixer_selem_get_enum_items(elem);
		if ( numEnumitems > 0 ) {
		  // OK. no error
		  for (int iEnum = 0; iEnum<numEnumitems; iEnum++ ) {
		    char buffer[100];
		    int ret = snd_mixer_selem_get_enum_item_name(elem, iEnum, 99, buffer);
		    if ( ret == 0 ) {
		      QString* enumName = new QString(buffer);
		      //enumName->append(buffer);
		      enumList.append( enumName);
		    } // enumName could be read succesfully
		  } // for all enum items of this device
		} // no error in reading enum list
		else {
		  // 0 items or Error code => ignore this entry
		}
		// --- get Enum names END ---
	    } // is an enum

	    else {
		Volume::ChannelMask chn = Volume::MNONE;
		Volume::ChannelMask chnTmp;
		if ( snd_mixer_selem_has_playback_volume(elem) ) {
			//kdDebug(67100) << "has_playback_volume()" << endl;
			chnTmp = snd_mixer_selem_is_playback_mono ( elem )
				? Volume::MLEFT : (Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT);
			chn = (Volume::ChannelMask) (chn | chnTmp);
			cc = MixDevice::SLIDER;
			volPlay = new Volume( chn, maxVolumePlay, minVolumePlay );
		} else {
			volPlay = new Volume();
		}
		if ( snd_mixer_selem_has_capture_volume(elem) ) {
			//kdDebug(67100) << "has_capture_volume()" << endl;
			chnTmp = snd_mixer_selem_is_capture_mono( elem )
				? Volume::MLEFT : (Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT );
			chn = (Volume::ChannelMask) (chn | chnTmp);
			cc = MixDevice::SLIDER;
			canCapture = true;
			volCapture = new Volume( chn, maxVolumeRec, minVolumeRec, true );
		} else {
			volCapture = new Volume();
		}

		/* Create Volume object. If there is no volume on this device,
		 * it will be created with maxVolume == 0 && minVolume == 0 */
		mixer_elem_list.append( elem );
		mixer_sid_list.append( sid );

		if ( snd_mixer_selem_has_playback_switch ( elem ) ) {
			//kdDebug(67100) << "has_playback_switch()" << endl;
			canMute = true;
		}
		if ( snd_mixer_selem_has_capture_switch ( elem ) )	{
			//kdDebug(67100) << "has_capture_switch()" << endl;
			canRecord = true;
		}
		if ( snd_mixer_selem_has_common_switch ( elem ) ) {
			//kdDebug(67100) << "has_common_switch()" << endl;
			canMute = true;
			canRecord = true;
		}

		if ( /*snd_mixer_selem_has_common_switch ( elem ) || */
                     cc == MixDevice::UNDEFINED )
		{
		    // Everything unknown is handled as switch
		    cc = MixDevice::SWITCH;
		}
	    } // is ordinary mixer element (NOT an enum)

		MixDevice* md = new MixDevice( mixerIdx,
				   *volPlay,
					canRecord,
				   canMute,
				   snd_mixer_selem_id_get_name( sid ),
				   ct,
				   cc );

			m_mixDevices.append( md );


		if (!masterChosen && ct==MixDevice::VOLUME) {
		// Determine a nicer MasterVolume
		m_recommendedMaster = md;
		masterChosen = true;
		}

		if ( canCapture && !canRecord ) {
			MixDevice *mdCapture =
		    	new MixDevice( mixerIdx,
				   *volCapture,
					true,
				   canMute,
				   snd_mixer_selem_id_get_name( sid ),
				   ct,
				   cc );
			m_mixDevices.append( mdCapture );
		}

		if ( enumList.count() > 0 ) {
		  int maxEnumId= enumList.count();
		  QPtrList<QString>& enumValuesRef = md->enumValues(); // retrieve a ref
		  for (int i=0; i<maxEnumId; i++ ) {
		    // we have an enum. Lets set the names of the enum items in the MixDevice
		    // the enum names are assumed to be static!
		    enumValuesRef.append(enumList.at(i) );
		  }
		}
		//kdDebug(67100) << "ALSA create MDW, vol= " << *vol << endl;
		delete volPlay;
		delete volCapture;
	    } // virginOpen
	    else
	    {
			MixDevice* md;
			bool found = false;
    		for ( md = m_mixDevices.first(); md != 0; md = m_mixDevices.next() ) {
				if ( md->num() == mixerIdx ) {
					found = true;
					writeVolumeToHW( mixerIdx, md->getVolume() );
				}
			}
			if( !found )
			{
				return Mixer::ERR_INCOMPATIBLESET;
			}
	    } // !virginOpen
    } // for all elems

    /**************************************************************************************
    // If no devices are supported by this soundcard, return "NO Devices"
       It is VERY important to return THIS error code, so that the caller knows, that the
       the device exists.
       This is used for scanning for existing soundcard devices, see MixerToolBox::initMixer().
    ***************************************************************************************/
    if ( !validDevice )
    {
	return Mixer::ERR_NODEV;
    }

    // Copy the name of kmix mixer from card name
    // Real name of mixer is not too good
    m_mixerName = mixer_card_name;

    // return with success
    m_isOpen = true;

    /* setup for select on stdin and the mixer fd */
    if ((m_count = snd_mixer_poll_descriptors_count(_handle)) < 0) {
	kdDebug(67100) << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" <<  m_count << "\n";
	return Mixer::ERR_OPEN;
    }

    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 2\n";

    m_fds = (struct pollfd*)calloc(m_count, sizeof(struct pollfd));
    if (m_fds == NULL) {
	kdDebug(67100) << "Mixer_ALSA::poll() , calloc() = null" << "\n";
        return Mixer::ERR_OPEN;
    }

    m_fds->events = POLLIN;
    if ((err = snd_mixer_poll_descriptors(_handle, m_fds, m_count)) < 0) {
	kdDebug(67100) << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" <<  err << "\n";
        return Mixer::ERR_OPEN;
    }
    if (err != m_count) {
	kdDebug(67100) << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" << err << " m_count=" <<  m_count << "\n";
        return Mixer::ERR_OPEN;
    }

    return 0;
}

void Mixer_ALSA::prepareSignalling( Mixer *mixer )
{
    assert( !m_sns );

    m_sns = new QSocketNotifier*[m_count];
    for ( int i = 0; i < m_count; ++i )
    {
        kdDebug() << "socket " << i << endl;
        m_sns[i] = new QSocketNotifier(m_fds[i].fd, QSocketNotifier::Read);
        mixer->connect(m_sns[i], SIGNAL(activated(int)), mixer, SLOT(readSetFromHW()));
    }
}

void Mixer_ALSA::removeSignalling()
{
  if ( m_fds )
      free( m_fds );
  m_fds = 0;

  if ( m_sns )
  {
      for ( int i = 0; i < m_count; i++ )
          delete m_sns[i];
      delete [] m_sns;
      m_sns = 0;
  }
}

int
Mixer_ALSA::close()
{
  int ret=0;
  m_isOpen = false;
  if ( _handle != 0 )
  {
    //kdDebug(67100) << "IN  Mixer_ALSA::close()" << endl;
    snd_mixer_free ( _handle );
    if ( ( ret = snd_mixer_detach ( _handle, devName.latin1() ) ) < 0 )
    {
        kdDebug(67100) << "snd_mixer_detach err=" << snd_strerror(ret) << endl;
    }
    int ret2 = 0;
    if ( ( ret2 = snd_mixer_close ( _handle ) ) < 0 )
    {
        kdDebug(67100) << "snd_mixer_close err=" << snd_strerror(ret2) << endl;
	if ( ret == 0 ) ret = ret2; // no error before => use current error code
    }

    _handle = 0;
    //kdDebug(67100) << "OUT Mixer_ALSA::close()" << endl;

  }

  mixer_elem_list.clear();
  mixer_sid_list.clear();
  m_mixDevices.clear();

  removeSignalling();

  return ret;
}


snd_mixer_elem_t* Mixer_ALSA::getMixerElem(int devnum) {
	snd_mixer_elem_t* elem = 0;
	if ( ! m_isOpen ) return elem; // unplugging guard

	if ( int( mixer_sid_list.count() ) > devnum ) {
		snd_mixer_selem_id_t * sid = mixer_sid_list[ devnum ];
		// The next line (hopefully) only finds selem's, not elem's.
		elem = snd_mixer_find_selem(_handle, sid);

		if ( elem == 0 ) {
			// !! Check, whether the warning should be omitted. Probably
			//    Route controls are non-simple elements.
			kdDebug(67100) << "Error finding mixer element " << devnum << endl;
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

bool Mixer_ALSA::prepareUpdateFromHW()
{
    if ( !m_fds || !m_isOpen )
	return false;

    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 1\n";

    // Poll on fds with 10ms timeout
    // Hint: alsamixer has an infinite timeout, but we cannot do this because we would block
    // the X11 event handling (Qt event loop) with this.
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 3\n";
    int finished = poll(m_fds, m_count, 10);
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 4\n";

    bool updated = false;
    if (finished > 0) {
        //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 5\n";

        unsigned short revents;

        if (snd_mixer_poll_descriptors_revents(_handle, m_fds, m_count, &revents) >= 0) {
            //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 6\n";

	    if (revents & POLLNVAL) {
		/* Bug 127294 shows, that we receieve POLLNVAL when the user
		 unplugs an USB soundcard. Lets close the card. */
		kdDebug(67100) << "Mixer_ALSA::poll() , Error: poll() returns POLLNVAL\n";
		close();  // Card was unplugged (unplug, driver unloaded)
		return false;
	    }
	    if (revents & POLLERR) {
		kdDebug(67100) << "Mixer_ALSA::poll() , Error: poll() returns POLLERR\n";
		return false;
	    }
	    if (revents & POLLIN) {
                //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 7\n";

		snd_mixer_handle_events(_handle);
                updated = true;
	    }
	}

    }
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() " << updated << endl;;
    return updated;
}

bool
Mixer_ALSA::isRecsrcHW( int devnum )
{
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
                if ( ret != 0 ) {
                        kdDebug(67100) << "snd_mixer_selem_get_capture_switch() failed 1\n";
                }

		if (snd_mixer_selem_has_capture_switch_joined( elem ) ) {
			isCurrentlyRecSrc = (swLeft != 0);
#ifdef ALSA_SWITCH_DEBUG
			kdDebug(67100) << "Mixer_ALSA::isRecsrcHW() has_switch joined: #" << devnum << " >>> " << swLeft << " : " << isCurrentlyRecSrc << endl;
#endif
		}
		else {
			int swRight;
			snd_mixer_selem_get_capture_switch( elem, SND_MIXER_SCHN_FRONT_RIGHT, &swRight );
			isCurrentlyRecSrc = ( (swLeft != 0) || (swRight != 0) );
#ifdef ALSA_SWITCH_DEBUG
			kdDebug(67100) << "Mixer_ALSA::isRecsrcHW() has_switch non-joined, state " << isCurrentlyRecSrc << endl;
#endif
		}
	}
	else {
		// Has no on-off switch
		if ( snd_mixer_selem_has_capture_volume( elem ) ) {
			// Has a volume, but has no OnOffSwitch => We assume that this is a fixed record source (always on). (esken)
			isCurrentlyRecSrc = true;
#ifdef ALSA_SWITCH_DEBUG
			kdDebug(67100) << "Mixer_ALSA::isRecsrcHW() has_no_switch, state " << isCurrentlyRecSrc << endl;
#endif
		}
	}

	return isCurrentlyRecSrc;
}

bool
Mixer_ALSA::setRecsrcHW( int devnum, bool on )
{
	int sw = 0;
	if (on)
		sw = !sw;

	snd_mixer_elem_t *elem = getMixerElem( devnum );
	if ( !elem )
	{
		return 0;
	}

	if (snd_mixer_selem_has_capture_switch_joined( elem ) )
	{
		int before, after;
		int ret = snd_mixer_selem_get_capture_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, &before );
		if ( ret != 0 ) {
			kdDebug(67100) << "snd_mixer_selem_get_capture_switch() failed 1\n";
		}

		ret = snd_mixer_selem_set_capture_switch_all( elem, sw );
                if ( ret != 0 ) {
                        kdDebug(67100) << "snd_mixer_selem_set_capture_switch_all() failed 2: errno=" << ret << "\n";
                }
		ret = snd_mixer_selem_get_capture_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, &after );
                if ( ret != 0 ) {
                        kdDebug(67100) << "snd_mixer_selem_get_capture_switch() failed 3: errno=" << ret << "\n";
                }

#ifdef ALSA_SWITCH_DEBUG
		kdDebug(67100) << "Mixer_ALSA::setRecsrcHW(" << devnum <<  "," << on << ")joined. Before=" << before << " Set=" << sw << " After=" << after <<"\n";
#endif

	}
	else
	{
#ifdef ALSA_SWITCH_DEBUG
			kdDebug(67100) << "Mixer_ALSA::setRecsrcHW LEFT\n";
#endif
			snd_mixer_selem_set_capture_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, sw );
#ifdef ALSA_SWITCH_DEBUG
			kdDebug(67100) << "Mixer_ALSA::setRecsrcHW RIGHT\n";
#endif
			snd_mixer_selem_set_capture_switch(elem, SND_MIXER_SCHN_FRONT_RIGHT, sw);
	}

#ifdef ALSA_SWITCH_DEBUG
	kdDebug(67100) << "EXIT Mixer_ALSA::setRecsrcHW(" << devnum << "," << on <<  ")\n";
#endif
	return false; // we should always return false, so that other devnum's get updated
	// The ALSA polling has been implemented some time ago. So it should be safe to
	// return "true" here.
	// The other devnum's Rec-Sources won't get update by KMix code, but ALSA will send
	// us an event, if neccesary. But OTOH it is possibly better not to trust alsalib fully,
        // because the old code is working also well (just takes more processing time).
	// return true;
}

/**
 * Sets the ID of the currently selected Enum entry.
 * Warning: ALSA supports to have different enums selected on each channel
 *          of the SAME snd_mixer_elem_t. KMix does NOT support that and
 *          always sets both channels (0 and 1).
 */
void Mixer_ALSA::setEnumIdHW(int mixerIdx, unsigned int idx) {
	//kdDebug(67100) << "Mixer_ALSA::setEnumIdHW(" << mixerIdx << ", idx=" << idx << ") 1\n";
        snd_mixer_elem_t *elem = getMixerElem( mixerIdx );
        if ( elem==0 || ( !snd_mixer_selem_is_enumerated(elem)) )
        {
                return;
        }

	//kdDebug(67100) << "Mixer_ALSA::setEnumIdHW(" << mixerIdx << ", idx=" << idx << ") 2\n";
	int ret = snd_mixer_selem_set_enum_item(elem,SND_MIXER_SCHN_FRONT_LEFT,idx);
        if (ret < 0) {
           kdError(67100) << "Mixer_ALSA::setEnumIdHW(" << mixerIdx << "), errno=" << ret << "\n";
        }
	snd_mixer_selem_set_enum_item(elem,SND_MIXER_SCHN_FRONT_RIGHT,idx);
	// we don't care about possible error codes on channel 1
        return;
}

/**
 * Return the ID of the currently selected Enum entry.
 * Warning: ALSA supports to have different enums selected on each channel
 *          of the SAME snd_mixer_elem_t. KMix does NOT support that and
 *          always shows the value of the first channel.
 */
unsigned int Mixer_ALSA::enumIdHW(int mixerIdx) {
	snd_mixer_elem_t *elem = getMixerElem( mixerIdx );
        if ( elem==0 || ( !snd_mixer_selem_is_enumerated(elem)) )
        {
                return 0;
        }

	unsigned int idx = 0;
	int ret = snd_mixer_selem_get_enum_item(elem,SND_MIXER_SCHN_FRONT_LEFT,&idx);
	//kdDebug(67100) << "Mixer_ALSA::enumIdHW(" << mixerIdx << ") idx=" << idx << "\n";
	if (ret < 0) {
	   idx = 0;
	   kdError(67100) << "Mixer_ALSA::enumIdHW(" << mixerIdx << "), errno=" << ret << "\n";
	}
	return idx;
}


int
Mixer_ALSA::readVolumeFromHW( int mixerIdx, Volume &volume )
{
	int elem_sw;
	long left, right;

	snd_mixer_elem_t *elem = getMixerElem( mixerIdx );
	if ( !elem )
	{
		return 0;
	}


	// *** READ PLAYBACK VOLUMES *************
	if ( snd_mixer_selem_has_playback_volume( elem ) && !volume.isCapture() )
	{
		int ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_FRONT_LEFT, &left );
                if ( ret != 0 ) kdDebug(67100) << "readVolumeFromHW(" << mixerIdx << ") [has_playback_volume,R] failed, errno=" << ret << endl;
		if ( snd_mixer_selem_is_playback_mono ( elem )) {
                    volume.setVolume( Volume::LEFT , left );
                    volume.setVolume( Volume::RIGHT, left );
                }
                else {
                    int ret = snd_mixer_selem_get_playback_volume( elem, SND_MIXER_SCHN_FRONT_RIGHT, &right );
                    if ( ret != 0 ) kdDebug(67100) << "readVolumeFromHW(" << mixerIdx << ") [has_playback_volume,R] failed, errno=" << ret << endl;
                    volume.setVolume( Volume::LEFT , left );
                    volume.setVolume( Volume::RIGHT, right );
                }
	}
	else
	if ( snd_mixer_selem_has_capture_volume ( elem ) && volume.isCapture() )
	{
            int ret = snd_mixer_selem_get_capture_volume ( elem, SND_MIXER_SCHN_FRONT_LEFT, &left );
            if ( ret != 0 ) kdDebug(67100) << "readVolumeFromHW(" << mixerIdx << ") [get_capture_volume,L] failed, errno=" << ret << endl;
	    if ( snd_mixer_selem_is_capture_mono  ( elem )) {
		volume.setVolume( Volume::LEFT , left );
		volume.setVolume( Volume::RIGHT, left );
	    }
	    else
	    {
		int ret = snd_mixer_selem_get_capture_volume( elem, SND_MIXER_SCHN_FRONT_RIGHT, &right );
		if ( ret != 0 ) kdDebug(67100) << "readVolumeFromHW(" << mixerIdx << ") [has_capture_volume,R] failed, errno=" << ret << endl;
		volume.setVolume( Volume::LEFT , left );
		volume.setVolume( Volume::RIGHT, right );
	    }
	}

        //kdDebug() << "snd_mixer_selem_has_playback_volume " << mixerIdx << " " << snd_mixer_selem_has_playback_switch( elem ) << endl;
	if ( snd_mixer_selem_has_playback_switch( elem ) )
	{
	    snd_mixer_selem_get_playback_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, &elem_sw );
            volume.setMuted( elem_sw == 0 );
	}

	return 0;
}

int
Mixer_ALSA::writeVolumeToHW( int devnum, Volume& volume )
{
	int left, right;

	snd_mixer_elem_t *elem = getMixerElem( devnum );
	if ( !elem )
	{
		return 0;
	}

	// --- VOLUME  - WE HAVE JUST ONE TYPE OF VOLUME A TIME,
	// CAPTURE OR PLAYBACK, SO IT"S JUST USE VOLUME ------------
	left = volume[ Volume::LEFT ];
	right = volume[ Volume::RIGHT ];

	if (snd_mixer_selem_has_playback_volume( elem ) && !volume.isCapture() ) {
		snd_mixer_selem_set_playback_volume ( elem, SND_MIXER_SCHN_FRONT_LEFT, left );
		if ( ! snd_mixer_selem_is_playback_mono ( elem ) )
			snd_mixer_selem_set_playback_volume ( elem, SND_MIXER_SCHN_FRONT_RIGHT, right );
	}
	else if ( snd_mixer_selem_has_capture_volume( elem ) && volume.isCapture() ) {
		snd_mixer_selem_set_capture_volume ( elem, SND_MIXER_SCHN_FRONT_LEFT, left );
		if ( ! snd_mixer_selem_is_playback_mono ( elem ) )
			snd_mixer_selem_set_capture_volume ( elem, SND_MIXER_SCHN_FRONT_RIGHT, right );
	}

	if ( snd_mixer_selem_has_playback_switch( elem ) )
	{
		int sw = 0;
		if (! volume.isMuted())
			sw = !sw;
		snd_mixer_selem_set_playback_switch_all(elem, sw);
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


