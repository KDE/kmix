/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
 *
 * HP/UX-Port:	Copyright (C) 1999 by Helge Deller
 *			  deller@gmx.de
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

#include "mixer_hpux.h"

#warning "HP/UX mixer (maybe) doesn't work yet !"

#define HPUX_ERROR_OFFSET 1024

#define myGain 	AUnityGain	/* AUnityGain or AZeroGain */

#define GAIN_OUT_DIFF	((long) ((int)aMaxOutputGain(audio) - (int)aMinOutputGain(audio)))
#define GAIN_OUT_MIN	((long) aMinOutputGain(audio))
#define GAIN_IN_DIFF	((long) ((int)aMaxInputGain(audio)  - (int)aMinInputGain(audio)))
#define GAIN_IN_MIN	((long) aMinOutputGain(audio))

/* standard */
#define	ID_PCM			4

/* AInputSrcType: */	     /*OSS:*/
#define ID_IN_MICROPHONE 	7	/* AISTMonoMicrophone */
#define ID_IN_AUX		6	/* AISTLeftAuxiliary, AISTRightAuxiliary */

/* AOutputDstType: */
#define ID_OUT_INT_SPEAKER	0	/* AODTMonoIntSpeaker */

/* not yet implemented:
    AODTLeftJack,    AODTRightJack,
    AODTLeftLineOut,  AODTRightLineOut,
    AODTLeftHeadphone, AODTRightHeadphone

const char* MixerDevNames[32]={"Volume"  , "Bass"    , "Treble"    , "Synth"   , "Pcm"  ,    \
			       "Speaker" , "Line"    , "Microphone", "CD"      , "Mix"  ,    \
			       "Pcm2"    , "RecMon"  , "IGain"     , "OGain"   , "Line1",    \
			       "Line2"   , "Line3"   , "Digital1"  , "Digital2", "Digital3", \
			       "PhoneIn" , "PhoneOut", "Video"     , "Radio"   , "Monitor",  \
			       "3D-depth", "3D-center", "unknown"  , "unknown" , "unknown",  \
			       "unknown" , "unused" };
*/



Mixer_HPUX::Mixer_HPUX(int devnum) : Mixer_Backend(devnum)
{
  char ServerName[10];
  ServerName[0] = 0;
  audio = AOpenAudio(ServerName,NULL);
}

Mixer_HPUX::~Mixer_HPUX()
{
  if (audio) {
      ACloseAudio(audio,0);
      audio = 0;
  }
}


int Mixer_HPUX::open()
{
  if (audio==0) {
    return Mixer::ERR_OPEN;
  }
  else
  {
    /* Mixer is open. Now define properties */
    stereodevs = devmask = (1<<ID_PCM); /* activate pcm */
    recmask = 0;

    /* check Input devices... */
    if (AInputSources(audio) & AMonoMicrophoneMask) {
	    devmask	|= (1<<ID_IN_MICROPHONE);
	    recmask	|= (1<<ID_IN_MICROPHONE);
    }
    if (AInputSources(audio) & (ALeftAuxiliaryMask|ARightAuxiliaryMask)) {
	    devmask	|= (1<<ID_IN_AUX);
	    recmask	|= (1<<ID_IN_AUX);
	    stereodevs	|= (1<<ID_IN_AUX);
    }

    /* check Output devices... */
    if (AOutputDestinations(audio) & AMonoIntSpeakerMask) {
	    devmask	|= (1<<ID_OUT_INT_SPEAKER);
	    stereodevs	|= (1<<ID_OUT_INT_SPEAKER);
    }

/*  implement later:
    ----------------
    if (AOutputDestinations(audio) & AMonoLineOutMask)	devmask |= 64; // Line
    if (AOutputDestinations(audio) & AMonoJackMask)	devmask |= (1<<14); // Line1
    if (AOutputDestinations(audio) & AMonoHeadphoneMask)	devmask |= (1<<15); // Line2
*/

    MaxVolume = 255;

    long error = 0;
    ASetSystemPlayGain(audio, myGain, &error);
    if (error) errorText(error + HPUX_ERROR_OFFSET);
    ASetSystemRecordGain(audio, myGain, &error);
    if (error) errorText(error + HPUX_ERROR_OFFSET);

    i_recsrc = 0;
    m_isOpen = true;

    m_mixerName = "HP Mixer"; /* AAudioString(audio); */
    return 0;
  }
}

int Mixer_HPUX::close()
{
  m_isOpen = false;
  m_mixDevices.clear();
  return 0;
}


/*
void Mixer_HPUX::setDevNumName_I(int devnum)
{
  devname = "HP Mixer";
}
*/
bool Mixer_HPUX::setRecsrcHW( int devnum, bool on )
{
    return FALSE;
}

