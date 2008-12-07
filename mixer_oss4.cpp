/*
 *              KMix -- KDE's full featured mini mixer
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
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

//OSS4 mixer backend for KMix by Yoper Team released under GPL v2 or later


#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <qregexp.h>
#include <kdebug.h>

// Since we're guaranteed an OSS setup here, let's make life easier
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
#include <sys/soundcard.h>
#else
#include <soundcard.h>
#endif

#include "mixer_oss4.h"
#include <klocale.h>

Mixer_Backend* OSS4_getMixer(int device)
{
	Mixer_Backend *l_mixer;
	l_mixer = new Mixer_OSS4(device);
	return l_mixer;
}

Mixer_OSS4::Mixer_OSS4(int device) : Mixer_Backend(device)
{
	if ( device == -1 ) m_devnum = 0;
	m_numExtensions = 0;
}

Mixer_OSS4::~Mixer_OSS4()
{
	close();
}

bool Mixer_OSS4::setRecsrcHW(int ctrlnum, bool on)
{
	return true;
}

//dummy implementation only
bool Mixer_OSS4::isRecsrcHW(int ctrlnum)
{
	return false;
}

//classifies mixexts according to their name, last classification wins
MixDevice::ChannelType Mixer_OSS4::classifyAndRename(QString &name, int flags)
{
	MixDevice::ChannelType cType = MixDevice::UNKNOWN;
	QStringList classes = QStringList::split ( QRegExp ( "[-,.]" ), name );


	if ( flags & MIXF_PCMVOL  ||
	     flags & MIXF_MONVOL  ||
	     flags & MIXF_MAINVOL )
	{
		cType = MixDevice::VOLUME;
	}

	for ( QStringList::Iterator it = classes.begin(); it != classes.end(); ++it )
	{
		if ( *it == "line" )
		{
			*it = "Line";
			cType = MixDevice::EXTERNAL;

		} else
		if ( *it == "mic" )
		{
			*it = "Microphone";
			cType = MixDevice::MICROPHONE;
		} else
		if ( *it == "vol" )
		{
			*it = "Volume";
			cType = MixDevice::VOLUME;
		} else
		if ( *it == "surr" )
		{
			*it = "Surround";
			cType = MixDevice::SURROUND;
		} else
		if ( *it == "bass" )
		{
			*it = "Bass";
			cType = MixDevice::BASS;
		} else
		if ( *it == "treble" )
		{
			*it = "Treble";
			cType = MixDevice::TREBLE;
		} else
		if ( (*it).startsWith ( "pcm" ) )
		{
			(*it).replace ( "pcm","PCM" );
			cType = MixDevice::AUDIO;
		} else
		if ( *it == "src" )
		{
			*it = "Source";
		} else
		if ( *it == "rec" )
		{
			*it = "Recording";
		} else
		if ( *it == "cd" )
		{
			*it = (*it).upper();
			cType = MixDevice::CD;
		}
		if ( (*it).startsWith("vmix") )
		{
			(*it).replace("vmix","Virtual Mixer");
			cType = MixDevice::VOLUME;
		} else
		if ( (*it).endsWith("vol") )
		{
			QChar &ref = (*it).ref(0);
			ref = ref.upper();
			cType = MixDevice::VOLUME;
		}
		else
		{
			QChar &ref = (*it).ref(0);
			ref = ref.upper();
		}
	}
	name = classes.join( " ");
	return cType;
}

int Mixer_OSS4::open()
{
	if ( (m_fd= ::open("/dev/mixer", O_RDWR)) < 0 )
	{
		if ( errno == EACCES )
			return Mixer::ERR_PERM;
		else
			return Mixer::ERR_OPEN;
	}

	if (wrapIoctl( ioctl (m_fd, OSS_GETVERSION, &m_ossVersion) ) < 0)
	{
		return Mixer::ERR_READ;
	}
	if (m_ossVersion < 0x040000)
	{
		return Mixer::ERR_NOTSUPP;
	}


	wrapIoctl( ioctl (m_fd, SNDCTL_MIX_NRMIX, &m_numMixers) );

	if ( m_mixDevices.isEmpty() )
	{
		if ( m_devnum >= 0 && m_devnum < m_numMixers )
		{
			m_numExtensions = m_devnum;
			bool masterChosen = false;
			oss_mixext ext;
			ext.dev = m_devnum;

			if ( wrapIoctl( ioctl (m_fd, SNDCTL_MIX_NREXT, &m_numExtensions) ) < 0 )
			{
				//TODO: more specific error handling here
				if ( errno == ENODEV ) return Mixer::ERR_NODEV;
				return Mixer::ERR_READ;
			}

			if( m_numExtensions == 0 )
			{
				return Mixer::ERR_NODEV;
			}

			ext.ctrl = 0;

			//read MIXT_DEVROOT, return Mixer::NODEV on error
			if ( wrapIoctl ( ioctl( m_fd, SNDCTL_MIX_EXTINFO, &ext) ) < 0 )
			{
				return Mixer::ERR_NODEV;
			}

			oss_mixext_root *root = (oss_mixext_root *) ext.data;
			m_mixerName = root->name;

			for ( int i = 1; i < m_numExtensions; i++ )
			{
				bool isCapture = false;

				ext.dev = m_devnum;
				ext.ctrl = i;
	
				//wrapIoctl handles reinitialization, cancel loading on EIDRM
				if ( wrapIoctl( ioctl( m_fd, SNDCTL_MIX_EXTINFO, &ext) ) == EIDRM )
				{
					return 0;
				}

				QString name = ext.extname;

				//skip vmix volume controls and mute controls
				if ( (name.find("vmix") > -1 && name.find( "pcm") > -1) ||
				     name.find("mute") > -1
#ifdef MIXT_MUTE
				|| (ext.type == MIXT_MUTE)
#endif
				)
				{
					continue;
				}

				//fix for old legacy names, according to Hannu's suggestions
				if ( name.contains('_') )
				{
					name = name.section('_',1,1).lower();
				}

				if ( ext.flags & MIXF_RECVOL || ext.flags & MIXF_MONVOL || name.find(".in") > -1  )
				{
					isCapture = true;
				}

				Volume::ChannelMask chMask = Volume::MNONE;

				MixDevice::ChannelType cType = classifyAndRename(name, ext.flags);

				if ( ext.type == MIXT_STEREOSLIDER16 ||
				        ext.type == MIXT_STEREOSLIDER   ||
				        ext.type == MIXT_MONOSLIDER16   ||
				        ext.type == MIXT_MONOSLIDER     ||
				        ext.type == MIXT_SLIDER
				   )
				{
					if ( ext.type == MIXT_STEREOSLIDER16 ||
					        ext.type == MIXT_STEREOSLIDER
					   )
					{
						if ( isCapture )
						{
							chMask = Volume::ChannelMask(Volume::MLEFT|Volume::MRIGHT);
						}
						else
						{
							chMask = Volume::ChannelMask(Volume::MLEFT|Volume::MRIGHT );
						}
					}
					else
					{
						if ( isCapture )
						{
							chMask = Volume::MLEFT;
						}
						else
						{
							chMask = Volume::MLEFT;
						}
					}

					Volume vol (chMask, ext.maxvalue, ext.minvalue, isCapture);

					MixDevice* md =	new MixDevice(i, vol, isCapture, true,
						                      name, cType, MixDevice::SLIDER);
					
					m_mixDevices.append(md);
					
					if ( !masterChosen && ext.flags & MIXF_MAINVOL )
					{
						m_recommendedMaster = md;
						masterChosen = true;
					}
				}
				else if ( ext.type == MIXT_ONOFF )
				{
					Volume vol;
					vol.setMuted(true);
					MixDevice* md = new MixDevice(i, vol, false, true, name, MixDevice::VOLUME, MixDevice::SWITCH);
					m_mixDevices.append(md);
				}
				else if ( ext.type == MIXT_ENUM )
				{
					oss_mixer_enuminfo ei;
					ei.dev = m_devnum;
					ei.ctrl = i;

					if ( wrapIoctl( ioctl (m_fd, SNDCTL_MIX_ENUMINFO, &ei) ) != -1 )
					{
						Volume vol(Volume::MLEFT, ext.maxvalue, ext.minvalue, false);

						MixDevice* md = new MixDevice (i, vol, false, false,
						                               name, MixDevice::UNKNOWN,
						                               MixDevice::ENUM);

						QPtrList<QString> &enumValuesRef = md->enumValues();
						QString thisElement;

						for ( int i = 0; i < ei.nvalues; i++ )
						{
							thisElement = &ei.strings[ ei.strindex[i] ];

							if ( thisElement.isEmpty() )
							{
								thisElement = QString::number(i);
							}
							enumValuesRef.append( new QString(thisElement) );
						}
						m_mixDevices.append(md);
					}
				}

			}
		}
		else
		{
			return -1;
		}
	}
	m_isOpen = true;
	return 0;
}

int Mixer_OSS4::close()
{
	m_isOpen = false;
	int l_i_ret = ::close(m_fd);
	m_mixDevices.clear();
	return l_i_ret;
}

QString Mixer_OSS4::errorText(int mixer_error)
{
	QString l_s_errmsg;

	switch( mixer_error )
	{
		case Mixer::ERR_PERM:
			l_s_errmsg = i18n("kmix: You do not have permission to access the mixer device.\n" \
			                   "Login as root and do a 'chmod a+rw /dev/mixer*' to allow the access.");
			break;
		case Mixer::ERR_OPEN:
			l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
			                  "Please check that the soundcard is installed and the\n" \
			                  "soundcard driver is loaded.\n" \
			                  "On Linux you might need to use 'insmod' to load the driver.\n" \
			                  "Use 'soundon' when using OSS4 from 4front.");
			break;
		case Mixer::ERR_NOTSUPP:
			l_s_errmsg = i18n("kmix expected an OSSv4 mixer module,\n" \
			                   "but instead found an older version.");
			break;
		default:
			l_s_errmsg = Mixer_Backend::errorText(mixer_error);
	}
	return l_s_errmsg;
}

int Mixer_OSS4::readVolumeFromHW(int ctrlnum, Volume &vol)
{

	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = ctrlnum;

	if ( wrapIoctl( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TODO: more specific error handling
		return Mixer::ERR_READ;
	}

	mv.dev = extinfo.dev;
	mv.ctrl = extinfo.ctrl;
	mv.timestamp = extinfo.timestamp;

	if ( wrapIoctl ( ioctl (m_fd, SNDCTL_MIX_READ, &mv) ) < 0 )
	{
		/* Oops, can't read mixer */
		return Mixer::ERR_READ;
	}
	else
	{
		if ( vol.isMuted() && extinfo.type != MIXT_ONOFF )
		{
			return 0;
		}

		if ( vol.isCapture() )
		{
			switch ( extinfo.type )
			{
				case MIXT_ONOFF:
					vol.setMuted(mv.value != extinfo.maxvalue);
					break;

				case MIXT_MONOSLIDER:
					vol.setVolume(Volume::LEFT, mv.value & 0xff);
					break;

				case MIXT_STEREOSLIDER:
					vol.setVolume(Volume::LEFT, mv.value & 0xff);
					vol.setVolume(Volume::RIGHT, ( mv.value >> 8 ) & 0xff);
					break;

				case MIXT_SLIDER:
					vol.setVolume(Volume::LEFT, mv.value);
					break;

				case MIXT_MONOSLIDER16:
					vol.setVolume(Volume::LEFT, mv.value & 0xffff);
					break;

				case MIXT_STEREOSLIDER16:
					vol.setVolume(Volume::LEFT, mv.value & 0xffff);
					vol.setVolume(Volume::RIGHT, ( mv.value >> 16 ) & 0xffff);
					break;
			}
		}
		else
		{
			switch( extinfo.type )
			{
				case MIXT_ONOFF:
					vol.setMuted(mv.value != extinfo.maxvalue);
					break;
				case MIXT_MONOSLIDER:
					vol.setVolume(Volume::LEFT, mv.value & 0xff);
					break;

				case MIXT_STEREOSLIDER:
					vol.setVolume(Volume::LEFT, mv.value & 0xff);
					vol.setVolume(Volume::RIGHT, ( mv.value >> 8 ) & 0xff);
					break;

				case MIXT_SLIDER:
					vol.setVolume(Volume::LEFT, mv.value);
					break;

				case MIXT_MONOSLIDER16:
					vol.setVolume(Volume::LEFT, mv.value & 0xffff);
					break;

				case MIXT_STEREOSLIDER16:
					vol.setVolume(Volume::LEFT, mv.value & 0xffff);
					vol.setVolume(Volume::RIGHT, ( mv.value >> 16 ) & 0xffff);
					break;
			}
		}
	}
	return 0;
}

