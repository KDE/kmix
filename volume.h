// -*-C++-*-
#ifndef VOLUME_H
#define VOLUME_H

#include <qarray.h>

class Volume
{
 public:
  enum ChannelID { LEFT = 0, RIGHT, CENTER,
                   REARLEFT, REARRIGHT, WOOFER, MAXCHANNELS };

  Volume( int channels = 2, int maxVolume = 100 );

  void setAllVolumes( int value )
       { v_volumes.fill( volrange(value) ); };
  void setVolume( int channel, int value )
       { v_volumes[ channel ] = volrange(value); };
  int  operator[](int channel ) const      { return v_volumes[channel]; };
  int  getVolume( int channel ) const      { return v_volumes[channel]; };
  int  maxVolume() const { return v_maxVolume; };
  int  channels() const { return v_volumes.size(); };

  void setMuted( bool val ) { v_muted = val; };
  bool isMuted() const      { return v_muted; };
private:
  int volrange( int vol ) { return vol > v_maxVolume ? v_maxVolume : vol; };
  int           v_maxVolume;

  bool          v_muted;
  QArray<int>   v_volumes;
};

class MonoVolume : public Volume
{
  MonoVolume( int maxVolume = 100 ) : Volume( 1, maxVolume ) {};
};

class StereoVolume : public Volume
{
  StereoVolume( int maxVolume = 100 ) : Volume( 2, maxVolume ) {};
};


#endif // VOLUME
