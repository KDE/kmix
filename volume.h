// -*-C++-*-
#ifndef VOLUME_H
#define VOLUME_H

#include <fstream>

#include <kdebug.h>

class Volume
{
 public:
  enum ChannelMask { MNONE     = 0,
                     MLEFT     = 1, MRIGHT     =   2, MCENTER =  4,
                     MREARLEFT = 8, MREARRIGHT =  16, MWOOFER = 32,
                     MCUSTOM1  =64, MCUSTOM2   = 128,
                     MALL=65535 };


 enum ChannelID { LEFT     = 0, RIGHT     = 1, CENTER = 2,
                  REARLEFT = 3, REARRIGHT = 4, WOOFER = 5,
                  CUSTOM1  = 6, CUSTOM2   = 7, CHIDMAX  = 7 };


  Volume( ChannelMask chmask = MALL, long maxVolume = 100, long minVolume=0 );
  Volume( const Volume &v );
  Volume( int channels, long maxVolume );



  // Set all volumes as given by vol
  void setAllVolumes(long vol);
  // Set all volumes to the ones given in vol
  void setVolume(const Volume &vol );
  // Set volumes as specified by the channel mask
  void setVolume( const Volume &vol, ChannelMask chmask);
  void setVolume( ChannelID chid, long volume);

  long getVolume(ChannelID chid);
  long getAvgVolume();
  long operator[](int);
  long maxVolume();
  long minVolume();
  int  channels(); // @deprecated
  int  count();

  void setMuted( bool val ) { _muted = val; };
  bool isMuted()      { return _muted; };

  friend std::ostream& operator<<(std::ostream& os, const Volume& vol);
  friend kdbgstream& operator<<(kdbgstream& os, const Volume& vol);

    // _channelMaskEnum[] and the following elements moved to public seection. operator<<() could not
    // access it, when private. Strange, as operator<<() is declared friend.
  static int _channelMaskEnum[8];
  bool          _muted;
  long          _chmask;
  long          _volumes[CHIDMAX+1];
  long          _maxVolume;
  long          _minVolume;

private:
  void init( ChannelMask chmask, int maxVolume, int minVolume );

  long volrange( int vol );
};

std::ostream& operator<<(std::ostream& os, const Volume& vol);
kdbgstream& operator<<(kdbgstream &os, const Volume& vol);

#endif // VOLUME

