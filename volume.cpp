#include "volume.h"


Volume::Volume( int channels, int maxVolume )
{
  v_volumes = QMemArray<int>( channels );
  v_maxVolume = maxVolume;
  v_muted = false;
}
