/*
 *              KMix -- KDE's full featured mini mixer
 *              Alsa 0.9x - Based on original alsamixer code 
 *              from alsa-project ( www/alsa-project.org )
 *
 *
 * Copyright (C) 2002 Helio Chissini de Castro <helio@conectiva.com.br>
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// STD Headers
#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>

extern "C"
{
	#include <alsa/asoundlib.h>
}

// KDE Headers
#include <klocale.h>

// Local Headers
#include "mixer_alsa.h"
#include "volume.h"

Mixer* 
ALSA_getMixer( int device, int card )
{
	Mixer *l_mixer;
	l_mixer = new Mixer_ALSA( device, card );
	l_mixer->setupMixer();
	return l_mixer;
}

Mixer* 
ALSA_getMixerSet( MixSet set, int device, int card )
{
	Mixer *l_mixer;
	l_mixer = new Mixer_ALSA( device, card );
	l_mixer->setupMixer( set );
	return l_mixer;
}

Mixer_ALSA::Mixer_ALSA( int device, int card ) : 
	Mixer( device, card ), handle(0)
{
}

Mixer_ALSA::~Mixer_ALSA()
{
}

int
Mixer_ALSA::identify( snd_mixer_selem_id_t *sid )
{
	QString name = snd_mixer_selem_id_get_name( sid );

	if ( name == "Master" )
	{
		m_masterDevice = snd_mixer_selem_id_get_index( sid );
		return MixDevice::VOLUME;
	}
	if ( name == "Master Mono" ) return MixDevice::VOLUME;
	if ( name == "Headphone" ) return MixDevice::EXTERNAL;
	if ( name == "Bass" ) return MixDevice::BASS;
	if ( name == "Treble" ) return MixDevice::TREBLE;
	if ( name == "CD" ) return MixDevice::CD;
	if ( name == "Video" ) return MixDevice::VIDEO;
	if ( name == "PCM" || name == "Wave" || name == "Line" )	return MixDevice::AUDIO;
	if ( name == "Surround" ) return MixDevice::SURROUND;
	
	return MixDevice::EXTERNAL;
}

int 
Mixer_ALSA::openMixer()
{
	kdDebug() << "Mixer_ALSA::openMixer - Enter" << endl;
	
	bool virginOpen = m_mixDevices.isEmpty();
	bool validDevice = false;
	int err;

	snd_ctl_t *ctl_handle;
	snd_ctl_card_info_t *hw_info;
	snd_ctl_card_info_alloca(&hw_info);

	snd_mixer_elem_t *elem;
   snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca( &sid );

	// Card information
	if( card_id.isEmpty() )
	{
		card_id = "default";
	}

	if ( ( err = snd_ctl_open ( &ctl_handle, card_id.latin1(), 0 ) ) < 0 )
	{
		errormsg( Mixer::ERR_OPEN );
		return false;
	}

	if ( ( err = snd_ctl_card_info ( ctl_handle, hw_info ) ) < 0 )
	{
		errormsg( Mixer::ERR_READ );
		snd_ctl_close( ctl_handle );
		return false;
	}

	// Device and mixer names
	mixer_card_name =  snd_ctl_card_info_get_name( hw_info );
	mixer_device_name = snd_ctl_card_info_get_mixername( hw_info );

	snd_ctl_close( ctl_handle );
	
	// release mixer before (re-)opening
	release();
		
	/* open mixer device */
	if ( ( err = snd_mixer_open ( &handle, 0 ) ) < 0 )
	{
		errormsg( Mixer::ERR_OPEN );
	}
	
	if ( ( err = snd_mixer_attach ( handle, card_id.latin1() ) ) < 0 )
	{
		errormsg( Mixer::ERR_PERM );
	}
	
	if ( ( err = snd_mixer_selem_register ( handle, NULL, NULL ) ) < 0 )
	{
		errormsg( Mixer::ERR_READ );
	}

	if ( ( err = snd_mixer_load ( handle ) ) < 0 )
	{
		errormsg( Mixer::ERR_READ );
		releaseMixer();
		return 1;
	}


	// default mixers?
	if( m_cardnum == -1 )
	{
		m_cardnum = 0;
	}

	if( m_devnum == -1 )
	{
		m_devnum = 0;
	}

	for ( elem = snd_mixer_first_elem( handle ); elem; elem = snd_mixer_elem_next( elem ) ) 
	{
		snd_mixer_selem_get_id( elem, sid );
		
		if ( !snd_mixer_selem_is_active( elem ) )
		{
			continue;
		}

		bool isMono = false;
		bool canRecord = false;
		long maxVolume, minVolume;
		int mixerIdx;
		validDevice = true;
		
		if ( snd_mixer_selem_is_playback_mono( elem ) )
		{
			isMono = true;
		}
		if ( snd_mixer_selem_has_capture_volume( elem ) )
		{
			canRecord = true;
		}
		
		snd_mixer_selem_get_playback_volume_range( elem, &minVolume, &maxVolume );
		mixerIdx = snd_mixer_selem_id_get_index( sid );

		// New mix device
		MixDevice::ChannelType ct = (MixDevice::ChannelType)identify( sid );
		
		if( virginOpen )
		{
			Volume vol( SND_MIXER_SCHN_LAST, (int)maxVolume );
			readVolumeHW( elem, vol );			
			m_mixDevices.append(	new MixDevice( mixerIdx, vol, canRecord, snd_mixer_selem_id_get_name( sid ), ct) );
		} 
		else
		{
			MixDevice* md = m_mixDevices.at( mixerIdx );
			if( !md ) 
			{
				return ERR_INCOMPATIBLESET;
			}
			writeVolumeToHW( mixerIdx, md->getVolume() );
		}
		
	}	

	//return error for invalid devices	
	if ( !validDevice ) 
	{
		return Mixer::ERR_NODEV;
	}

	// Copy the name of kmix mixer from card name
	// Real name of mixer is not too good
	m_mixerName = mixer_card_name; 
	
	// return with success
	m_isOpen = true;

	kdDebug() << "Mixer_ALSA::openMixer - Out" << endl;
	
	return 0;
}


