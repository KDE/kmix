//-*-C++-*-

#ifndef MIXER_OSS4_H
#define MIXER_OSS4_H

#include "mixer_backend.h"
#include <sys/soundcard.h>

class Mixer_OSS4 : public Mixer_Backend
{
public:
  Mixer_OSS4(Mixer* mixer, int device);
  virtual ~Mixer_OSS4();

  virtual QString errorText(int mixer_error);
  virtual QString getDriverName();
  virtual bool CheckCapture(oss_mixext *ext);
  virtual bool prepareUpdateFromHW();
  virtual int readVolumeFromHW(const QString& id, shared_ptr<MixDevice> md);
  virtual int writeVolumeToHW(const QString& id, shared_ptr<MixDevice> md );
  virtual void setEnumIdHW(const QString& id, unsigned int idx);
  virtual unsigned int enumIdHW(const QString& id);

protected:

  MixDevice::ChannelType classifyAndRename(QString &name, int flags);

  int wrapIoctl(int ioctlRet);

  void reinitialize() { open(); close(); };
  virtual int open();
  virtual int close();

  int	  m_ossVersion;
  int     m_fd;
  int	  m_numMixers;
  int	  m_numExtensions;
  int	  m_modifyCounter;
  QString m_deviceName;
};
#endif
