//-*-C++-*-
#ifndef CHANNEL_H
#define CHANNEL_H

#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qlineedit.h>

/// Channel setup
class ChannelSetup
{
public:
  ChannelSetup(int num,QLineEdit *qleName, QCheckBox *qcbShow, QCheckBox *qcbSplit);
  ~ChannelSetup(void);
  /// Identification number of channel
  int		num;
  QLineEdit	*qleName;
  QCheckBox	*qcbShow;
  QCheckBox	*qcbSplit;
};

#endif