bool Mixer_HPUX::isRecsrcHW( int devnum )
{
    return FALSE;
}

int Mixer_HPUX::readVolumeFromHW( int devnum, Volume &vol )
{
    long Gain;
    long error = 0;
    int  vl,vr;

    switch (devnum) {
    case ID_OUT_INT_SPEAKER:	/* AODTMonoIntSpeaker */
	AGetSystemChannelGain(audio, ASGTPlay, ACTMono, &Gain, &error );
	vl = vr = (Gain-GAIN_OUT_MIN)*255 / GAIN_OUT_DIFF;
	vol.setVolume( Volume::LEFT, vl);
	vol.setVolume( Volume::RIGHT, vr);
printf("READ - Devnum: %d, Left: %d, Right: %d\n", devnum, vl, vr );
	break;

    case ID_IN_AUX:		/* AISTLeftAuxiliary, AISTRightAuxiliary */
    case ID_IN_MICROPHONE:	/* AISTMonoMicrophone */
	AGetSystemChannelGain(audio, ASGTRecord, ACTMono, &Gain, &error );
	vl = vr = (Gain-GAIN_IN_MIN)*255 / GAIN_IN_DIFF;
	vol.setVolume( Volume::LEFT, vl);
	vol.setVolume( Volume::RIGHT, vr);
	break;

    default:
	error = Mixer::ERR_NODEV - HPUX_ERROR_OFFSET;
	break;
    };

  return (error ? (error+HPUX_ERROR_OFFSET) : 0);
}

/*
        ASystemGainType         =     ASGTPlay, ASGTRecord, ASGTMonitor
	AChType                	=     ACTMono, ACTLeft, ACTRight
*/

int Mixer_HPUX::writeVolumeToHW( int devnum, Volume& vol )
{
    long Gain;
    long error = 0;
    int vl = vol.getVolume(Volume::LEFT);
    int vr = vol.getVolume(Volume::RIGHT);

    switch (devnum) {
    case ID_OUT_INT_SPEAKER:	/* AODTMonoIntSpeaker */
printf("WRITE - Devnum: %d, Left: %d, Right: %d\n", devnum, vl, vr);
	Gain = vl;	// only left Volume
	Gain = (Gain*GAIN_OUT_DIFF) / 255 - GAIN_OUT_MIN;
	ASetSystemChannelGain(audio, ASGTPlay, ACTMono, (AGainDB) Gain, &error );
	break;

    case ID_IN_MICROPHONE:	/* AISTMonoMicrophone */
	Gain = vl;	// only left Volume
	Gain = (Gain*GAIN_IN_DIFF) / 255 - GAIN_IN_MIN;
	ASetSystemChannelGain(audio, ASGTRecord, ACTMono, (AGainDB) Gain, &error );
	break;

    case ID_IN_AUX:		/* AISTLeftAuxiliary, AISTRightAuxiliary */
	Gain = (vl*GAIN_IN_DIFF) / 255 - GAIN_IN_MIN;
	ASetSystemChannelGain(audio, ASGTRecord, ACTLeft, (AGainDB) Gain, &error );
	Gain = (vr*GAIN_IN_DIFF) / 255 - GAIN_IN_MIN;
	ASetSystemChannelGain(audio, ASGTRecord, ACTRight, (AGainDB) Gain, &error );
	break;

    default:
	error = Mixer::ERR_NODEV - HPUX_ERROR_OFFSET;
	break;
    };
  return (error ? (error+HPUX_ERROR_OFFSET) : 0);
}


QString Mixer_HPUX::errorText(int mixer_error)
{
  QString l_s_errmsg;
  if (mixer_error >= HPUX_ERROR_OFFSET) {
      char errorstr[200];
      AGetErrorText(audio, (AError) (mixer_error-HPUX_ERROR_OFFSET),
                	    errorstr, sizeof(errorstr));
      printf("kmix: %s: %s\n",mixerName().data(), errorstr);
      l_s_errmsg = errorstr;
  } else
  switch (mixer_error)
    {
    case Mixer::ERR_OPEN:
		// should use i18n...
      l_s_errmsg = "kmix: HP-UX Alib-Mixer cannot be found.\n" \
			"Please check that you have:\n" \
			"  1. Installed the libAlib package  and\n" \
			"  2. started the Aserver program from the /opt/audio/bin directory\n";
      break;
    default:
      l_s_errmsg = Mixer_Backend::errorText(mixer_error);
      break;
    }
  return l_s_errmsg;
}

QString HPUX_getDriverName() {
        return "HPUX";
}

