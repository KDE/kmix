#ifndef MixerSelectionInfo_h
#define MixerSelectionInfo_h

#include <qstring.h>
#include "mixer.h"

class MixerSelectionInfo
{
	public:
	MixerSelectionInfo(int num, QString name,
		MixDevice::DeviceCategory deviceTypeMask1 = ((MixDevice::DeviceCategory)0),
		MixDevice::DeviceCategory deviceTypeMask2 = ((MixDevice::DeviceCategory)0),
		MixDevice::DeviceCategory deviceTypeMask3 = ((MixDevice::DeviceCategory)0)
		);
	~MixerSelectionInfo();
	int m_num;
	QString m_name;
	MixDevice::DeviceCategory m_deviceTypeMask1;
	MixDevice::DeviceCategory m_deviceTypeMask2;
	MixDevice::DeviceCategory m_deviceTypeMask3;
};

#endif // MixerSelectionInfo

