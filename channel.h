//-*-C++-*-
#ifndef CHANNEL_H
#define CHANNEL_H

#include <qchkbox.h>
#include <qbttngrp.h>
#include <qlined.h>

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
