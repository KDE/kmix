#ifndef MixerSelectionInfo_h
#define MixerSelectionInfo_h

#include <qstring.h>
#include "mixer.h"

class MixerSelectionInfo
{
public:
	MixerSelectionInfo(int num, QString name, bool tabDistribution, MixDevice::DeviceCategory deviceTypeMask);
	~MixerSelectionInfo();
	int m_num;
	QString m_name;
	bool m_tabDistribution;
	MixDevice::DeviceCategory m_deviceTypeMask;
};

#endif // MixerSelectionInfo

