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

extern "C"
{
	#include <alsa/asoundlib.h>
}

// KDE Headers
#include <klocale.h>
#include <kdebug.h>

// Local Headers
#include "mixer_alsa.h"
#include "volume.h"
// #define if you want MUCH debugging output
//#define ALSA_SWITCH_DEBUG
//#define KMIX_ALSA_VOLUME_DEBUG

// @todo Add file descriptor based notifying for seeing changes

Mixer*
ALSA_getMixer( int device, int card )
{
	Mixer *l_mixer;
	l_mixer = new Mixer_ALSA( device, card );
	return l_mixer;
}

Mixer_ALSA::Mixer_ALSA( int device, int card ) :
	Mixer( device, card ), _handle(0)
{
        _initialUpdate = true;
}

Mixer_ALSA::~Mixer_ALSA()
{
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
Mixer_ALSA::openMixer()
{
    bool virginOpen = m_mixDevices.isEmpty();
    bool validDevice = false;
    bool masterChosen = false;
    int err;

    release();

    snd_ctl_t *ctl_handle;
    snd_ctl_card_info_t *hw_info;
    snd_ctl_card_info_alloca(&hw_info);

    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca( &sid );

    // Card information
    QString devName;
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
    if ( ( err = snd_mixer_open ( &_handle, 0 ) ) < 0 )
    {
	kdDebug(67100) << probeMessage << "not found: snd_mixer_open err=" << snd_strerror(err) << endl;
	//errormsg( Mixer::ERR_NODEV );
	return Mixer::ERR_NODEV; // if we cannot open the mixer, we have no devices
    }

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
	releaseMixer();
	return Mixer::ERR_READ;
    }

    kdDebug(67100) << probeMessage << "found" << endl;	

    unsigned int mixerIdx = 0;
    for ( elem = snd_mixer_first_elem( _handle ); elem; elem = snd_mixer_elem_next( elem ), mixerIdx++ )
    {
	// If element is not active, just skip
	if ( ! snd_mixer_selem_is_active ( elem ) ) {
	    continue;
	}


	sid = (snd_mixer_selem_id_t*)malloc(snd_mixer_selem_id_sizeof());  // I believe *we* must malloc it for ourself
	snd_mixer_selem_get_id( elem, sid );

	bool canRecord = false;
	bool canMute = false;
	long maxVolumePlay= 0, minVolumePlay= 0;
	long maxVolumeRec = 0, minVolumeRec = 0;
	validDevice = true;

	snd_mixer_selem_get_playback_volume_range( elem, &minVolumePlay, &maxVolumePlay );
	snd_mixer_selem_get_capture_volume_range( elem, &minVolumeRec , &maxVolumeRec  );
	// New mix device
	MixDevice::ChannelType ct = (MixDevice::ChannelType)identify( sid );
        if (!masterChosen && ct==MixDevice::VOLUME) {
           // Determine a nicer MasterVolume
	   m_masterDevice = mixerIdx;
           masterChosen = true;
        }

	if( virginOpen )
	{
	    MixDevice::DeviceCategory cc = MixDevice::UNDEFINED;
		
		//kdDebug() << "--- Loop: name=" << snd_mixer_selem_id_get_name( sid ) << " , mixerIdx=" << mixerIdx << "------------" << endl;

	    Volume* vol = 0;
	    QPtrList<QString> enumList;
	    if ( snd_mixer_selem_is_enumerated(elem) ) {
		cc = MixDevice::ENUM;
		vol = new Volume(); // Dummy, unused
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
		}
		else if ( snd_mixer_selem_has_capture_volume(elem) ) {
			//kdDebug(67100) << "has_capture_volume()" << endl;
			chnTmp = snd_mixer_selem_is_capture_mono( elem )
				? Volume::MLEFT : (Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT );
			chn = (Volume::ChannelMask) (chn | chnTmp);
			cc = MixDevice::SLIDER;
			// We can have Playback OR Capture. Not both at same time
			// It's not best coding ever, anyway
			snd_mixer_selem_get_capture_volume_range( elem, &minVolumePlay, &maxVolumePlay );
		}
		
		/* Create Volume object. If there is no volume on this device,
		 * it will be created with maxVolume == 0 && minVolume == 0 */
		vol = new Volume( chn, maxVolumePlay, minVolumePlay, maxVolumeRec, minVolumeRec );
		//mixer_elem_list.append( elem );
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

		MixDevice* mdw =
		    new MixDevice( mixerIdx,
				   *vol,
					canRecord,
				   canMute,
				   snd_mixer_selem_id_get_name( sid ),
				   ct,
				   cc );
		if ( enumList.count() > 0 ) {
		  int maxEnumId= enumList.count();
		  QPtrList<QString>& enumValuesRef = mdw->enumValues(); // retrieve a ref
		  for (int i=0; i<maxEnumId; i++ ) {
		    // we have an enum. Lets set the names of the enum items in the MixDevice
		    // the enum names are assumed to be static!
		    enumValuesRef.append(enumList.at(i) );
		  }
		}
		m_mixDevices.append( mdw );
		//kdDebug(67100) << "ALSA create MDW, vol= " << *vol << endl;
		delete vol;
	    } // virginOpen
	    else
	    {
		MixDevice* md = m_mixDevices.at( mixerIdx );
		if( !md )
		{
		    return ERR_INCOMPATIBLESET;
		}
		writeVolumeToHW( mixerIdx, md->getVolume() );
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

    return 0;
}


int
Mixer_ALSA::releaseMixer()
{
	int ret = snd_mixer_close( _handle );
	return ret;
}


snd_mixer_elem_t* Mixer_ALSA::getMixerElem(int devnum) {
	snd_mixer_elem_t* elem = 0;
	if ( int( mixer_sid_list.count() ) > devnum ) {
		snd_mixer_selem_id_t * sid = mixer_sid_list[ devnum ];
		elem = snd_mixer_find_selem(_handle, sid);

		if ( elem == 0 ) {
			kdDebug(67100) << "Error finding mixer element " << devnum << endl;
		}
	}
	return elem;
//	return mixer_elem_list[ devnum ];
}

bool Mixer_ALSA::prepareUpdate() {
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 1\n";
    if ( _initialUpdate ) {
        // make sure the very first call to prepareUpdate() returns "true". Otherwise kmix will
        // show wrong values until a mixer change happens.
        _initialUpdate = false;
        return true;
    }
    bool updated = false;
    struct pollfd  *fds;
    unsigned short revents;
    int count, err;

/* setup for select on stdin and the mixer fd */
    if ((count = snd_mixer_poll_descriptors_count(_handle)) < 0) {
	kdDebug(67100) << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" <<  count << "\n";
	return false;
    }

    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 2\n";
    
    fds = (struct pollfd*)calloc(count, sizeof(struct pollfd));
    if (fds == NULL) {
	kdDebug(67100) << "Mixer_ALSA::poll() , calloc() = null" << "\n";
	return false;
    }

    fds->events = POLLIN;
    if ((err = snd_mixer_poll_descriptors(_handle, fds, count)) < 0) {
	kdDebug(67100) << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" <<  err << "\n";
        free(fds);
	return false;
    }
    if (err != count) {
	kdDebug(67100) << "Mixer_ALSA::poll() , snd_mixer_poll_descriptors_count() err=" << err << " count=" <<  count << "\n";
        free(fds);
	return false;
    }

    // Poll on fds with 10ms timeout
    // Hint: alsamixer has an infinite timeout, but we cannot do this because we would block
    // the X11 event handling (Qt event loop) with this.
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 3\n";
    int finished = poll(fds, count, 10);
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 4\n";

    if (finished > 0) {
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 5\n";

	if (snd_mixer_poll_descriptors_revents(_handle, fds, count, &revents) >= 0) {
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 6\n";


	    if (revents & POLLNVAL) {
		kdDebug(67100) << "Mixer_ALSA::poll() , Error: poll() returns POLLNVAL\n";
                free(fds);
		return false;
	    }
	    if (revents & POLLERR) {
		kdDebug(67100) << "Mixer_ALSA::poll() , Error: poll() returns POLLERR\n";
                free(fds);
		return false;
	    }
	    if (revents & POLLIN) {
    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 7\n";

		snd_mixer_handle_events(_handle);
                updated = true;
	    }
	}
    }

    //kdDebug(67100) << "Mixer_ALSA::prepareUpdate() 8\n";
    free(fds);
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
	if ( snd_mixer_selem_has_playback_volume( elem ) )
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
	if ( snd_mixer_selem_has_capture_volume ( elem ) )
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

	if ( snd_mixer_selem_has_playback_switch( elem ) )
	{
	    snd_mixer_selem_get_playback_switch( elem, SND_MIXER_SCHN_FRONT_LEFT, &elem_sw );
	    if( elem_sw == 0 )
		volume.setMuted(true);
	    else
		volume.setMuted(false);
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
	
	if (snd_mixer_selem_has_playback_volume( elem ) ) {
		snd_mixer_selem_set_playback_volume ( elem, SND_MIXER_SCHN_FRONT_LEFT, left );
		if ( ! snd_mixer_selem_is_playback_mono ( elem ) )
			snd_mixer_selem_set_playback_volume ( elem, SND_MIXER_SCHN_FRONT_RIGHT, right );
	}
	else if ( snd_mixer_selem_has_capture_volume( elem ) ) {
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
		case ERR_PERM:
			l_s_errmsg = i18n("You do not have permission to access the alsa mixer device.\n" \
					"Please verify if all alsa devices are properly created.");
      break;
		case ERR_OPEN:
			l_s_errmsg = i18n("Alsa mixer cannot be found.\n" \
					"Please check that the soundcard is installed and the\n" \
					"soundcard driver is loaded.\n" );
			break;
		default:
			l_s_errmsg = Mixer::errorText( mixer_error );
	}
	return l_s_errmsg;
}


QString
ALSA_getDriverName()
{
	return "ALSA";
}


