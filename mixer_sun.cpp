/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken <esken@kde.org>
 *                            2000 Brian Hanson <bhanson@hotmail.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/audioio.h>
#include <errno.h>
#include <unistd.h>

#include "mixer_sun.h"


//======================================================================
// CONSTANT/ENUM DEFINITIONS
//======================================================================

//
// Mixer Device Numbers
//
// Note: We can't just use the Sun port #defines because :
// 1) Some logical devices don't correspond to ports (master&recmon)
// 2) The play and record port definitions reuse the same values
//
enum MixerDevs
{
   MIXERDEV_MASTER_VOLUME,
   MIXERDEV_INTERNAL_SPEAKER,
   MIXERDEV_HEADPHONE,
   MIXERDEV_LINE_OUT,
   MIXERDEV_RECORD_MONITOR,
   MIXERDEV_MICROPHONE,
   MIXERDEV_LINE_IN,
   MIXERDEV_CD,
   // Insert new devices before this marker
   MIXERDEV_END_MARKER
};
const int numDevs = MIXERDEV_END_MARKER;

//
// Device name strings
//
const char* MixerDevNames[] =
{
   I18N_NOOP("Master Volume"),
   I18N_NOOP("Internal Speaker"),
   I18N_NOOP("Headphone"),
   I18N_NOOP("Line Out"),
   I18N_NOOP("Record Monitor"),
   I18N_NOOP("Microphone"),
   I18N_NOOP("Line In"),
   I18N_NOOP("CD")
};

//
// Channel types (this specifies which icon to display)
//
const MixDevice::ChannelType MixerChannelTypes[] =
{
   MixDevice::VOLUME,       // MASTER_VOLUME
   MixDevice::AUDIO,        // INTERNAL_SPEAKER
   MixDevice::EXTERNAL,     // HEADPHONE (we really need an icon for this)
   MixDevice::EXTERNAL,     // LINE_OUT
   MixDevice::RECMONITOR,   // RECORD_MONITOR
   MixDevice::MICROPHONE,   // MICROPHONE
   MixDevice::EXTERNAL,     // LINE_IN
   MixDevice::CD            // CD
};

//
// Mapping from device numbers to Sun port mask values
//
const uint_t MixerSunPortMasks[] =
{
   0,                  // MASTER_VOLUME - no associated port
   AUDIO_SPEAKER,
   AUDIO_HEADPHONE,
   AUDIO_LINE_OUT,
   0,                  // RECORD_MONITOR - no associated port
   AUDIO_MICROPHONE,
   AUDIO_LINE_IN,
   AUDIO_CD
};


//======================================================================
// FUNCTION/METHOD DEFINITIONS
//======================================================================


//======================================================================
// FUNCTION    : SUN_getMixer
// DESCRIPTION : Creates and returns a new mixer object.
//======================================================================
Mixer* SUN_getMixer( int devnum, int SetNum )
{
   Mixer *l_mixer;
   l_mixer = new Mixer_SUN( devnum, SetNum );
   l_mixer->setupMixer();
   return l_mixer;
}

//======================================================================
// FUNCTION    : SUN_getMixerSet
// DESCRIPTION : Creates and returns a new mixer object.
//======================================================================
Mixer* SUN_getMixerSet( MixSet set, int device, int card )
{
   Mixer *l_mixer;
   l_mixer = new Mixer_SUN( device, card );
   l_mixer->setupMixer( set );
   return l_mixer;
}

//======================================================================
// FUNCTION    : Mixer::Mixer
// DESCRIPTION : Class constructor.
//======================================================================
Mixer_SUN::Mixer_SUN(int devnum, int card) : Mixer(devnum, card)
{
   if ( devnum == -1 )
      m_devnum = 0;
   if ( card == -1 )
      m_cardnum = 0;
}

