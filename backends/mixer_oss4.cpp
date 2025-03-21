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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//OSS4 mixer backend for KMix by Yoper Team released under GPL v2 or later

#include "mixer_oss4.h"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#else
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif
#endif

#include <qregularexpression.h>
#include <qplatformdefs.h>

#include "core/mixer.h"

#include <klocalizedstring.h>


MixerBackend* OSS4_getMixer(Mixer *mixer, int device)
{
	return (new Mixer_OSS4(mixer, device));
}

Mixer_OSS4::Mixer_OSS4(Mixer *mixer, int device) : MixerBackend(mixer, device)
{
	if ( device == -1 ) m_devnum = 0;
	m_numExtensions = 0;
	m_fd = -1;
	m_ossVersion = 0;
	m_modifyCounter = -1;
}

bool Mixer_OSS4::CheckCapture(oss_mixext *ext)
{
	QString name = ext->extname;
#ifdef MIXF_RECVOL					// in 4Front only
	if (ext->flags & MIXF_RECVOL) return true;
#endif
	return (name.split('.').contains("in"));
}

Mixer_OSS4::~Mixer_OSS4()
{
	close();
}


static QString ucfirst(const QString &s)
{
	return (s.left(1).toUpper()+s.mid(1));
}


//classifies mixexts according to their name, last classification wins
MixDevice::ChannelType Mixer_OSS4::classifyAndRename(QString &name, int flags)
{
	MixDevice::ChannelType cType = MixDevice::UNKNOWN;
	QStringList classes = name.split(QRegularExpression(QLatin1String("[-,.]")));

#ifdef MIXF_PCMVOL
	if (flags & MIXF_PCMVOL  ||			// these flags in 4Front only
	    flags & MIXF_MONVOL  ||
	    flags & MIXF_MAINVOL)
	{
		cType = MixDevice::VOLUME;
	}
#endif

	for (QString &it : classes)
	{
		if ( it == "line" )
		{
			it = ucfirst(it);
			cType = MixDevice::EXTERNAL;

		} else
		if ( it == "mic" )
		{
			it = "Microphone";
			cType = MixDevice::MICROPHONE;
		} else
		if ( it == "vol" )
		{
			it = "Volume";
			cType = MixDevice::VOLUME;
		} else
		if ( it == "surr" )
		{
			it = "Surround";
			cType = MixDevice::SURROUND;
		} else
		if ( it == "bass" )
		{
			it = ucfirst(it);
			cType = MixDevice::BASS;
		} else
		if ( it == "treble" )
		{
			it = ucfirst(it);
			cType = MixDevice::TREBLE;
		} else
		if ( it.startsWith(QLatin1String("pcm")) )
		{
			it.replace( "pcm", "PCM" );
			cType = MixDevice::AUDIO;
		} else
		if ( it == "src" )
		{
			it = "Source";
		} else
		if ( it == "rec" )
		{
			it = "Recording";
		} else
		if ( it == "cd" )
		{
			it = it.toUpper();
			cType = MixDevice::CD;
		}
		if ( it.startsWith(QLatin1String("vmix")) )
		{
			it.replace( "vmix", "Virtual Mixer" );
			cType = MixDevice::VOLUME;
		} else
		if ( it.endsWith(QLatin1String("vol")) )
		{
			it = ucfirst(it);
			cType = MixDevice::VOLUME;
		} else
		if ( it.contains("speaker") )
		{
			it = ucfirst(it);
			cType = MixDevice::SPEAKER;	
		} else
		if ( it.contains("center") && it.contains("lfe"))
		{
			it = ucfirst(it);
			cType = MixDevice::SURROUND_LFE;
		} else
		if ( it.contains("rear") )
		{
			it = ucfirst(it);
			cType = MixDevice::SURROUND_CENTERBACK;
		} else
		if ( it.contains("front") )
		{
			it = ucfirst(it);
			cType = MixDevice::SURROUND_CENTERFRONT;
		} else
		if ( it.contains("headphone") )
		{
			it = ucfirst(it);
			cType = MixDevice::HEADPHONE;
		}
		else
		{
			it = ucfirst(it);
		}
	}
	name = classes.join( QLatin1String(  " " ));
	return cType;
}