int Mixer_OSS4::writeVolumeToHW(int ctrlnum, Volume &vol)
{
	int volume = 0;

	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = ctrlnum;

	if ( wrapIoctl( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TODO: more specific error handling
		kdDebug ( 67100 ) << "failed to read info for control " << ctrlnum << endl;
		return Mixer::ERR_READ;
	}

	if ( vol.isMuted() && extinfo.type != MIXT_ONOFF )
	{
		volume = 0;
	}
	else
	{
		switch ( extinfo.type )
		{
			case MIXT_ONOFF:
				volume = (vol.isMuted()) ? (extinfo.minvalue) : (extinfo.maxvalue);
				break;
			case MIXT_MONOSLIDER:
				volume = vol[Volume::LEFT];
				break;

			case MIXT_STEREOSLIDER:
				volume = vol[Volume::LEFT] | ( vol[Volume::RIGHT] << 8 );
				break;

			case MIXT_SLIDER:
				volume = vol[Volume::LEFT];
				break;

			case MIXT_MONOSLIDER16:
				volume = vol[Volume::LEFT];
				break;

			case MIXT_STEREOSLIDER16:
				volume = vol[Volume::LEFT] | ( vol[Volume::RIGHT] << 16 );
				break;
			default:
				return -1;
		}
	}

	mv.dev = extinfo.dev;
	mv.ctrl = extinfo.ctrl;
	mv.timestamp = extinfo.timestamp;
	mv.value = volume;

	if ( wrapIoctl ( ioctl (m_fd, SNDCTL_MIX_WRITE, &mv) ) < 0 )
	{
		kdDebug ( 67100 ) << "error writing: " << endl;
		return Mixer::ERR_WRITE;
	}
	return 0;
}

void Mixer_OSS4::setEnumIdHW(int ctrlnum, unsigned int idx)
{
	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = ctrlnum;

	if ( wrapIoctl ( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TODO: more specific error handling
		kdDebug ( 67100 ) << "failed to read info for control " << ctrlnum << endl;
		return;
	}

	if ( extinfo.type != MIXT_ENUM )
	{
		return;
	}


	//according to oss docs maxVal < minVal could be true - strange...
	unsigned int maxVal = (unsigned int) extinfo.maxvalue;
	unsigned int minVal = (unsigned int) extinfo.minvalue;

	if ( maxVal < minVal )
	{
		int temp;
		temp = maxVal;
		maxVal = minVal;
		minVal = temp;
	}

	if ( idx > maxVal || idx < minVal )
		idx = minVal;

	mv.dev = extinfo.dev;
	mv.ctrl = extinfo.ctrl;
	mv.timestamp = extinfo.timestamp;
	mv.value = idx;

	if ( wrapIoctl ( ioctl (m_fd, SNDCTL_MIX_WRITE, &mv) ) < 0 )
	{
		/* Oops, can't write to mixer */
		kdDebug ( 67100 ) << "error writing: " << endl;
	}
}

unsigned int Mixer_OSS4::enumIdHW(int ctrlnum)
{
	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = ctrlnum;

	if ( wrapIoctl ( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TODO: more specific error handling
		//TODO: check whether those return values are actually possible
		return Mixer::ERR_READ;
	}

	if ( extinfo.type != MIXT_ENUM )
	{
		return Mixer::ERR_READ;
	}

	mv.dev = extinfo.dev;
	mv.ctrl = extinfo.ctrl;
	mv.timestamp = extinfo.timestamp;

	if ( wrapIoctl ( ioctl (m_fd, SNDCTL_MIX_READ, &mv) ) < 0 )
	{
		/* Oops, can't read mixer */
		return Mixer::ERR_READ;
	}
	return mv.value;
}

int Mixer_OSS4::wrapIoctl(int ioctlRet)
{
	switch( ioctlRet )
	{
		case EIO:
		{
			kdDebug ( 67100 ) << "A hardware level error occured" << endl;
			break;
		}
		case EINVAL:
		{
			kdDebug ( 67100 ) << "Operation caused an EINVAL. You may have found a bug in kmix OSS4 module or in OSS4 itself" << endl;
			break;
		}
		case ENXIO:
		{
			kdDebug ( 67100 ) << "Operation index out of bounds or requested device does not exist - you likely found a bug in the kmix OSS4 module" << endl;
			break;
		}
		case EPERM:
		case EACCES:
		{
			kdDebug ( 67100 ) << errorText ( Mixer::ERR_PERM ) << endl;
			break;
		}
		case ENODEV:
		{
			kdDebug ( 67100 ) << "kmix received an ENODEV errors - are the OSS4 drivers loaded?" << endl;
			break;
		}
		case EPIPE:
		case EIDRM:
		{
			reinitialize();
		}

	}
	return ioctlRet;
}


QString OSS4_getDriverName()
{
	return "OSS4";
}