//======================================================================
// FUNCTION    : Mixer::openMixer
// DESCRIPTION : Initialize the mixer and open the hardware driver.
//======================================================================
int Mixer_SUN::openMixer()
{
   //
   // We don't support multiple cards or devices
   //
   if ( m_cardnum != 0 )
      return Mixer::ERR_OPEN;
   if ( m_devnum !=0 )
      return Mixer::ERR_OPEN;

   //
   // Release mixer before (re-)opening
   //
   release();

   //
   // Open the mixer hardware driver
   //
   if ( ( fd = open( "/dev/audioctl", O_RDWR ) ) < 0 )
   {
      if ( errno == EACCES )
         return Mixer::ERR_PERM;
      else
         return Mixer::ERR_OPEN;
   }
   else
   {
      //
      // Mixer is open. Now define all of the mix devices.
      //

      if( m_mixDevices.isEmpty() )
      {
         for ( int idx = 0; idx < numDevs; idx++ )
         {
            Volume vol( 2, AUDIO_MAX_GAIN );
            readVolumeFromHW( idx, vol );
            MixDevice* md = new MixDevice( idx, vol, 0,
               QString(MixerDevNames[idx]), MixerChannelTypes[idx]);
            md->setRecsrc( isRecsrcHW( idx ) );
            m_mixDevices.append( md );
         }
     }
     else
     {
        for( unsigned int idx = 0; idx < m_mixDevices.count(); idx++ )
        {
           MixDevice* md = m_mixDevices.at( idx );
           if( !md )
              return ERR_INCOMPATIBLESET;
           writeVolumeToHW( idx, md->getVolume() );
        }
     }

     m_mixerName = "SUN Audio Mixer";
     m_isOpen = true;

     return 0;
   }
}

//======================================================================
// FUNCTION    : Mixer::releaseMixer
// DESCRIPTION : Close the hardware driver.
//======================================================================
int Mixer_SUN::releaseMixer()
{
   int l_i_ret = ::close( fd );
   return l_i_ret;
}

//======================================================================
// FUNCTION    : Mixer::errorText
// DESCRIPTION : Convert an error code enum to a text string.
//======================================================================
QString Mixer_SUN::errorText( int mixer_error )
{
   QString errmsg;
   switch (mixer_error)
   {
      case ERR_PERM:
         errmsg = i18n(
           "kmix: You do not have permission to access the mixer device.\n"
           "Ask your system administrator to fix /dev/audioctl to allow access."
         );
         break;
      default:
         errmsg = Mixer::errorText( mixer_error );
   }
   return errmsg;
}


//======================================================================
// FUNCTION    : Mixer::readVolumeFrmoHW
// DESCRIPTION : Read the audio information from the driver.
//======================================================================
int Mixer_SUN::readVolumeFromHW( int devnum, Volume& volume )
{
   audio_info_t audioinfo;
   uint_t devMask = MixerSunPortMasks[devnum];

   //
   // Read the current audio information from the driver
   //
   if ( ioctl( fd, AUDIO_GETINFO, &audioinfo ) < 0 )
   {
      return( Mixer::ERR_READ );
   }
   else
   {
      //
      // Extract the appropriate fields based on the requested device
      //
      switch ( devnum )
      {
         case MIXERDEV_MASTER_VOLUME :
            volume.setMuted( audioinfo.output_muted );
            GainBalanceToVolume( audioinfo.play.gain,
                                 audioinfo.play.balance,
                                 volume );
            break;

         case MIXERDEV_RECORD_MONITOR :
            volume.setMuted(FALSE);
            volume.setAllVolumes( audioinfo.monitor_gain );
            break;

         case MIXERDEV_INTERNAL_SPEAKER :
         case MIXERDEV_HEADPHONE :
         case MIXERDEV_LINE_OUT :
            volume.setMuted( (audioinfo.play.port & devMask) ? FALSE : TRUE );
            GainBalanceToVolume( audioinfo.play.gain,
                                 audioinfo.play.balance,
                                 volume );
            break;

         case MIXERDEV_MICROPHONE :
         case MIXERDEV_LINE_IN :
         case MIXERDEV_CD :
            volume.setMuted( (audioinfo.record.port & devMask) ? FALSE : TRUE );
            GainBalanceToVolume( audioinfo.record.gain,
                                 audioinfo.record.balance,
                                 volume );
            break;

         default :
            return Mixer::ERR_NODEV;
      }
      return 0;
   }
}

