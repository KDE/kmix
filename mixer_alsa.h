#ifndef MIXER_ALSA_H
#define MIXER_ALSA_H

// Forward QT includes
class QString;

class Mixer_ALSA : public Mixer
{
	public:
		Mixer_ALSA( int device = -1, int card = -1 );
		~Mixer_ALSA();
		
		virtual int  readVolumeFromHW( int devnum, Volume &vol );
		virtual int  writeVolumeToHW( int devnum, Volume vol );
		virtual bool setRecsrcHW( int devnum, bool on);
		virtual bool isRecsrcHW( int devnum );
		
	protected:
		virtual int	openMixer();
		virtual int releaseMixer();
		
	private:
		snd_mixer_t *handle;
#ifdef HAVE_ALSA_ASOUNDLIB_H
		int identify( snd_mixer_selem_id_t *sid );
		void readVolumeHW( snd_mixer_elem_t *elem, Volume &volume );
		QString mixer_card_name;
		QString mixer_device_name;
		QString card_id;
#elif defined(HAVE_SYS_ASOUNDLIB_H
		snd_mixer_groups_t  groups;
		snd_mixer_gid_t    *gid;
		int numChannels( int mask );
		int identify( int, const char* id );
#endif
};

#endif
