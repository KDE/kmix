#include "volume.h"


Volume::Volume( int channels, int maxVolume )
{
  v_volumes = QArray<int>( channels );
  v_maxVolume = maxVolume;
  v_muted = false;
}
