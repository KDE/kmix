//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
 * Software Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef MIXER_ALSA_H
#define MIXER_ALSA_H

// QT includes
#include <qlist.h>

// Forward QT includes
class QString;
class QSocketNotifier;

#include "mixer_backend.h"

class Mixer_ALSA : public Mixer_Backend
{
	public:
		Mixer_ALSA( int device = -1 );
		~Mixer_ALSA();
		
		virtual int  readVolumeFromHW( int devnum, Volume &vol );
		virtual int  writeVolumeToHW( int devnum, Volume &vol );
		virtual bool setRecsrcHW( int devnum, bool on);
		virtual bool isRecsrcHW( int devnum );
	        virtual void setEnumIdHW(int mixerIdx, unsigned int);
      		virtual unsigned int enumIdHW(int mixerIdx);
		virtual bool prepareUpdateFromHW();

                virtual bool needsPolling() { return false; }
                virtual void prepareSignalling( Mixer *mixer );

		virtual QString getDriverName();
		
	protected:
		virtual int open();
		virtual int close();
		
	private:
		int identify( snd_mixer_selem_id_t *sid );
		snd_mixer_elem_t* getMixerElem(int devnum);

		virtual QString errorText(int mixer_error);
		typedef QList<snd_mixer_selem_id_t *>AlsaMixerSidList;
		AlsaMixerSidList mixer_sid_list;
		typedef QList<snd_mixer_elem_t *> AlsaMixerElemList; // !! remove
		AlsaMixerElemList mixer_elem_list; // !! remove

                bool _initialUpdate;
		snd_mixer_t *_handle;
		QString devName;
	        struct pollfd  *m_fds;
                QSocketNotifier **m_sns;
		int m_count;

};

#endif
