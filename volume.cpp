#include "volume.h"


Volume::Volume( int channels, int maxVolume ) : v_volumes(channels)
{
  v_maxVolume = maxVolume;
  v_muted = false;
}
Volume::Volume( const Volume &v ) : v_volumes( v.v_volumes )
{
  v_maxVolume = v.v_maxVolume;
  v_muted = v.v_muted;

}