int 
Mixer_ALSA::releaseMixer()
{
	int ret = snd_mixer_close( handle );
	return ret;
}

bool 
Mixer_ALSA::isRecsrcHW( int devnum )
{
	devnum++;
	//gid = &groups.pgroups[devnum];
	
	// get device caps to check for capture
	//snd_mixer_group_t group;
	//bzero(&group, sizeof(group));
	//group.gid = *gid;
	//if ( snd_mixer_group_read(handle, &group)<0 ) return Mixer::ERR_READ;
	
	//return group.capture && SND_MIXER_CHN_MASK_FRONT_LEFT;
	return false;
}

bool 
Mixer_ALSA::setRecsrcHW( int devnum, bool on )
{
	devnum++;
	on = false;
	//snd_mixer_open( &handle, m_cardnum );
	//gid = &groups.pgroups[devnum];
		
	// get current device caps
	//snd_mixer_group_t group;
	//bzero(&group, sizeof(group));
	//group.gid = *gid;
	//if ( snd_mixer_group_read(handle, &group)<0 ) return true;
	
	// set capture flag
	//group.capture = on ? group.capture | SND_MIXER_CHN_MASK_FRONT_LEFT :
		//group.capture & ~SND_MIXER_CHN_MASK_FRONT_LEFT;
	//if ( numChannels(group.channels)>1 ) group.capture = on ? ~0 : 0;

   // write caps back
   //if ( snd_mixer_group_write(handle, &group)<0 ) return true;
	
	return false;
}

void 
Mixer_ALSA::readVolumeHW( snd_mixer_elem_t *elem, Volume &volume )
{
   int volChannel = 0;
	long leftvol, rightvol;
	long vol;
	snd_mixer_selem_channel_id_t chn;
	
	for (int channel = 0; channel < SND_MIXER_SCHN_LAST; channel++)
	{

		if ( ( ( snd_mixer_selem_has_playback_volume( elem ) ) || 
				( snd_mixer_selem_has_capture_volume( elem ) ) ) &	( 1 << channel ) )
		{
			if( snd_mixer_selem_has_playback_volume( elem ) )
			{
				//snd_mixer_selem_get_playback_volume(elem, chn, &vol);
			}
			else
			{
				//snd_mixer_selem_get_capture_volume(elem, chn, &vol);
			}
			
			//volume.setVolume( volChannel, (int)&vol );
			
			//if ( volChannel==0 ) 
			//	leftvol = vol;
			
			//if ( volChannel==1 )
			//{
			//	rightvol = vol;
			//	if( leftvol != rightvol )
			//	{
			//		m_balance = ( leftvol > rightvol ) ?
			//			(-(leftvol-rightvol)*100/leftvol) :
			//			(+(rightvol-leftvol)*100/rightvol);
			//	} 
			//	else
			//		m_balance = 0;
			//}
			volChannel++;
		}
	}
}

int 
Mixer_ALSA::readVolumeFromHW( int devnum, Volume &volume )
{
	// Avoid warnings
	Volume data = volume;
	int teste = devnum;
	teste++;
	
	return 0;
}


int 
Mixer_ALSA::writeVolumeToHW( int devnum, Volume volume )
{
	// Avoid warnings
	Volume data = volume;
	int teste = devnum;
	teste++;

	kdDebug() << "MixerALSA::Write volume call..." << endl;
	//snd_mixer_open( &handle, m_cardnum );
	//gid = &groups.pgroups[devnum];
	
	// get current group info
   //snd_mixer_group_t group;
   //bzero(&group, sizeof(group));
   //group.gid = *gid;
   //if ( snd_mixer_group_read(handle, &group)<0 ) return Mixer::ERR_READ;

   // update muting flags
   //group.mute = volume.isMuted() ? ~0 : 0;

   // write volumes channel for channel
   //int volChannel = 0;
   //for ( int channel=0; channel<32; channel++ )
   //{
      //if ( group.channels & (1 << channel) )
      //{
         //group.volume.values[channel] = volume[volChannel];
         //volChannel++;
      //}
   //}

   // write volumes back
   //if ( snd_mixer_group_write(handle, &group)<0 ) return Mixer::ERR_WRITE;

   return 0;
}