int Mixer_OSS4::open()
{
	QByteArray devnode = "/dev/mixer";
	if ((m_fd = QT_OPEN(devnode, O_RDWR))<0)
	{
		if ( errno == EACCES )
			return Mixer::ERR_PERM;
		else
			return Mixer::ERR_OPEN;
	}

	/*
	 * Intentionally not wrapped - some systems may not support this ioctl, and therefore
	 * aren't OSSv4. No need to throw needless error messages at the user in that case.
	 */
	if( ::ioctl (m_fd, OSS_GETVERSION, &m_ossVersion) < 0)
	{
		return Mixer::ERR_OPEN;
	}
	if (m_ossVersion < 0x040000)
	{
		return Mixer::ERR_OPEN;
	}

	wrapIoctl( ioctl (m_fd, SNDCTL_MIX_NRMIX, &m_numMixers) );

	if ( m_mixDevices.isEmpty() )
	{
		if ( m_devnum >= 0 && m_devnum < m_numMixers )
		{
			m_numExtensions = m_devnum;
			bool masterChosen = false;
			bool masterHeuristicAvailable = false;
			bool saveAsMasterHeuristc = false;
			shared_ptr<MixDevice> masterHeuristic;

			oss_mixext ext;
			ext.dev = m_devnum;
			oss_mixerinfo mi;

			mi.dev = m_devnum;
			if ( wrapIoctl( ioctl (m_fd, SNDCTL_MIXERINFO, &mi) ) < 0 )
			{
				return Mixer::ERR_READ;
			}

			/* Mixer is disabled - this can happen, e.g. disconnected USB device */
			if (!mi.enabled)
			{
				return Mixer::ERR_READ;
			}

			QT_CLOSE(m_fd);
#ifdef HAVE_MIXERINFO_DEVNODE
			// oss_mixerinfo.devnode is in 4Front but not in BSD.
			devnode = mi.devnode;
#else
			// Assume that the device is "/dev/mixerN" as for OSS3.
			if (m_devnum>0) devnode += QByteArray::number(m_devnum);
#endif
			if ((m_fd = QT_OPEN(devnode, O_RDWR))<0)
			{
				return Mixer::ERR_OPEN;
			}

			if ( wrapIoctl( ioctl (m_fd, SNDCTL_MIX_NREXT, &m_numExtensions) ) < 0 )
			{
				//TO DO: more specific error handling here
				return Mixer::ERR_READ;
			}

			if( m_numExtensions == 0 )
			{
				return Mixer::ERR_OPEN;
			}

			ext.ctrl = 0;

			//read MIXT_DEVROOT, return Mixer::NODEV on error
			if ( wrapIoctl ( ioctl( m_fd, SNDCTL_MIX_EXTINFO, &ext) ) < 0 )
			{
				return Mixer::ERR_OPEN;
			}

			oss_mixext_root *root = (oss_mixext_root *) ext.data;
			registerCard(root->name);

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

				//skip unreadable controls
				if ( ext.flags & MIXF_READABLE 
#ifndef MIXT_MUTE
					&& (name.contains("mute"))
#endif
#ifdef MIXT_MUTE
					&& (name.contains("mute") || ext.flags == MIXT_MUTE)
#endif
				   )
				{
					continue;
				}
				//skip all vmix controls with the exception of outvol
				else if ( name.contains("vmix") && ! name.contains("enable") )
				{
					//some heuristic in case we got no exported main volume control
					if( name.contains("outvol") && ext.type != MIXT_ONOFF )
					{
						saveAsMasterHeuristc = true;
					}
					else
					{
						continue;
					}
				}


				//fix for old legacy names, according to Hannu's suggestions
				if ( name.contains('_') )
				{
					name = name.section('_',1,1).toLower();
				}

				isCapture = CheckCapture (&ext);
				Volume::ChannelMask chMask = Volume::MNONE;

				MixDevice::ChannelType cType = classifyAndRename(name, ext.flags);

				if ((ext.type == MIXT_STEREOSLIDER   ||
				     ext.type == MIXT_MONOSLIDER     ||
#ifdef MIXT_STEREOSLIDER16				// in 4Front only
				     ext.type == MIXT_STEREOSLIDER16 ||
#endif
#ifdef MIXT_MONOSLIDER16				// in 4Front only
				     ext.type == MIXT_MONOSLIDER16   ||
#endif
				     ext.type == MIXT_SLIDER))
				{
					if (
#ifdef MIXT_STEREOSLIDER16				// in 4Front only
						ext.type == MIXT_STEREOSLIDER16 ||
#endif
					        ext.type == MIXT_STEREOSLIDER)
					{
						chMask = Volume::ChannelMask(Volume::MLEFT|Volume::MRIGHT);
					}
					else
					{
						chMask = Volume::MLEFT;
					}

					Volume vol (ext.maxvalue, ext.minvalue, false, isCapture);
					vol.addVolumeChannels(chMask);

					MixDevice* md_ptr =	new MixDevice(_mixer,
									QString::number(i),
									name,
									cType);
                                        
                                        shared_ptr<MixDevice> md = md_ptr->addToPool();
                                        m_mixDevices.append(md);
					
					if(isCapture)
					{
						md->addCaptureVolume(vol);
					}
					else
					{
						md->addPlaybackVolume(vol);
					}

					if( saveAsMasterHeuristc && ! masterHeuristicAvailable )
					{
						masterHeuristic = md;
						masterHeuristicAvailable = true;
					}

					
#ifdef MIXF_MAINVOL					// in 4Front only
					// If it is not possible to choose the master
					// channel this way, then hope that the heuristic
					// will do that.
					if (!masterChosen && (ext.flags & MIXF_MAINVOL))
					{
						m_recommendedMaster = md;
						masterChosen = true;
					}
#endif
				}
				else if ( ext.type == MIXT_HEXVALUE )
				{
					chMask = Volume::ChannelMask(Volume::MLEFT);
					Volume vol (ext.maxvalue, ext.minvalue, false, isCapture);
					vol.addVolumeChannels(chMask);

					MixDevice* md_ptr =	new MixDevice(_mixer,
								      QString::number(i),
								      name,
								      cType);
                                        
                                        shared_ptr<MixDevice> md = md_ptr->addToPool();
                                        m_mixDevices.append(md);
					
					if(isCapture)
					{
						md->addCaptureVolume(vol);
					}
					else
					{
						md->addPlaybackVolume(vol);
					}
					
#ifdef MIXF_MAINVOL					// in 4Front only, see above
					if (!masterChosen && (ext.flags & MIXF_MAINVOL))
					{
						m_recommendedMaster = md;
						masterChosen = true;
					}
#endif
				}
				else if ( ext.type == MIXT_ONOFF 
#ifdef MIXT_MUTE					// in 4Front only
					|| ext.type == MIXT_MUTE
#endif
					)
				{
					Volume vol(1, 0, true, isCapture);
					
					if (isCapture)
						 vol.setSwitchType (Volume::CaptureSwitch);
					else if (ext.type == MIXT_ONOFF)
					{
						 vol.setSwitchType (Volume::SpecialSwitch);
					}
					
					MixDevice* md_ptr = new MixDevice(_mixer,
								      QString::number(i),
							 	      name,
								      cType);
                                        
                                        shared_ptr<MixDevice> md = md_ptr->addToPool();
                                        m_mixDevices.append(md);

                                        if(isCapture)
					{
						md->addCaptureVolume(vol);
					}
					else
					{
						md->addPlaybackVolume(vol);
					}
				}
				else if ( ext.type == MIXT_ENUM )
				{
					oss_mixer_enuminfo ei;
					ei.dev = m_devnum;
					ei.ctrl = i;

					if ( wrapIoctl( ioctl (m_fd, SNDCTL_MIX_ENUMINFO, &ei) ) != -1 )
					{
						Volume vol(ext.maxvalue, ext.minvalue,
									false, isCapture);
						vol.addVolumeChannel(VolumeChannel(Volume::LEFT));

						MixDevice* md_ptr = new MixDevice (_mixer,
						                               QString::number(i),
									       name,
						                               cType);

						QList<QString*> enumValuesRef;
						QString thisElement;

						for ( int j = 0; j < ei.nvalues; j++ )
						{
							thisElement = &ei.strings[ ei.strindex[j] ];

							if ( thisElement.isEmpty() )
							{
								thisElement = QString::number(j);
							}
							enumValuesRef.append( new QString(thisElement) );
						}
						md_ptr->addEnums(enumValuesRef);
                                        
                                                shared_ptr<MixDevice> md = md_ptr->addToPool();
                                                m_mixDevices.append(md);
					}
				}

				if ( ! masterChosen && masterHeuristicAvailable )
				{
					m_recommendedMaster = masterHeuristic;
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
	m_recommendedMaster.reset();
	closeCommon();
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
		default:
			l_s_errmsg = MixerBackend::errorText(mixer_error);
	}
	return l_s_errmsg;
}

int Mixer_OSS4::id2num(const QString& id)
{
	return id.toInt();
}

bool Mixer_OSS4::hasChangedControls()
{
	oss_mixerinfo minfo;

	minfo.dev = -1;
	if ( wrapIoctl( ioctl(m_fd, SNDCTL_MIXERINFO, &minfo) ) < 0 )
	{
		qCDebug(KMIX_LOG) << "Can't get mixerinfo from card!\n";
		return false;
	}

	if (!minfo.enabled)
	{
		// Mixer is disabled. Probably disconnected USB device or card is unavailable;
		qCDebug(KMIX_LOG) << "Mixer for card is disabled!\n";
		close();
		return false;
	}
	if (minfo.modify_counter == m_modifyCounter) return false;
	else m_modifyCounter = minfo.modify_counter;
	return true;
}

int Mixer_OSS4::readVolumeFromHW(const QString& id, shared_ptr<MixDevice> md)
{
	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = id2num(id);

	if ( wrapIoctl( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TO DO: more specific error handling
		return Mixer::ERR_READ;
	}

	Volume &vol = (CheckCapture (&extinfo)) ? md->captureVolume() : md->playbackVolume();
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
		if ( md->isMuted() && extinfo.type != MIXT_ONOFF )
		{
			return 0;
		}
		
		switch ( extinfo.type )
		{
#ifdef MIXT_MUTE		  			// in 4Front only
			case MIXT_MUTE:
#endif			  
			case MIXT_ONOFF:
				md->setMuted(mv.value != extinfo.minvalue);
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
#ifdef MIXT_MONOSLIDER16	  			// in 4Front only
			case MIXT_MONOSLIDER16:
				vol.setVolume(Volume::LEFT, mv.value & 0xffff);
				break;
#endif
#ifdef MIXT_STEREOSLIDER16	  			// in 4Front only
			case MIXT_STEREOSLIDER16:
				vol.setVolume(Volume::LEFT, mv.value & 0xffff);
				vol.setVolume(Volume::RIGHT, ( mv.value >> 16 ) & 0xffff);
				break;
#endif
		}
	}
	return 0;
}

int Mixer_OSS4::writeVolumeToHW(const QString& id, shared_ptr<MixDevice> md)
{
	int volume = 0;

	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = id2num(id);

	if ( wrapIoctl( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TO DO: more specific error handling
		qCDebug(KMIX_LOG) << "failed to read info for control " << id2num(id);
		return Mixer::ERR_READ;
	}

	Volume &vol = (CheckCapture (&extinfo)) ? md->captureVolume() : md->playbackVolume();

	switch ( extinfo.type )
	{
#ifdef MIXT_MUTE
		case MIXT_MUTE:
#endif	    
		case MIXT_ONOFF:
			volume = (md->isMuted()) ? (extinfo.maxvalue) : (extinfo.minvalue);
			break;

		case MIXT_MONOSLIDER:
			volume = vol.getVolume(Volume::LEFT);
			break;

		case MIXT_STEREOSLIDER:
			volume = vol.getVolume(Volume::LEFT) | ( vol.getVolume(Volume::RIGHT) << 8 );
			break;

		case MIXT_SLIDER:
			volume = vol.getVolume(Volume::LEFT);
			break;

#ifdef MIXT_MONOSLIDER16				// in 4Front only
		case MIXT_MONOSLIDER16:
			volume = vol.getVolume(Volume::LEFT);
			break;
#endif
#ifdef MIXT_STEREOSLIDER16				// in 4Front only
		case MIXT_STEREOSLIDER16:
			volume = vol.getVolume(Volume::LEFT) | ( vol.getVolume(Volume::RIGHT) << 16 );
			break;
#endif
		default:
			return -1;
	}

	mv.dev = extinfo.dev;
	mv.ctrl = extinfo.ctrl;
	mv.timestamp = extinfo.timestamp;
	mv.value = volume - extinfo.minvalue;

	if ( wrapIoctl ( ioctl (m_fd, SNDCTL_MIX_WRITE, &mv) ) < 0 )
	{
		qCDebug(KMIX_LOG) << "error writing to control" << extinfo.extname;
		return Mixer::ERR_WRITE;
	}
	return 0;
}

void Mixer_OSS4::setEnumIdHW(const QString& id, unsigned int idx)
{
	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = id2num(id);

	if ( wrapIoctl ( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TO DO: more specific error handling
		qCDebug(KMIX_LOG) << "failed to read info for control " << id2num(id);
		return;
	}

	if ( extinfo.type != MIXT_ENUM )
	{
		return;
	}


	//according to oss docs maxVal < minVal could be true - strange...
	unsigned int maxVal = static_cast<unsigned int>(extinfo.maxvalue);
	unsigned int minVal = static_cast<unsigned int>(extinfo.minvalue);
	if (maxVal < minVal) qSwap(maxVal, minVal);
	idx = qBound(minVal, idx, maxVal);

	mv.dev = extinfo.dev;
	mv.ctrl = extinfo.ctrl;
	mv.timestamp = extinfo.timestamp;
	mv.value = idx;

	if ( wrapIoctl ( ioctl (m_fd, SNDCTL_MIX_WRITE, &mv) ) < 0 )
	{
		/* Oops, can't write to mixer */
		qCDebug(KMIX_LOG) << "error writing to control" << extinfo.extname;
	}
}

unsigned int Mixer_OSS4::enumIdHW(const QString& id)
{
	oss_mixext extinfo;
	oss_mixer_value mv;

	extinfo.dev = m_devnum;
	extinfo.ctrl = id2num(id);

	if ( wrapIoctl ( ioctl(m_fd, SNDCTL_MIX_EXTINFO, &extinfo) ) < 0 )
	{
		//TO DO: more specific error handling
		//TO DO: check whether those return values are actually possible
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
			qCDebug(KMIX_LOG) << "A hardware level error occurred";
			break;
		}
		case EINVAL:
		{
			qCDebug(KMIX_LOG) << "Operation caused an EINVAL. You may have found a bug in kmix OSS4 module or in OSS4 itself";
			break;
		}
		case ENXIO:
		{
			qCDebug(KMIX_LOG) << "Operation index out of bounds or requested device does not exist - you likely found a bug in the kmix OSS4 module";
			break;
		}
		case EPERM:
		case EACCES:
		{
			qCDebug(KMIX_LOG) << errorText ( Mixer::ERR_PERM );
			break;
		}
		case ENODEV:
		{
			qCDebug(KMIX_LOG) << "kmix received an ENODEV error - are the OSS4 drivers loaded ?";
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


const char *OSS4_driverName = "OSS4";

QString Mixer_OSS4::getDriverName()
{
	return (OSS4_driverName);
}