//======================================================================
// FUNCTION    : Mixer::writeVolumeToHW
// DESCRIPTION : Write the specified audio settings to the hardware.
//======================================================================
int Mixer_SUN::writeVolumeToHW( int devnum, Volume volume )
{
   uint_t gain;
   uchar_t balance;
   uchar_t mute;

   //
   // Convert the Volume(left vol, right vol) to the Gain/Balance Sun uses
   //
   VolumeToGainBalance( volume, gain, balance );
   mute = volume.isMuted() ? 1 : 0;

   //
   // Read the current audio settings from the hardware
   //
   audio_info_t audioinfo;
   if ( ioctl( fd, AUDIO_GETINFO, &audioinfo ) < 0 )
   {
      return( Mixer::ERR_READ );
   }

   //
   // Now, based on the devnum that we are writing to, update the appropriate
   // volume field and twiddle the appropriate bitmask to enable/mute the
   // device as necessary.
   //
   switch ( devnum )
   {
      case MIXERDEV_MASTER_VOLUME :
         audioinfo.play.gain = gain;
         audioinfo.play.balance = balance;
         audioinfo.output_muted = mute;
         break;

      case MIXERDEV_RECORD_MONITOR :
         audioinfo.monitor_gain = gain;
         // no mute or balance for record monitor
         break;

      case MIXERDEV_INTERNAL_SPEAKER :
      case MIXERDEV_HEADPHONE :
      case MIXERDEV_LINE_OUT :
         audioinfo.play.gain = gain;
         audioinfo.play.balance = balance;
         if ( mute )
            audioinfo.play.port &= ~MixerSunPortMasks[devnum];
         else
            audioinfo.play.port |= MixerSunPortMasks[devnum];
         break;

      case MIXERDEV_MICROPHONE :
      case MIXERDEV_LINE_IN :
      case MIXERDEV_CD :
         audioinfo.record.gain = gain;
         audioinfo.record.balance = balance;
         if ( mute )
            audioinfo.record.port &= ~MixerSunPortMasks[devnum];
         else
            audioinfo.record.port |= MixerSunPortMasks[devnum];
         break;

      default :
         return Mixer::ERR_NODEV;
   }

   //
   // Now that we've updated the audioinfo struct, write it back to the hardware
   //
   if ( ioctl( fd, AUDIO_SETINFO, &audioinfo ) < 0 )
   {
      return( Mixer::ERR_WRITE );
   }
   else
   {
      return 0;
   }
}

//======================================================================
// FUNCTION    : Mixer::setRecsrcHW
// DESCRIPTION :
//======================================================================
bool Mixer_SUN::setRecsrcHW( int /* devnum */, bool /* on */ )
{
   return FALSE;
}

//======================================================================
// FUNCTION    : Mixer::isRecsrcHW
// DESCRIPTION : Returns true if the specified device is a record source.
//======================================================================
bool Mixer_SUN::isRecsrcHW( int devnum )
{
   switch ( devnum )
   {
      case MIXERDEV_MICROPHONE :
      case MIXERDEV_LINE_IN :
      case MIXERDEV_CD :
         return TRUE;

      default :
         return FALSE;
   }
}

//======================================================================
// FUNCTION    : Mixer::VolumeToGainBalance
// DESCRIPTION : Converts a Volume(left vol + right vol) into the
//               Gain/Balance values used by Sun.
//======================================================================
void Mixer_SUN::VolumeToGainBalance( Volume& volume, uint_t& gain, uchar_t& balance )
{
   if ( ( volume.channels() == 1 ) ||
        ( volume[Volume::LEFT] == volume[Volume::RIGHT] ) )
   {
      gain = volume[Volume::LEFT];
      balance = AUDIO_MID_BALANCE;
   }
   else
   {
      if ( volume[Volume::LEFT] > volume[Volume::RIGHT] )
      {
         gain = volume[Volume::LEFT];
         balance = AUDIO_LEFT_BALANCE +
           ( AUDIO_MID_BALANCE - AUDIO_LEFT_BALANCE ) *
           volume[Volume::RIGHT] / volume[Volume::LEFT];
      }
      else
      {
         gain = volume[Volume::RIGHT];
         balance = AUDIO_RIGHT_BALANCE -
           ( AUDIO_RIGHT_BALANCE - AUDIO_MID_BALANCE ) *
           volume[Volume::LEFT] / volume[Volume::RIGHT];
      }
   }
}

//======================================================================
// FUNCTION    : Mixer::GainBalanceToVolume
// DESCRIPTION : Converts Gain/Balance returned by Sun driver to the
//               Volume(left vol + right vol) format used by kmix.
//======================================================================
void Mixer_SUN::GainBalanceToVolume( uint_t& gain, uchar_t& balance, Volume& volume )
{
   if ( volume.channels() == 1 )
   {
      volume.setVolume( Volume::LEFT, gain );
   }
   else
   {
      if ( balance <= AUDIO_MID_BALANCE )
      {
         volume.setVolume( Volume::LEFT, gain );
         volume.setVolume( Volume::RIGHT, gain *
            ( balance - AUDIO_LEFT_BALANCE ) /
            ( AUDIO_MID_BALANCE - AUDIO_LEFT_BALANCE ) );
      }
      else
      {
         volume.setVolume( Volume::RIGHT, gain );
         volume.setVolume( Volume::LEFT, gain *
            ( AUDIO_RIGHT_BALANCE - balance ) /
            ( AUDIO_RIGHT_BALANCE - AUDIO_MID_BALANCE ) );
      }
   }
}
